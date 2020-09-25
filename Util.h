
#ifndef _UTIL_H_
#define _UTIL_H_

#include "Camera_Params.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
using namespace std;

#define _CUDA_LAUNCH_SUCCESS 0
#define _CUDA_LAUNCH_FAILURE -1

#define GCAMP_CHANNEL       0x01
#define RFP_CHANNEL         0x02
#define IMAGING_CHANNEL_LEN 0x09

typedef unsigned char uchar;
typedef unsigned short ushort;

enum DeviceStatus{ OPENED, CONNECTED, DISCONNECTED }; //status for devices
enum SerialStatus { SERIAL_OPENED, SERIAL_CLOSED};              //status for serial port
enum DATATYPE{ UCHAR_TYPE, USHORT_TYPE };                       //image pixel format
enum DisplayWindowFlag{ HAMAMATSU_WINDOW, ANDOR_WINDOW, IO_WINDOW};
enum DisplayWindowOrientation{ NORMAL, FLIP_UP_DOWN, FLIP_LEFT_RIGHT, FLIP_BOTH, ROT_90, ROT_180, ROT_270 };
enum PixelEncodingType{ Mono8, Mono10, Mono12, Mono12Packed, Mono16, InvalidType };
enum DataRightShift{ BIT_0, BIT_1, BIT_2, BIT_3, BIT_4, BIT_5, BIT_6, BIT_7, BIT_8 };
enum ObjectiveLens{ NO_SELECTED, X10, X20, X40 };
enum ImagingChannelsSeq{SINGLE, COMP_G1_R1_G1_R1_G1_R1, COMP_G1_R1_G1_R1_G1, COMP_G1_R1_G1_R1};

//display window information
struct WindowInfo{
	int isLive;
	int image_width;
	int image_height;
	int image_stride;
	unsigned long image_num;
	void* image_data;
	DATATYPE data_type;
	int channelOffset;
	ImagingChannelsSeq imagingChannelSeq;
	PixelEncodingType image_pixelEncoding;
	DataRightShift dataRightShift;
	DisplayWindowFlag windowFlag;
	DisplayWindowOrientation windowOrientation;
};

struct ImageBuffer{
	long long timestamp;
	int image_width;
	int image_height;
	void* image_data;
	DATATYPE data_type;
};

struct ImageSize{
	int width;
	int height;
	int stride;
};

struct ImageRegion{
	int x_offset;
	int y_offset;
	int width;
	int height;
};

struct Range{
	double max;
	double min;
	double current;
};

//Position and value for status bar
struct PositionStatus{
	DisplayWindowFlag windowFlag;
	int currentRow;
	int currentCol;
	int value;
};

inline double Get_MotionTime(double speed, double distance){
	return (1.0e3*abs(distance)/speed); // ms
}
inline string GetErrorString(const string object, const string source, const string description){
	return (object + "::" + source + ": " + description + "\n");
}
inline double Square(double value){ 
	return value*value; 
}
void ConvertImagingChannelSeqToArray(ImagingChannelsSeq seq, char array[], int & len);
void CopyData(DATATYPE type, uchar* data, uchar*dst, int width, int height);

#endif //_UTIL_H_