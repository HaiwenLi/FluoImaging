
#ifndef _PIXEL_CONVERT_H_
#define _PIXEL_CONVERT_H_

#include "Util.h"
#include <cuda_runtime.h>
#include <vector_types.h>
#include <device_launch_parameters.h>

__global__  void rgb8_kernel(uchar3* dev_output_data, uchar* dev_original_data, int width, int height)
{
	int col = blockIdx.x*  blockDim.x + threadIdx.x;
	int row = blockIdx.y*  blockDim.y + threadIdx.y;
	if (col<width && row<height){
		dev_output_data[row*width + col].x = dev_original_data[row*width + col];
		dev_output_data[row*width + col].y = dev_original_data[row*width + col];
		dev_output_data[row*width + col].z = dev_original_data[row*width + col];
	}
}

__global__ void rgb16_kernel(uchar3* dev_output_data, ushort* dev_original_data, int width, int height, int rightShiftBits)
{
	int col = blockIdx.x*blockDim.x + threadIdx.x;
	int row = blockIdx.y*blockDim.y + threadIdx.y;
	ushort DataMask = 0xFF;
	DataMask <<=rightShiftBits;//left shift to get mask

	if (col<width && row<height){
		uchar pixel_data = (uchar)(((dev_original_data[row*width + col])&DataMask)>>rightShiftBits);//Andor pixel encoding is Mono16
		dev_output_data[row*width + col].x = pixel_data;
		dev_output_data[row*width + col].y = pixel_data;
		dev_output_data[row*width + col].z = pixel_data;
	}
}

//pixel convert for 8-bits
/*Parameteters:
dev_output_data:  output image (RGB) in the device memory;
dev_original_data: original image in the device memory;
width:   image width;
height: image height
*/
extern "C"
int pixelConvert8(uchar3* dev_output_data, uchar* dev_original_data, int width, int height)
{
	dim3 Db = dim3(16, 16);
	dim3 Dg = dim3((width + Db.x - 1) / Db.x, (height + Db.y - 1) / Db.y);
	rgb8_kernel << <Dg, Db >> >(dev_output_data, dev_original_data, width, height);
	cudaThreadSynchronize();

	cudaError error = cudaGetLastError();
	if (error != cudaSuccess){
		printf("rgb8_kernel() failed to launch, error = %d\n", error);
		return _CUDA_LAUNCH_FAILURE;
	}
	return _CUDA_LAUNCH_SUCCESS;
}

//pixel convert for 16-bits
/*Parameteters:
dev_output_data:  output image (RGB) in the device memory;
dev_original_data: original image in the device memory;
width:   image width;
height: image height
*/
extern "C"
int pixelConvert16(uchar3* dev_output_data, ushort* dev_original_data, int width, int height, int rightShiftBits)
{
	dim3 Db = dim3(16, 16);
	dim3 Dg = dim3((width + Db.x - 1) / Db.x, (height + Db.y - 1) / Db.y);
	rgb16_kernel << <Dg, Db >> >(dev_output_data, dev_original_data, width, height, rightShiftBits);
	cudaThreadSynchronize();

	cudaError error = cudaGetLastError();
	if (error != cudaSuccess){
		printf("rgb16_kernel() failed to launch, error = %d\n", error);
		return _CUDA_LAUNCH_FAILURE;
	}
	return _CUDA_LAUNCH_SUCCESS;
}
#endif //_PIXEL_CONVERT_H_