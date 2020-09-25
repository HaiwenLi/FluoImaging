
#include "Hamamatsu_Camera.h"
#include "DevicePackage.h"
#include <QtWidgets/QMessageBox>
#include <QtCore/QString>

string Hamamatsu_Camera::OBJECT_NAME = "Hamamatsu_Camera";
string Hamamatsu_Camera::DEVICE_NAME = "Hamamatsu Camera";

Hamamatsu_Camera::Hamamatsu_Camera()
{
	hdcam = NULL;
	acquireImageThread = NULL;
	AcqBuffer = NULL;
	status = DISCONNECTED;
}

Hamamatsu_Camera::~Hamamatsu_Camera()
{
	Disconnect();
	if (AcqBuffer != NULL){
		delete AcqBuffer;
		AcqBuffer = NULL;
		hamamatsuWindowInfo.image_data = NULL;
	}
}

bool Hamamatsu_Camera::Connect()
{
	hdcam = Init_Open();
	if (hdcam == NULL){
		status = DISCONNECTED;
		//cout<<GetErrorString(OBJECT_NAME, "Connect()", "Cannot connect Hamamatsu camera");
		throw QException(OBJECT_NAME, "Connect()", "Cannot connect Hamamatsu camera");
		return false;
	}
	status = CONNECTED;
	cout<<"Connect to hamamatsu camera successfully"<<endl;
	return true;
}

//initialize DCAM-API and get HDCAM camera handle
HDCAM Hamamatsu_Camera::Init_Open()
{
	int32 nDevice;
	int32 iDevice;
	HDCAM dcam = NULL;

	cout <<"Initializing hamamatsu camera ..."<<endl;
	if (!dcam_init(NULL, &nDevice, NULL)){
		cout<<GetErrorString(OBJECT_NAME, "dcam_init()", "Cannot initialize the device");
	}
	if (nDevice<0){
		cout<<GetErrorString(OBJECT_NAME, "init_open()", "No Device");
	}

	//there are only one camera installed in this computer, then nDevice = 1
	iDevice = 0;
	if (!dcam_open(&dcam, iDevice, NULL)){
		dcam = NULL;
		dcam_uninit(NULL, NULL); //uninitialize DCAM-API
		cout<<GetErrorString(OBJECT_NAME, "dcam_open()", "Cannot open camera");
	}
	return dcam; //successfully open the camera
}

void Hamamatsu_Camera::Disconnect()
{
	ReleaseData();
	if (hdcam != NULL){
		dcam_close( hdcam );     //process camera termination
		dcam_uninit(NULL, NULL); //terminate the driver
		hdcam = NULL;
	}
	status = DISCONNECTED;
}

bool Hamamatsu_Camera::IsConnected()
{ 
	return ((hdcam != NULL) && (status == CONNECTED)); 
}

void Hamamatsu_Camera::ReleaseData()
{
	if (hamamatsuWindowInfo.image_data != NULL){
		hamamatsuWindowInfo.image_data = NULL;
	}
}

void Hamamatsu_Camera::Get_CameraInfo()
{
	char buf[256];
	if (hdcam != NULL){
		dcam_getstring(hdcam, DCAM_IDSTR_VENDOR, buf, sizeof(buf));
		vendor = string(buf);

		dcam_getstring(hdcam, DCAM_IDSTR_MODEL, buf, sizeof(buf));
		model = string(buf);

		dcam_getstring(hdcam, DCAM_IDSTR_BUS, buf, sizeof(buf));
		bus = string(buf);

		dcam_getstring(hdcam, DCAM_IDSTR_CAMERAID, buf, sizeof(buf));
		cameraID = string(buf);

		dcam_getstring(hdcam, DCAM_IDSTR_CAMERAVERSION, buf, sizeof(buf));
		cameraVersion = string(buf);

		dcam_getstring(hdcam, DCAM_IDSTR_DRIVERVERSION, buf, sizeof(buf));
		driverVersion = string(buf);

		dcam_getstring(hdcam, DCAM_IDSTR_MODULEVERSION, buf, sizeof(buf));
		moduleVersion = string(buf);

		dcam_getstring(hdcam, DCAM_IDSTR_DCAMAPIVERSION, buf, sizeof(buf));
		dcamAPIVersion = string(buf);
	}
}

