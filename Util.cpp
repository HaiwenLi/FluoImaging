
#include "Util.h"

void ConvertImagingChannelSeqToArray(ImagingChannelsSeq seq, char array[], int & len){
	// array length is 9
	if (seq == SINGLE){
		array[0] = 0xFF;
		len = 1;
	} else if (seq == COMP_G1_R1_G1_R1_G1_R1){
		array[0] = GCAMP_CHANNEL;
		array[1] = RFP_CHANNEL;
		array[2] = GCAMP_CHANNEL;
		array[3] = RFP_CHANNEL;
		array[4] = GCAMP_CHANNEL;
		array[5] = RFP_CHANNEL;
		array[6] = 0xFF;
		len = 6;
	}else if (seq == COMP_G1_R1_G1_R1_G1) {
		array[0] = GCAMP_CHANNEL;
		array[1] = RFP_CHANNEL;
		array[2] = GCAMP_CHANNEL;
		array[3] = RFP_CHANNEL;
		array[4] = GCAMP_CHANNEL;
		array[5] = 0xFF;
		len = 5;
	} else if (seq == COMP_G1_R1_G1_R1){
		array[0] = GCAMP_CHANNEL;
		array[1] = RFP_CHANNEL;
		array[2] = GCAMP_CHANNEL;
		array[3] = RFP_CHANNEL;
		array[4] = 0xFF;
		len = 4;
	}
}

void CopyData(DATATYPE type, uchar* data, uchar* dst, int width, int height)
{
	int LoopHeight = height>>4;
	int NewTotalHeight = LoopHeight<<4;

	size_t rowBytes;
	if (type == USHORT_TYPE){
		rowBytes = sizeof(ushort)*width;
	} else{
		rowBytes = width;
	}
	for(int i=0; i<LoopHeight; ++i){
		int base = i<<4;
		memcpy(dst+base*rowBytes, data+base*rowBytes, rowBytes);         //1st line
		memcpy(dst+(base+1)*rowBytes, data+(base+1)*rowBytes, rowBytes); //2nd line
		memcpy(dst+(base+2)*rowBytes, data+(base+2)*rowBytes, rowBytes); //3rd line
		memcpy(dst+(base+3)*rowBytes, data+(base+3)*rowBytes, rowBytes); //4th line
		memcpy(dst+(base+4)*rowBytes, data+(base+4)*rowBytes, rowBytes); //5th line
		memcpy(dst+(base+5)*rowBytes, data+(base+5)*rowBytes, rowBytes); //6th line
		memcpy(dst+(base+6)*rowBytes, data+(base+6)*rowBytes, rowBytes); //7th line
		memcpy(dst+(base+7)*rowBytes, data+(base+7)*rowBytes, rowBytes); //8th line
		memcpy(dst+(base+8)*rowBytes, data+(base+8)*rowBytes, rowBytes); //9th line
		memcpy(dst+(base+9)*rowBytes, data+(base+9)*rowBytes, rowBytes); //10th line
		memcpy(dst+(base+10)*rowBytes, data+(base+10)*rowBytes, rowBytes); //11th line
		memcpy(dst+(base+11)*rowBytes, data+(base+11)*rowBytes, rowBytes); //12th line
		memcpy(dst+(base+12)*rowBytes, data+(base+12)*rowBytes, rowBytes); //13th line
		memcpy(dst+(base+13)*rowBytes, data+(base+13)*rowBytes, rowBytes); //14th line
		memcpy(dst+(base+14)*rowBytes, data+(base+14)*rowBytes, rowBytes); //15th line
		memcpy(dst+(base+15)*rowBytes, data+(base+15)*rowBytes, rowBytes); //16th line
	}

	int base = NewTotalHeight;
	for(int i=0; i<height-NewTotalHeight; ++i){
		memcpy(dst+(base+i)*rowBytes, data+(base+i)*rowBytes, rowBytes); //the remain lines
	}
}