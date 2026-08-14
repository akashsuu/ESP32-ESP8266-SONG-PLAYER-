#!/usr/bin/env python3
"""
ESP32-CAMERA Bluetooth SPP Photo Receiver & Decoder
===================================================
Receives framed photo stream over Bluetooth SPP, verifies packet checksums,
decodes raw RGB565 binary frames into JPEG/PNG images, and auto-saves them.

Usage:
  python receiver.py [PORT]

Example:
  Windows: python receiver.py COM5
  Linux:   python receiver.py /dev/rfcomm0
"""

import sys
import os
import time
import struct
import argparse
try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("Error: 'pyserial' is required. Install via: pip install pyserial")
    sys.exit(1)

try:
    from PIL import Image
except ImportError:
    print("Error: 'Pillow' is required for image saving. Install via: pip install Pillow")
    sys.exit(1)

# Protocol Constants
START_MAGIC = b'\xaa\xbb\xcc\xdd'
PKT_MAGIC   = b'\x55\xaa'
END_MAGIC   = b'\xde\xad\xbe\xef'

FMT_RGB565  = 0x01
FMT_JPEG    = 0x02


def decode_rgb565(raw_data: bytes, width: int, height: int) -> Image.Image:
    """
    Converts raw 16-bit RGB565 byte array into a 24-bit PIL RGB Image.
    """
    img = Image.new("RGB", (width, height))
    pixels = img.load()
    
    # Process 2 bytes per pixel
    idx = 0
    for y in range(height):
        for x in range(width):
            if idx + 1 >= len(raw_data):
                break
            # Extract 16-bit RGB565 value (Big-Endian from ESP32 camera buffer)
            b1 = raw_data[idx]
            b2 = raw_data[idx + 1]
            idx += 2
            
            val = (b1 << 8) | b2
            
            # Extract R5, G6, B5 components
            r = ((val >> 11) & 0x1F) << 3
            g = ((val >> 5)  & 0x3F) << 2
            b = (val & 0x1F) << 3
            
            pixels[x, y] = (r, g, b)
            
    return img


def find_esp32_port():
    """Attempts to auto-detect Bluetooth serial COM port."""
    ports = list(serial.tools.list_ports.comports())
    for port in ports:
        if "Bluetooth" in port.description or "ESP32" in port.description or "Standard Serial over Bluetooth" in port.description:
            return port.device
    if ports:
        return ports[0].device
    return None


def receive_photos(port_name: str, baud_rate: int = 115200):
    print(f"Connecting to ESP32-CAMERA on {port_name}...")
    try:
        ser = serial.Serial(port_name, baudrate=baud_rate, timeout=2.0)
    except Exception as e:
        print(f"Failed to open port {port_name}: {e}")
        print("\nTroubleshooting:")
        print("1. Pair your phone/PC to 'ESP32-CAMERA' in Bluetooth Settings.")
        print("2. Make sure the correct COM port (Windows) or /dev/rfcomm device (Linux) is specified.")
        return

    print("Connected successfully! Waiting for shutter button presses...")
    photo_count = 1

    while True:
        try:
            # 1. Search for 4-byte Start-of-Photo Header Magic
            byte_buf = bytearray()
            while True:
                b = ser.read(1)
                if not b:
                    continue
                byte_buf.append(b[0])
                if len(byte_buf) > 4:
                    byte_buf.pop(0)
                if bytes(byte_buf) == START_MAGIC:
                    break

            print("\n[+] Photo Stream Header Detected!")
            
            # Read remainder of 24-byte Header
            header_bytes = ser.read(20) # 24 - 4 magic bytes
            if len(header_bytes) < 20:
                print("[-] Incomplete header received. Skipping...")
                continue

            full_header = START_MAGIC + header_bytes
            fmt, width, height, image_size, total_packets, checksum = struct.unpack("<BHHIBB", full_header[4:17])

            print(f"    Format: RGB565 (0x{fmt:02X}) | Resolution: {width}x{height} | Size: {image_size} bytes | Packets: {total_packets}")

            # 2. Receive Data Chunks
            image_buffer = bytearray()
            packets_received = 0

            while packets_received < total_packets:
                # Find Packet Magic
                pkt_magic_buf = bytearray()
                while len(pkt_magic_buf) < 2:
                    b = ser.read(1)
                    if not b:
                        break
                    pkt_magic_buf.append(b[0])

                if bytes(pkt_magic_buf) != PKT_MAGIC:
                    continue

                # Read Packet Header (5 bytes: packet_num (2B), payload_len (2B), checksum (1B))
                pkt_meta = ser.read(5)
                if len(pkt_meta) < 5:
                    break
                
                pkt_num, payload_len, p_checksum = struct.unpack("<HHB", pkt_meta)
                payload = ser.read(payload_len)

                # Verify Checksum
                calc_chk = 0
                for pb in payload:
                    calc_chk ^= pb

                if calc_chk != p_checksum:
                    print(f"    [!] Warning: Checksum mismatch in packet {pkt_num}")

                image_buffer.extend(payload)
                packets_received += 1
                
                # Progress indicator
                progress = int((packets_received / total_packets) * 100)
                print(f"\r    Receiving Photo: [{progress:3d}%] ({packets_received}/{total_packets} packets)", end="")

            print()

            # 3. Read End Footer
            footer_bytes = ser.read(4)
            if footer_bytes == END_MAGIC:
                print("    [✓] Photo End Marker Verified!")

            # 4. Decode & Save Image
            filename = f"photo_{photo_count:03d}.jpg"
            print(f"    Decoding RGB565 data and saving to '{filename}'...")
            
            img = decode_rgb565(image_buffer, width, height)
            img.save(filename, "JPEG", quality=95)
            
            print(f"    [SUCCESS] Saved {filename} ({os.path.getsize(filename)} bytes)\n")
            photo_count += 1

        except KeyboardInterrupt:
            print("\nExiting Bluetooth receiver script...")
            ser.close()
            break
        except Exception as e:
            print(f"\n[!] Error during reception: {e}")
            time.sleep(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="ESP32-CAMERA Bluetooth Photo Receiver")
    parser.add_argument("port", nargs="?", help="Serial COM port (e.g. COM5 or /dev/rfcomm0)")
    args = parser.parse_args()

    port = args.port
    if not port:
        port = find_esp32_port()
        if not port:
            print("No Bluetooth serial port auto-detected.")
            port = input("Please enter your ESP32 Bluetooth COM port (e.g. COM5): ").strip()

    receive_photos(port)