bool Hamamatsu_Camera::Get_ExposureTimeRange(Range & range)
{
	if (hdcam == NULL){ return false; }

	DCAM_PARAM_FEATURE_INQ inq;
	memset(&inq, 0, sizeof(inq));
	inq.hdr.cbSize = sizeof(inq);
	inq.hdr.id = DCAM_IDPARAM_FEATURE_INQ;
	inq.hdr.iFlag = dcamparam_featureinq_featureid
		                 | dcamparam_featureinq_capflags
						 | dcamparam_featureinq_min
						 | dcamparam_featureinq_max
						 | dcamparam_featureinq_step
						 | dcamparam_featureinq_defaultvalue
						 | dcamparam_featureinq_units
						 ;

	inq.featureid = DCAM_IDFEATURE_EXPOSURETIME;
	if (!dcam_extended(hdcam, DCAM_IDMSG_GETPARAM, &inq, sizeof(inq))){
		char buf[256];
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Get_ExposureTimeRange()", string(buf));
		return false;
	} else{
		if (inq.hdr.oFlag & dcamparam_featureinq_min) range.min = inq.min;
		if (inq.hdr.oFlag & dcamparam_featureinq_max) range.max = inq.max;
		return true;
	}
}

bool Hamamatsu_Camera::Set_ExposureTime(double time)
{
	if (hdcam == NULL){ return false; }
	if (!dcam_setexposuretime(hdcam, time)){
		cout<<GetErrorString(OBJECT_NAME, "Set_ExposureTime()", "Fail to set exposure time");
		return false;
	}
	return true;
}

bool Hamamatsu_Camera::Get_ExposureTime(double& time)
{
	if (hdcam == NULL){ return false; }
	time = 0;
	if (!dcam_getexposuretime(hdcam, &time)){
		char buf[256];
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Get_ExposureTime()", string(buf));
		return false;
	}
	return true;
}

bool Hamamatsu_Camera::Get_FrameRateRange(Range &)
{
	return true;
}

bool Hamamatsu_Camera::Set_FrameRate(double)
{
	return true;
}

bool Hamamatsu_Camera::Get_FrameRate(double &)
{
	return true;
}

