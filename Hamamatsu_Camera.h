/***********************************************************************************
	Hamamastu Camera: 关于Hamamastu相机的操作
***********************************************************************************/
#ifndef _HAMAMATSU_CAMERA_H_
#define _HAMAMATSU_CAMERA_H_

//Important!!!
#ifdef _WIN32
#include <Windows.h>
#endif

#include "dcamapi.h"
#include "dcamprop.h"

#include "Camera.h"
#include "QException.h"
#include "Hamamatsu_AcquireImageThread.h"

#pragma comment(lib, "D:/SDK/DCAMSDK/lib/win64/dcamapi.lib")

class Hamamatsu_AcquireImageThread;
class Hamamatsu_Camera : public Camera{
	Q_OBJECT
public:
	static string OBJECT_NAME;
	static string DEVICE_NAME;

	explicit Hamamatsu_Camera();
	~Hamamatsu_Camera();
	Hamamatsu_AcquireImageThread* acquireImageThread;

	inline HDCAM Get_Handle(){ return hdcam; }
	bool Connect();
	bool IsConnected();
	void Disconnect();

	void Capture(); 
	void Live();
	void StopLive();
	
	bool Set_ImageSize(int left, int top, int width, int height);
	bool Get_ImageSize(ImageSize &);
	bool Set_TriggerMode(string);
	bool Get_TriggerMode(string &);
	bool Set_TriggerPolarity(int32 polarity);
	bool Get_TriggerPolarity(int32 &polarity);

	bool Set_SensorCool(bool);
	bool Get_SensorCool(bool &);
	void Get_CameraInfo();
	bool Get_CurrentTemperature(double &);
	void GrabLatestImageBuffer(ImageBuffer& imageBuffer); //get the latest image buffer

	bool Set_PixelEncoding(PixelEncodingType type);
	bool Get_PixelEncoding(PixelEncodingType& type);
	bool Set_ReadoutRate(int rate);
	bool Get_ReadoutRate(int& rate);
	bool Get_MaxReadoutRate(int& max_rate);

	bool Get_ExposureTimeRange(Range &);
	bool Set_ExposureTime(double);
	bool Get_ExposureTime(double &);
	bool Get_FrameRateRange(Range &);
	bool Set_FrameRate(double);
	bool Get_FrameRate(double &);
	
	inline void SendDisplayImageSignal(){
		emit DisplayImageSignal(HAMAMATSU_WINDOW);
	}

signals:
	void DisplayImageSignal(int);

protected:
	HDCAM Init_Open(); //initialize DCAM-API and get HDCAM camera handle
	void ReleaseData();
	void StartStreaming();
	void StopStreaming();

	/*All Features:
		DCAM_IDFEATURE_INITIALIZE
		DCAM_IDFEATURE_BRIGHTNESS
		DCAM_IDFEATURE_GAIN
		DCAM_IDFEATURE_CONTRAST
		DCAM_IDFEATURE_HUE
		DCAM_IDFEATURE_SATURATION
		DCAM_IDFEATURE_SHARPNESS
		DCAM_IDFEATURE_GAMMA
		DCAM_IDFEATURE_WHITEBALANCE	
		DCAM_IDFEATURE_PAN
		DCAM_IDFEATURE_TILT
		DCAM_IDFEATURE_ZOOM
		DCAM_IDFEATURE_IRIS
		DCAM_IDFEATURE_FOCUS
		DCAM_IDFEATURE_AUTOEXPOSURE
		DCAM_IDFEATURE_SHUTTER
		DCAM_IDFEATURE_EXPOSURETIME
		DCAM_IDFEATURE_TEMPERATURE
		DCAM_IDFEATURE_OPTICALFILTER
		DCAM_IDFEATURE_MECHANICALSHUTTER
		DCAM_IDFEATURE_LIGHTMODE
		DCAM_IDFEATURE_OFFSET
		DCAM_IDFEATURE_CONTRASTOFFSET
		DCAM_IDFEATURE_CONTRASTGAIN
		DCAM_IDFEATURE_AMPLIFIERGAIN
		DCAM_IDFEATURE_TEMPERATURETARGET
		DCAM_IDFEATURE_SENSITIVITY
		DCAM_IDFEATURE_TRIGGERTIMES
	*/
	BOOL dcamex_getfeatureinq(long feature_id, long& cap_flags, double& min, double& max, double& default_value, double& step);
	BOOL dcamex_getfeature(long feature_id, double& feature_value);
	BOOL dcamex_getfeature(long feature_id, double& feature_value, long& feature_flags);
	BOOL dcamex_setfeaturevalue(long feature_id, double feature_value);
	BOOL dcamex_setfeatureflags(long feature_id, long feature_flags);
	BOOL dcamex_setfeature(long feature_id, double feature_value, long feature_flags);
	BOOL dcamex_getsubarrayrect(int &left, int &top, int &width, int &height);
	BOOL dcamex_setsubarrayrect(int left, int top, int width, int height);

private:
	DeviceStatus status;
	HDCAM hdcam; //Hamamastu camera handle

	//camera information
	string vendor;               //Vendor information
	string model;                //Product name
	string bus;                    //Name of bus being used by camera
	string cameraID;           //Name identifying the camera
	string cameraVersion;   //Camera version
	string driverVersion;     //Driver version
	string moduleVersion;  //Version of DCAM Module
	string dcamAPIVersion;//Version of DCAM-API the Module supports

	uchar* AcqBuffer;
	int ImageLeft;
	int ImageTop;
	int ImageWidth;
	int ImageHeight;
};
#endif //_Hamamatsu_Camera_H_