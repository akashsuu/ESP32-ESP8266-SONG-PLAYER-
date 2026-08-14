#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "esp_camera.h"
#include "config.h"

void initDisplay();
void showWaitingScreen();
void showConnectedScreen();
void renderCameraFrame(camera_fb_t* fb);

#endif // DISPLAY_DRIVER_H