/* the trigger mode for hamamatsu camera
	 DCAM_TRIGMODE_INTERNAL: the internal synchronization mode;
	 DCAM_TRIGMODE_EDGE: the external synchronization edge mode;
	 DCAM_TRIGMODE_LEVEL: the external synchronization level mode;
	 DCAM_TRIGMODE_START: the trigger changes the camera mode from external trigger to internal;
	 DCAM_TRIGMODE_SYNCREADOUT: the trigger starts reading out. The exposure time is the period between two triggers;
*/
bool Hamamatsu_Camera::Set_TriggerMode(string strMode)
{
	if (hdcam == NULL){ return false; }

	long mode;
	if (strMode == "Internal"){
		mode = DCAM_TRIGMODE_INTERNAL;
		if (!dcam_setpropertyvalue(hdcam,DCAM_IDPROP_TRIGGERSOURCE,DCAMPROP_TRIGGERSOURCE__INTERNAL) 
			&& !dcam_setpropertyvalue(hdcam,DCAM_IDPROP_TRIGGERACTIVE,DCAMPROP_TRIGGERACTIVE__EDGE)
			&& !dcam_setpropertyvalue(hdcam,DCAM_IDPROP_TRIGGER_GLOBALEXPOSURE,DCAMPROP_TRIGGER_GLOBALEXPOSURE__DELAYED)){
			cout<<GetErrorString(OBJECT_NAME, "set_triggermode()", "fail to set trigger mode");
			return false;
		}
		cout<<"trigger mode : internal"<<endl;
	}
	else if (strMode == "External Level"){ // "External Level"
		mode = DCAM_TRIGMODE_LEVEL;
		if (!dcam_setpropertyvalue(hdcam,DCAM_IDPROP_TRIGGERSOURCE,DCAMPROP_TRIGGERSOURCE__EXTERNAL) 
			&& !dcam_setpropertyvalue(hdcam,DCAM_IDPROP_TRIGGERACTIVE,DCAMPROP_TRIGGERACTIVE__LEVEL)
			&& !dcam_setpropertyvalue(hdcam,DCAM_IDPROP_TRIGGER_GLOBALEXPOSURE,DCAMPROP_TRIGGER_GLOBALEXPOSURE__DELAYED)){
			cout<<GetErrorString(OBJECT_NAME, "set_triggermode()", "fail to set trigger mode");
			return false;
		}
		cout<<"trigger mode : external level"<<endl;
	}
	else if (strMode == "Global Reset"){ // "External Level"
		mode = DCAM_TRIGMODE_LEVEL;
		if (!dcam_setpropertyvalue(hdcam,DCAM_IDPROP_TRIGGERSOURCE,DCAMPROP_TRIGGERSOURCE__EXTERNAL) 
			&& !dcam_setpropertyvalue(hdcam,DCAM_IDPROP_TRIGGERACTIVE,DCAMPROP_TRIGGERACTIVE__LEVEL)
			&& !dcam_setpropertyvalue(hdcam,DCAM_IDPROP_TRIGGER_GLOBALEXPOSURE,DCAMPROP_TRIGGER_GLOBALEXPOSURE__GLOBALRESET)){
			cout<<GetErrorString(OBJECT_NAME, "set_triggermode()", "fail to set trigger mode");
			return false;
		}
		cout<<"Trigger mode : global reset"<<endl;
	}
	else{
		cout<<GetErrorString(OBJECT_NAME, "Set_TriggerMode()", "Invalid trigger mode");
		return false;
	}

	//if (!dcam_settriggermode(hdcam, mode)){
	//	cout<<geterrorstring(object_name, "set_triggermode()", "fail to set trigger mode");
	//	return false;
	//}
	return true;
}

bool Hamamatsu_Camera::Get_TriggerMode(string & strMode)
{
	if (hdcam == NULL){ return false; }

	long mode;
	dcam_gettriggermode(hdcam, &mode);

	if (mode == DCAM_TRIGMODE_INTERNAL){
		strMode == "Internal";
	}
	else if (mode == DCAM_TRIGMODE_LEVEL){
		strMode == "External Level";
	}

	return true;
}

//polarity (1): positive, 0: negative
bool Hamamatsu_Camera::Set_TriggerPolarity(int32 polarity)
{
	if (hdcam == NULL) { return false; }

	if (polarity == DCAM_TRIGPOL_POSITIVE || polarity == DCAM_TRIGPOL_NEGATIVE){
		if (!dcam_settriggerpolarity(hdcam, polarity)){
			cout<<GetErrorString(OBJECT_NAME, "Set_TriggerPolarity", "Fail to set trigger polarity");
			return false;
		}
		return true;
	}
	else{
		cout<<GetErrorString(OBJECT_NAME, "Set_TriggerPolarity", "Invalid trigger polarity");
		return false;
	}
}

bool Hamamatsu_Camera::Get_TriggerPolarity(int32 &polarity)
{
	if (hdcam == NULL){ return false; }

	if (!dcam_gettriggerpolarity(hdcam, &polarity)){
		cout<<GetErrorString(OBJECT_NAME, "Set_TriggerPolarity", "Fail to get trigger polarity");
		return false;
	}
	return true;
}

bool Hamamatsu_Camera::Get_CurrentTemperature(double& temp)
{
	if (hdcam == NULL){ return false; }
	DCAM_PARAM_FEATURE param;
	memset( &param, 0, sizeof(param) );
	param.hdr.cbSize = sizeof(param);                        //size of whole structure
	param.hdr.id = DCAM_IDPARAM_FEATURE;         //specify the kind of this structure
	param.hdr.iFlag = 0
		                     | dcamparam_feature_featureid //specify the member to be requested
							 | dcamparam_feature_flags
							 | dcamparam_feature_featurevalue
							 ;

	param.featureid = DCAM_IDFEATURE_TEMPERATURE;
	if ( !dcam_extended( hdcam, DCAM_IDMSG_GETPARAM, &param, sizeof(param) ) ){
		char buf[256];
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Get_CurrentTemperature()", string(buf))<<endl;
		return false;
	}

	//check output status
	if (param.hdr.oFlag & dcamparam_feature_featurevalue){
		temp = param.featurevalue;
		return true;
	}
	return false;
}

