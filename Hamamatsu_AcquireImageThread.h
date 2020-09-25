
#ifndef _HAMAMASTU_ACQUIRE_IMAGE_H_
#define _HAMAMASTU_ACQUIRE_IMAGE_H_

#include "Hamamatsu_Camera.h"
#include <QtCore/QThread>

#ifndef _DEFINE_HAMAMATSU_BUFFER
#define _DEFINE_HAMAMATSU_BUFFER
#define HAMAMATSU_BUFFER_SIZE 16
#endif

class Hamamatsu_Camera;
class Hamamatsu_AcquireImageThread : public QThread
{
	Q_OBJECT
public:
	static string OBJECT_NAME;

	explicit Hamamatsu_AcquireImageThread(Hamamatsu_Camera* camera);
	~Hamamatsu_AcquireImageThread();
	Hamamatsu_Camera* Camera;

	void StopThread();
	void StartThread();
	void ClearImageCount(){
		ImageCount = 0;
	}
	ImageBuffer Get_LatestImageBuffer();

signals:
	void FinishSaveImageSignal(int);

protected:
	void CreateBuffers();//create cicular buffers with 8 alignment
	void ClearBuffers();
	void AcquireImage();
	virtual void run();

private:
	bool hasAllocatedFrames;
	volatile bool isStopAcquireImage;
	int SaveImage_Index;
	int Buffer_Index;
	int image_width;
	int image_height;
	uchar* acqBuffers[HAMAMATSU_BUFFER_SIZE];
	uchar* circularBuffers[HAMAMATSU_BUFFER_SIZE];
	unsigned long ImageCount;
};

#endif //_HAMAMATSU_ACQUIRE_IMAGE_H_