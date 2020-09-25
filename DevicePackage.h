
#ifndef _DEVICE_PACKAGE_H_
#define _DEVICE_PACKAGE_H_

#include "Hamamatsu_Camera.h"

extern Hamamatsu_Camera* hamamatsuCamera;
extern WindowInfo hamamatsuWindowInfo;
extern int HAMAMATSU_DISPLAY_INTERVAL;
extern ImageBuffer* HamamatsuImageBuffers;
extern int HamamatsuSaveImageNum;
extern bool HamamatsuStartSaveImage;

extern PositionStatus positionStatus;
extern DisplayWindowFlag CurrentWindowFlag;
#endif // _DEVICE_PACKAGE_H_