bool Hamamatsu_Camera::Set_SensorCool(bool on)
{
	if (hdcam == NULL){ return false; }
	if (on){
		if (!dcam_setpropertyvalue(hdcam, DCAM_IDPROP_SENSORCOOLER, (double)DCAM_FEATURE_FLAGS_COOLING_ON)){
			char buf[256];
			dcam_getlasterror(hdcam, buf, sizeof(buf));
			cout<<GetErrorString(OBJECT_NAME, "Set_SensorCool()", string(buf));
			return false;
		}
	}
	else{
		if (!dcam_setpropertyvalue(hdcam, DCAM_IDPROP_SENSORCOOLER, (double)DCAM_FEATURE_FLAGS_COOLING_OFF)){
			char buf[256];
			dcam_getlasterror(hdcam, buf, sizeof(buf));
			cout<<GetErrorString(OBJECT_NAME, "Set_SensorCool()", string(buf));
			return false;
		}
	}
	return true;
	/*DCAM_PARAM_FEATURE param;
	memset( &param, 0, sizeof(param));
	param.hdr.cbSize = sizeof(param);
	param.hdr.id = DCAM_IDPARAM_FEATURE;
	param.hdr.iFlag = dcamparam_feature_featureid
		                      | dcamparam_feature_flags
							  ;
	
	param.featureid = DCAM_IDFEATURE_TEMPERATURE;
	if (on){
		param.flags = DCAM_FEATURE_FLAGS_COOLING_ON;
	}
	else{
		param.flags = DCAM_FEATURE_FLAGS_COOLING_OFF;
	}
	if (!dcam_extended(hdcam, DCAM_IDMSG_SETPARAM, &param, sizeof(param))){
		char buf[256];
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Set_SensorCool()", string(buf));
	}*/
}

bool Hamamatsu_Camera::Get_SensorCool(bool& on)
{
	if (hdcam == NULL) { return false; }
	double value;
	if (! dcam_getpropertyvalue(hdcam, DCAM_IDPROP_SENSORCOOLER, &value)){
		char buf[256];
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Get_SensorCool()", string(buf))<<endl;
		return false;
	}
	if ( (int)value == DCAM_FEATURE_FLAGS_ON){
		on = true;
	}
	else if( (int)value == DCAM_FEATURE_FLAGS_OFF){
		on = false;
	}
	return true;
	/*DCAM_PARAM_FEATURE param;
	memset( &param, 0, sizeof(param));
	param.hdr.cbSize = sizeof(param);
	param.hdr.id = DCAM_IDPARAM_FEATURE;
	param.hdr.iFlag = dcamparam_feature_featureid
		                      | dcamparam_feature_flags
							  ;

	param.featureid = DCAM_IDFEATURE_TEMPERATURE;
	if (!dcam_extended(hdcam, DCAM_IDMSG_GETPARAM, &param, sizeof(param)) || (param.hdr.oFlag & dcamparam_feature_flags)==0){
		char buf[256];
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Get_SensorCool()", string(buf))<<endl;
		return false;
	}
	if (param.flags & DCAM_FEATURE_FLAGS_OFF){
		on = false; // sensor cooling is off
	} else{
		on = true; //sensor cooling is on
	}
	return true;*/
}

