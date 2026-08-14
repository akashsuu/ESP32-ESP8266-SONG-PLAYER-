#ifndef CAMERA_DRIVER_H
#define CAMERA_DRIVER_H

#include "esp_camera.h"
#include "config.h"

bool initCamera();
camera_fb_t* captureFrame();
void releaseFrame(camera_fb_t* fb);

#endif // CAMERA_DRIVER_H
