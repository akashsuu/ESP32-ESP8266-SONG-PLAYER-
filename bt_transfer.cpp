#include "bt_transfer.h"

static BluetoothSerial SerialBT;

bool initBluetooth() {
    return SerialBT.begin(BT_DEVICE_NAME);
}

bool isBluetoothConnected() {
    return SerialBT.hasClient();
}

bool sendPhotoFrame(camera_fb_t* fb) {
    if (!fb || !fb->buf || !isBluetoothConnected()) {
        return false;
    }

    uint32_t total_bytes = fb->len;
    uint16_t total_packets = (total_bytes + BT_PAYLOAD_CHUNK_SZ - 1) / BT_PAYLOAD_CHUNK_SZ;

    // 1. Build and Send Photo Header Packet
    PhotoHeader header;
    header.magic[0] = START_MAGIC_0;
    header.magic[1] = START_MAGIC_1;
    header.magic[2] = START_MAGIC_2;
    header.magic[3] = START_MAGIC_3;
    header.format = IMAGE_FMT_RGB565;
    header.width = fb->width;
    header.height = fb->height;
    header.image_size = total_bytes;
    header.total_packets = total_packets;

    // Compute Header XOR Checksum
    uint8_t h_chk = 0;
    uint8_t* p_hdr = (uint8_t*)&header;
    for (size_t i = 0; i < sizeof(PhotoHeader) - 1; i++) {
        h_chk ^= p_hdr[i];
    }
    header.checksum = h_chk;

    SerialBT.write((uint8_t*)&header, sizeof(PhotoHeader));
    SerialBT.flush();
    delay(10);

    // 2. Stream Data Chunks
    uint32_t offset = 0;
    for (uint16_t pkt = 0; pkt < total_packets; pkt++) {
        if (!isBluetoothConnected()) {
            return false;
        }

        uint16_t chunk_len = BT_PAYLOAD_CHUNK_SZ;
        if (offset + chunk_len > total_bytes) {
            chunk_len = total_bytes - offset;
        }

        PacketHeader pkt_hdr;
        pkt_hdr.magic[0] = PKT_MAGIC_0;
        pkt_hdr.magic[1] = PKT_MAGIC_1;
        pkt_hdr.packet_num = pkt;
        pkt_hdr.payload_len = chunk_len;

        // Calculate Payload Checksum
        uint8_t p_chk = 0;
        for (uint16_t j = 0; j < chunk_len; j++) {
            p_chk ^= fb->buf[offset + j];
        }
        pkt_hdr.checksum = p_chk;

        // Transmit Packet Header + Payload
        SerialBT.write((uint8_t*)&pkt_hdr, sizeof(PacketHeader));
        SerialBT.write(fb->buf + offset, chunk_len);
        SerialBT.flush();

        offset += chunk_len;
        vTaskDelay(pdMS_TO_TICKS(2)); // Pacing delay for BT SPP stack
    }

    // 3. Send Photo End Footer Marker
    PhotoFooter footer;
    footer.magic[0] = END_MAGIC_0;
    footer.magic[1] = END_MAGIC_1;
    footer.magic[2] = END_MAGIC_2;
    footer.magic[3] = END_MAGIC_3;

    SerialBT.write((uint8_t*)&footer, sizeof(PhotoFooter));
    SerialBT.flush();

    return true;
}