/*
The bitmap type and image data type are called before other parameters are set,
and before LUT settings are entered, to determine the operation mode of the digital camera.
*/
bool Hamamatsu_Camera::Set_PixelEncoding(PixelEncodingType type)
{
	if (hdcam == NULL){ return false; }
	DCAM_DATATYPE pixelType = DCAM_DATATYPE_UINT16;
	if (type == Mono8){
		pixelType = DCAM_DATATYPE_UINT8;
	}
	else if (type == Mono16){
		pixelType = DCAM_DATATYPE_UINT16;
	}
	if (!dcam_setdatatype(hdcam, pixelType)){
		cout<<GetErrorString(OBJECT_NAME, "Set_PixelEncoding()", "Fail to set pixel encoding")<<endl;
		return false;
	}
	return true;
}

bool Hamamatsu_Camera::Get_PixelEncoding(PixelEncodingType& type)
{
	if (hdcam == NULL){ return false; }
	bool result = false;
	DCAM_DATATYPE pixelType;
	dcam_getdatatype(hdcam, &pixelType);
	if (type == DCAM_DATATYPE_UINT8){
		type = Mono8;
		result = true;
	}
	else if (type == DCAM_DATATYPE_UINT16){
		type = Mono16;
		result = true;
	}
	return result;
}

bool Hamamatsu_Camera::Set_ReadoutRate(int rate)
{
	if (hdcam == NULL) { return false; }
	DCAM_PARAM_SCANMODE param;
	memset(&param, 0, sizeof(param));
	param.hdr.cbSize = sizeof(param);
	param.hdr.id = DCAM_IDPARAM_SCANMODE;
	param.hdr.iFlag = dcamparam_scanmode_speed;

	param.speed = rate;
	if (!dcam_extended(hdcam, DCAM_IDMSG_SETPARAM, &param, sizeof(param)) || !(param.hdr.oFlag & dcamparam_scanmode_speed) ){
		char buf[256];
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Set_ReadoutRate()", string(buf));
		return false;
	}
	return true;
}

bool Hamamatsu_Camera::Get_ReadoutRate(int& rate)
{
	if (hdcam == NULL){ return false; }
	DCAM_PARAM_SCANMODE param;
	memset(&param, 0, sizeof(param));
	param.hdr.cbSize = sizeof(param);
	param.hdr.id = DCAM_IDPARAM_SCANMODE;
	param.hdr.iFlag = dcamparam_scanmode_speed;

	if (!dcam_extended(hdcam, DCAM_IDMSG_GETPARAM, &param, sizeof(param))){
		char buf[256];
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Get_ReadoutRate()", string(buf))<<endl;
		rate = 1;
	} else{
		rate = param.speed;
	}
	return true;
}

bool Hamamatsu_Camera::Get_MaxReadoutRate(int& max_rate)
{
	if (hdcam == NULL){ return false; }
	DCAM_PARAM_SCANMODE_INQ inq;
	memset( &inq, 0, sizeof(inq));
	inq.hdr.cbSize = sizeof(inq);
	inq.hdr.id = DCAM_IDPARAM_SCANMODE_INQ;
	inq.hdr.iFlag = dcamparam_scanmodeinq_speedmax;

	if (!dcam_extended(hdcam, DCAM_IDMSG_GETPARAM, &inq, sizeof(inq))){
		char buf[256];
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Get_MaxReadoutRate()", string(buf))<<endl;
		max_rate = 1;
	} else{
		max_rate = inq.speedmax;
	}
	return true;
}

//Set subarray area of camera
bool Hamamatsu_Camera::Set_ImageSize(int left, int top, int width, int height)
{
	if (hdcam == NULL) { return false; }
	if (!dcamex_setsubarrayrect(left, top, width, height)){
		cout<<GetErrorString(OBJECT_NAME, "SetImageSize()", "Fail to set image size");
		return false;
	}
	else {
		ImageLeft = left;
		ImageTop = top;
		ImageWidth = width;
		ImageHeight = height;
		return true;
	}
}

bool Hamamatsu_Camera::Get_ImageSize(ImageSize &imageSize)
{
	if (hdcam == NULL) { return false; }

	SIZE size;
	if (dcam_getdatasize(hdcam, &size)){
		ImageWidth = size.cx;
		ImageHeight = size.cy;
		imageSize.width = ImageWidth;
		imageSize.height = ImageHeight;
		return true;
	} else{
		cout<<GetErrorString(OBJECT_NAME, "GetImageSize()", "Fail to get image size");
		return false;
	}
}

