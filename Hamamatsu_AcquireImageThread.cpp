
#include "Hamamatsu_AcquireImageThread.h"
#include "DevicePackage.h"

#include <QtCore\QDateTime>

string Hamamatsu_AcquireImageThread::OBJECT_NAME = "Hamamatsu_AcquireImageThread";
Hamamatsu_AcquireImageThread::Hamamatsu_AcquireImageThread(Hamamatsu_Camera* camera):Camera(camera)
{
	isStopAcquireImage = false;
	hasAllocatedFrames = false;
	Buffer_Index = 0;
	SaveImage_Index = 0;
	ImageCount = 0;
	CreateBuffers();
}

Hamamatsu_AcquireImageThread::~Hamamatsu_AcquireImageThread()
{
	if (hasAllocatedFrames){
		dcam_freeframe( Camera->Get_Handle() );
	}
	Camera = NULL;
	ClearBuffers();
}

void Hamamatsu_AcquireImageThread::StopThread()
{
	isStopAcquireImage = true;
}

void Hamamatsu_AcquireImageThread::StartThread()
{
	isStopAcquireImage = false;
	start();
}

void Hamamatsu_AcquireImageThread::run()
{
	//allocate capturing buffer
	char buf[256];
	if (!dcam_allocframe(Camera->Get_Handle(), 3)){
		dcam_getlasterror(Camera->Get_Handle(), buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "run(): dcam_allocframe()", string(buf))<<endl<<"exit acquiring image thread ..."<<endl;
		return;
	} else{
		hasAllocatedFrames = true;
	}

	if (!dcam_capture( Camera->Get_Handle())){
		dcam_getlasterror(Camera->Get_Handle(), buf, sizeof(buf));
		cout<<GetErrorString(OBJECT_NAME, "run(): dcam_allocframe()", string(buf))<<endl<<"exit acquiring image thread ..."<<endl;
		//release capturing buffer
		dcam_freeframe(Camera->Get_Handle());
		hasAllocatedFrames = false;
		return;
	}
	
	//get image size
	ImageSize imageSize;
	Camera->Get_ImageSize(imageSize);
	image_width = imageSize.width;
	image_height = imageSize.height;

	//start capturing images
	while (true){
		if (isStopAcquireImage){
			dcam_idle(Camera->Get_Handle());//stop capturing
			break;
		}
		AcquireImage();
	}
}

void Hamamatsu_AcquireImageThread::CreateBuffers()
{
	_DWORD frameByte;
	char buf[256];
	if (!dcam_getdataframebytes( Camera->Get_Handle(), &frameByte)){
		dcam_getlasterror(Camera->Get_Handle(), buf, sizeof(buf));
		throw QException(OBJECT_NAME, "CreateBuffers(): dcam_getdataframebytes()", string(buf));
	}
	for (int i=0; i<HAMAMATSU_BUFFER_SIZE; ++i){
		acqBuffers[i] = NULL;
		circularBuffers[i] = NULL;
	}
	//create cicular buffer
	for (int i=0; i<HAMAMATSU_BUFFER_SIZE; ++i){
		acqBuffers[i] = new uchar[frameByte];
		circularBuffers[i] = acqBuffers[i];
	}
}

void Hamamatsu_AcquireImageThread::ClearBuffers()
{
	for (int i=0; i<HAMAMATSU_BUFFER_SIZE;++i){
		if (acqBuffers[i] != NULL){
			delete acqBuffers[i];
			acqBuffers[i] = NULL;
			circularBuffers[i] = NULL;
		}
	}
	hamamatsuWindowInfo.image_data = NULL;
}

void Hamamatsu_AcquireImageThread::AcquireImage()
{
	_DWORD dw = DCAM_EVENT_FRAMEEND;
	int32 rowBytes;
	uchar* pBuf;

	// Test time consumption of AcquireImage
	qint64 start_time = QDateTime::currentMSecsSinceEpoch();

	if( dcam_wait(Camera->Get_Handle(), &dw, 100, NULL) ){
		if (dcam_lockdata(Camera->Get_Handle(), (void**) &pBuf, &rowBytes, -1)){ //get data and emit display signal
			//rowBytes will be negetive sometimes
			if (rowBytes<0){
				dcam_unlockdata(Camera->Get_Handle());
				return;
			}
			//cout << "AcquireImage: pic "<<Image_Count<<endl;
			//cout<<"image width: "<<image_width<<", image height: "<<image_height<<", rowBytes: "<<rowBytes<<endl;

			//save images
			if (HamamatsuImageBuffers!=NULL && HamamatsuStartSaveImage){
				HamamatsuImageBuffers[SaveImage_Index].timestamp = QDateTime::currentMSecsSinceEpoch();
				CopyData(USHORT_TYPE, (uchar*)pBuf, (uchar*)HamamatsuImageBuffers[SaveImage_Index].image_data, image_width, image_height);
				++SaveImage_Index;
				if (SaveImage_Index == HamamatsuSaveImageNum){
					SaveImage_Index = 0;
					HamamatsuStartSaveImage = false;
					emit FinishSaveImageSignal((int)HAMAMATSU_WINDOW);
				}
			}
			//emit signal to hamamastu display window
			if (ImageCount%HAMAMATSU_DISPLAY_INTERVAL == 0){
				//cout << "AcquireImage: pic "<<Image_Count<<endl;
				//cout<<"image width: "<<image_width<<", image height: "<<image_height<<", rowBytes: "<<rowBytes<<endl;
				hamamatsuWindowInfo.image_width = image_width;
				hamamatsuWindowInfo.image_height = image_height;
				hamamatsuWindowInfo.image_stride = rowBytes;
				hamamatsuWindowInfo.image_num = ImageCount;

				CopyData(USHORT_TYPE, (uchar*)pBuf, circularBuffers[Buffer_Index], image_width, image_height); // Time comsumption is 2ms
				hamamatsuWindowInfo.image_data = circularBuffers[Buffer_Index];
				Buffer_Index = (Buffer_Index+1)%HAMAMATSU_BUFFER_SIZE;
				Camera->SendDisplayImageSignal(); 
			}
			++ImageCount;
			dcam_unlockdata(Camera->Get_Handle());
		}
	}

	qint64 end_time = QDateTime::currentMSecsSinceEpoch();
	//cout<<"AcquireImage time consumption:  " << (end_time - start_time) << "ms" <<endl;
}

ImageBuffer Hamamatsu_AcquireImageThread::Get_LatestImageBuffer()
{
	ImageBuffer buffer;
	buffer.image_data = NULL;
	buffer.image_width = 0;
	buffer.image_height = 0;
	buffer.data_type = USHORT_TYPE;

	if (circularBuffers[Buffer_Index] != NULL){
		buffer.image_width = image_width;
		buffer.image_height = image_height;
		buffer.image_data = circularBuffers[Buffer_Index];
		buffer.data_type = USHORT_TYPE;
	}
	return buffer;
}