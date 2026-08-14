#ifndef BT_TRANSFER_H
#define BT_TRANSFER_H

#include <BluetoothSerial.h>
#include "esp_camera.h"
#include "config.h"
#include "protocol.h"

bool initBluetooth();
bool isBluetoothConnected();
bool sendPhotoFrame(camera_fb_t* fb);

#endif // BT_TRANSFER_H