BOOL Hamamatsu_Camera::dcamex_getfeatureinq(long feature_id, long& cap_flags, 
	double& min, double& max, double& default_value, double& step)
{
	if (hdcam == NULL){
		return DCAMERR_INVALIDHANDLE;
	}
	DCAM_PARAM_FEATURE_INQ	inq;
	memset( &inq, 0, sizeof( inq ) );
	inq.hdr.cbSize = sizeof( inq );
	inq.hdr.id	= DCAM_IDPARAM_FEATURE_INQ;
	inq.hdr.iFlag	= dcamparam_featureinq_featureid
					| dcamparam_featureinq_capflags
					| dcamparam_featureinq_min
					| dcamparam_featureinq_max
					| dcamparam_featureinq_step
					| dcamparam_featureinq_defaultvalue
					| dcamparam_featureinq_units
					;
	inq.featureid	= feature_id;
	BOOL result;
	if (! (result = dcam_extended(hdcam, DCAM_IDMSG_GETPARAM, &inq, sizeof(inq))) ){
		return result;
	}
	if (inq.hdr.oFlag & dcamparam_featureinq_capflags) cap_flags = inq.capflags;
	if (inq.hdr.oFlag & dcamparam_featureinq_min) min = inq.min;
	if (inq.hdr.oFlag & dcamparam_featureinq_max) max = inq.max;
	if (inq.hdr.oFlag & dcamparam_featureinq_step) step = inq.step;
	if (inq.hdr.oFlag & dcamparam_featureinq_defaultvalue) default_value = inq.defaultvalue;
	return result;//result = DCAMERR_SUCCESS
}

BOOL Hamamatsu_Camera::dcamex_getfeature(long feature_id, double& feature_value)
{
	if (hdcam == NULL){
		return DCAMERR_INVALIDHANDLE;
	}
	DCAM_PARAM_FEATURE	param;
	memset( &param, 0, sizeof( param ));
	param.hdr.cbSize = sizeof( param);
	param.hdr.id	= DCAM_IDPARAM_FEATURE;
	param.hdr.iFlag		= dcamparam_feature_featureid
								 | dcamparam_feature_featurevalue
								;

	param.featureid = feature_id;
	BOOL result;
	if( !(result = dcam_extended(hdcam, DCAM_IDMSG_GETPARAM, &param, sizeof(param))) ){
		return result;
	}
	feature_value = param.featurevalue;
	return result;//result = DCAMERR_SUCCESS
}

BOOL Hamamatsu_Camera::dcamex_getfeature(long feature_id, double& feature_value, long& feature_flags)
{
	if (hdcam == NULL){
		return DCAMERR_INVALIDHANDLE;
	}
	DCAM_PARAM_FEATURE	param;
	memset( &param, 0, sizeof( param ));
	param.hdr.cbSize = sizeof( param);
	param.hdr.id	= DCAM_IDPARAM_FEATURE;
	param.hdr.iFlag		= dcamparam_feature_featureid
								 | dcamparam_feature_featurevalue
								 | dcamparam_feature_flags
								;

	param.featureid = feature_id;
	BOOL result;
	if( !(result = dcam_extended(hdcam, DCAM_IDMSG_GETPARAM, &param, sizeof(param))) ){
		return result;
	}
	feature_value = param.featurevalue;
	feature_flags = param.flags;
	return result;//result = DCAMERR_SUCCESS
}

BOOL Hamamatsu_Camera::dcamex_setfeaturevalue(long feature_id, double feature_value)
{
	if (hdcam == NULL){
		return DCAMERR_INVALIDHANDLE;
	}
	DCAM_PARAM_FEATURE	param;
	memset( &param, 0, sizeof( param ));
	param.hdr.cbSize = sizeof( param);
	param.hdr.id = DCAM_IDPARAM_FEATURE;
	param.hdr.iFlag = dcamparam_feature_featureid
		                      | dcamparam_feature_flags
							  | dcamparam_feature_featurevalue
							  ;

	param.featureid = feature_id;
	param.featurevalue = feature_value;
	return dcam_extended(hdcam, DCAM_IDMSG_SETPARAM, &param, sizeof(param));
}

