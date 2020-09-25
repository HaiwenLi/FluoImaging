
#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "Util.h"
#include <QtCore/QObject>
#include <string>
#include <iostream>

class Camera : public QObject
{
	Q_OBJECT
public:
	Camera(){}

	virtual bool Connect() = 0;
	virtual bool IsConnected() = 0;
	virtual void Disconnect() = 0;

	virtual void Capture() = 0;  //capture one image
	virtual void Live() = 0;     //capture images continuously
	virtual void StopLive() = 0;

	virtual void Get_CameraInfo() = 0;
	virtual bool Set_ImageSize(int left, int top, int width, int height) = 0;
	virtual bool Get_ImageSize(ImageSize &) = 0;
	virtual bool Set_TriggerMode(std::string mode) = 0;
	virtual bool Get_TriggerMode(std::string &) = 0;

	virtual bool Get_ExposureTimeRange(Range &) = 0;
	virtual bool Set_ExposureTime(double) = 0;
	virtual bool Get_ExposureTime(double &) = 0;
	virtual bool Get_FrameRateRange(Range &) = 0;
	virtual bool Set_FrameRate(double) = 0;
	virtual bool Get_FrameRate(double &) = 0;
};

#endif //_CAMERA_H_