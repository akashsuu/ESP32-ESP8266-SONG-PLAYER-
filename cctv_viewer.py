#!/usr/bin/env python3
"""
ESP32 CCTV Laptop OpenCV Viewer & Video Recorder
================================================
Connects to the ESP32 OV7670 Wi-Fi CCTV camera live MJPEG stream,
displays the live feed in a desktop window, and records video on your laptop.

Usage:
  python cctv_viewer.py [URL]

Example:
  python cctv_viewer.py http://192.168.1.50/stream
"""

import cv2
import sys
import time
import os
import argparse

def main():
    parser = argparse.ArgumentParser(description="ESP32 CCTV Laptop Live Viewer")
    parser.add_argument("url", nargs="?", help="ESP32 Stream URL (e.g. http://192.168.1.50/stream)")
    args = parser.parse_args()

    stream_url = args.url
    if not stream_url:
        stream_url = input("Enter your ESP32 CCTV IP URL (e.g. http://192.168.1.50/stream): ").strip()
        if not stream_url.startswith("http://"):
            stream_url = "http://" + stream_url
        if not stream_url.endswith("/stream"):
            stream_url = stream_url.rstrip("/") + "/stream"

    print(f"\nConnecting to ESP32 CCTV Live Feed: {stream_url} ...")
    cap = cv2.VideoCapture(stream_url)

    if not cap.isOpened():
        print("[-] Error: Unable to open stream! Make sure your Laptop is connected to the same Wi-Fi as the ESP32.")
        sys.exit(1)

    print("[+] Connected to ESP32 CCTV Stream!")
    print("\nKeyboard Controls:")
    print("  [s] - Save Snapshot Photo")
    print("  [r] - Start / Stop Video Recording")
    print("  [q] - Quit Viewer\n")

    is_recording = False
    out = None
    photo_count = 1
    rec_count = 1

    window_name = "ESP32 CCTV Live Camera Feed"
    cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)

    while True:
        ret, frame = cap.read()
        if not ret:
            print("[-] Connection lost. Retrying...")
            time.sleep(1)
            cap.open(stream_url)
            continue

        # Status Overlay
        display_frame = frame.copy()
        if is_recording:
            cv2.circle(display_frame, (30, 30), 10, (0, 0, 255), -1)
            cv2.putText(display_frame, "REC", (50, 35), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
            out.write(frame)
        else:
            cv2.circle(display_frame, (30, 30), 8, (0, 255, 0), -1)
            cv2.putText(display_frame, "LIVE", (45, 35), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

        cv2.imshow(window_name, display_frame)

        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            break
        elif key == ord('s'):
            filename = f"cctv_snapshot_{photo_count:03d}.jpg"
            cv2.imwrite(filename, frame)
            print(f"[✓] Saved Snapshot: {filename}")
            photo_count += 1
        elif key == ord('r'):
            if not is_recording:
                filename = f"cctv_video_{rec_count:03d}.avi"
                h, w = frame.shape[:2]
                fourcc = cv2.VideoWriter_fourcc(*'XVID')
                out = cv2.VideoWriter(filename, fourcc, 20.0, (w, h))
                is_recording = True
                print(f"[●] Started Recording: {filename}")
                rec_count += 1
            else:
                is_recording = False
                out.release()
                out = None
                print("[■] Stopped Recording.")

    cap.release()
    if out:
        out.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