BOOL Hamamatsu_Camera::dcamex_setfeatureflags(long feature_id, long feature_flags)
{
	if (hdcam == NULL){
		return DCAMERR_INVALIDHANDLE;
	}
	DCAM_PARAM_FEATURE	param;
	memset( &param, 0, sizeof( param ));
	param.hdr.cbSize = sizeof( param);
	param.hdr.id = DCAM_IDPARAM_FEATURE;
	param.hdr.iFlag = dcamparam_feature_featureid
		                      | dcamparam_feature_flags
							  | dcamparam_feature_featurevalue
							  ;

	param.featureid = feature_id;
	param.flags = feature_flags;
	return dcam_extended(hdcam, DCAM_IDMSG_SETPARAM, &param, sizeof(param));
}

BOOL Hamamatsu_Camera::dcamex_setfeature(long feature_id, double feature_value, long feature_flags)
{
	if (hdcam == NULL){
		return DCAMERR_INVALIDHANDLE;
	}
	DCAM_PARAM_FEATURE	param;
	memset( &param, 0, sizeof( param ));
	param.hdr.cbSize = sizeof( param);
	param.hdr.id = DCAM_IDPARAM_FEATURE;
	param.hdr.iFlag = dcamparam_feature_featureid
		                      | dcamparam_feature_flags
							  | dcamparam_feature_featurevalue
							  ;

	param.featureid = feature_id;
	param.featurevalue = feature_value;
	param.flags = feature_flags;
	return dcam_extended(hdcam, DCAM_IDMSG_SETPARAM, &param, sizeof(param));
}

BOOL Hamamatsu_Camera::dcamex_getsubarrayrect(int &left, int &top, int &width, int &height)
{
	if (hdcam == NULL){
		return DCAMERR_INVALIDHANDLE;
	}
	DCAM_PARAM_SUBARRAY		param;
	memset( &param, 0, sizeof( param ) );
	param.hdr.cbSize	= sizeof( param );
	param.hdr.id		    = DCAM_IDPARAM_SUBARRAY;
	param.hdr.iFlag		= dcamparam_subarray_hpos
						| dcamparam_subarray_vpos	
						| dcamparam_subarray_hsize
						| dcamparam_subarray_vsize
						;

	BOOL result;
	if( !(result = dcam_extended( hdcam, DCAM_IDMSG_GETPARAM, &param, sizeof( param ) )) ){
		return result;
	}

	left	    = param.hpos;
	top		= param.vpos;
	width	= param.hsize;
	height	= param.vsize;
	return TRUE;
}

BOOL Hamamatsu_Camera::dcamex_setsubarrayrect(int left, int top, int width, int height)
{
	if (hdcam == NULL){
		return DCAMERR_INVALIDHANDLE;
	}
	DCAM_PARAM_SUBARRAY		param;
	memset( &param, 0, sizeof( param ) );
	param.hdr.cbSize	= sizeof( param );
	param.hdr.id		    = DCAM_IDPARAM_SUBARRAY;
	param.hdr.iFlag		= dcamparam_subarray_hpos
						| dcamparam_subarray_vpos	
						| dcamparam_subarray_hsize
						| dcamparam_subarray_vsize
						;

	param.hpos	= left;
	param.vpos	= top;
	param.hsize	= width;
	param.vsize	= height;

	return dcam_extended( hdcam, DCAM_IDMSG_SETPARAM, &param, sizeof( param ) );
}

//capture one image
void  Hamamatsu_Camera::Capture()
{
	if (hdcam == NULL){ return; }
	char buf[256];
	ImageSize imageSize;

	if (hamamatsuWindowInfo.image_data != NULL){
		//delete hamamatsuWindowInfo.image_data;
		hamamatsuWindowInfo.image_data = NULL;
	}
	//pre capturing
	if (! dcam_precapture(hdcam, DCAM_CAPTUREMODE_SNAP)){
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Capture(): dcam_precapture()",  string(buf));
	}
	try{ Get_ImageSize(imageSize); } 
	catch(QException e){ throw e; }

	//allocate capturing buffer
	if (!dcam_allocframe(hdcam, 3)){
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Capture(): dcam_allocframe()", string(buf));
	}

	//start capturing
	_DWORD dw = DCAM_EVENT_FRAMEEND;
	uchar* pBuf;
	int32 rowBytes;
	if (!dcam_capture(hdcam)){
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Capture(): dcam_capture()", string(buf));
	}
	else{
		if (!dcam_wait(hdcam, &dw, 10000, NULL)){
			dcam_getlasterror(hdcam, buf, sizeof(buf));
			cout<<GetErrorString(OBJECT_NAME, "Capture(): dcam_wait()", string(buf));
		}
		else{
			//read data and display
			if (!dcam_lockdata(hdcam, (void**) &pBuf, &rowBytes, -1)){
				dcam_getlasterror(hdcam, buf, sizeof(buf));
				cout<<GetErrorString(OBJECT_NAME, "Capture(): dcam_lockdata()", string(buf));
			}
			if (rowBytes < 0){
				cout<<GetErrorString(OBJECT_NAME, "Capture(): dcam_lockdata()", "Invalid row bytes");
			}

			int image_width = imageSize.width;
			int image_height = imageSize.height;
			if (AcqBuffer == NULL){
				AcqBuffer = new uchar[image_width*image_height*sizeof(ushort)];
			}
			CopyData(USHORT_TYPE, pBuf, AcqBuffer, image_width, image_height);
			hamamatsuWindowInfo.image_data = (void*)AcqBuffer;
			hamamatsuWindowInfo.image_width = image_width;
			hamamatsuWindowInfo.image_height = image_height;
			hamamatsuWindowInfo.image_stride = rowBytes;
			hamamatsuWindowInfo.image_num = 1;
			hamamatsuWindowInfo.imagingChannelSeq = SINGLE;
			cout<<"image width: "<<image_width<<", image height: "<<image_height<<", rowBytes: "<<rowBytes<<endl;

			dcam_unlockdata(hdcam);
			emit DisplayImageSignal(HAMAMATSU_WINDOW); //display image
		}
		dcam_idle(hdcam); //stop capturing
	}
	dcam_freeframe(hdcam);
}

//capture images continuously
void Hamamatsu_Camera::Live()
{
	if (hdcam == NULL){ return; }
	char buf[256];
	ImageSize imageSize;

	//clear resources and make preparation to capture images continuously
	if (hamamatsuWindowInfo.image_data != NULL){
		dcam_unlockdata(hdcam);
		hamamatsuWindowInfo.image_data = NULL;
	}

	//pre capturing
	if (! dcam_precapture(hdcam, DCAM_CAPTUREMODE_SEQUENCE)){
		dcam_getlasterror(hdcam, buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "Live(): dcam_precapture()",  string(buf));
	}
	try{ Get_ImageSize(imageSize); } 
	catch(QException e){ throw e; }

	//start capturing images
	StartStreaming();
}

void Hamamatsu_Camera::StopLive()
{
	if (hdcam == NULL){ return; }
	StopStreaming();
}

void Hamamatsu_Camera::StartStreaming()
{
	if (acquireImageThread == NULL){
		acquireImageThread = new Hamamatsu_AcquireImageThread(this);
		acquireImageThread->StartThread();
		acquireImageThread->setPriority(QThread::HighPriority);
	}
}

void Hamamatsu_Camera::StopStreaming()
{
	if (acquireImageThread != NULL){
		acquireImageThread->StopThread();
		acquireImageThread->wait();
		delete acquireImageThread;
		acquireImageThread = NULL;
	}
}

//get the latest image buffer
void Hamamatsu_Camera::GrabLatestImageBuffer(ImageBuffer& imageBuffer)
{
	imageBuffer = acquireImageThread->Get_LatestImageBuffer();
}