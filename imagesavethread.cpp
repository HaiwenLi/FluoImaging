#include "imagesavethread.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

ImageSaveThread::ImageSaveThread(QVector<ImageBuffer *> buffers, QString imageFolder, QString prefix, QString imageFormat, QObject *parent)
	: buffers(buffers), imageFolder(imageFolder), prefix(prefix), imageFormat(imageFormat), QThread(parent)
{
	m_stopFlag = false;
	m_isStopped = true;
}

ImageSaveThread::ImageSaveThread(ImageBuffer *imageBuffer, unsigned int count, QString imageFolder, QString prefix, QString imageFormat, QObject *parent)
	: imageFolder(imageFolder), prefix(prefix), imageFormat(imageFormat), QThread(parent)
{
	m_stopFlag = false;
	this->buffers.resize(count);
	for (unsigned int i = 0; i < count; i++) {
		this->buffers[i] = imageBuffer + i;
	}
}

ImageSaveThread::~ImageSaveThread()
{
}

void ImageSaveThread::stop() {
	//QMutexLocker locker(&mutex);
	m_stopFlag = true;
}

bool ImageSaveThread::isStoped() {
	return m_isStopped;
}

void ImageSaveThread::run() {
	m_isStopped = false;
	for (QVector<ImageBuffer *>::iterator iterBuffer = buffers.begin(); iterBuffer != buffers.end(); iterBuffer++) {
		//{
		//	QMutexLocker locker(&mutex);
		//	if (m_stopFlag) break;
		//}
		if (m_stopFlag) break;

		ImageBuffer *p_buffer = *iterBuffer;

		QString filename = imageFolder+"\\"+prefix+"_"+QString::number(p_buffer->timestamp)+"."+imageFormat;

		if (p_buffer->data_type == USHORT_TYPE){
			ushort* image_data = (ushort*)(p_buffer->image_data);
			cv::Mat image(p_buffer->image_height, p_buffer->image_width, CV_16UC1, image_data);
			imwrite(filename.toStdString(), image);
		}
		else if (p_buffer->data_type == UCHAR_TYPE){
			uchar* image_data = (uchar*)(p_buffer->image_data);
			cv::Mat image(p_buffer->image_height, p_buffer->image_width, CV_8UC1, image_data);
			imwrite(filename.toStdString(), image);
		}

		delete p_buffer->image_data;
		p_buffer->image_data = NULL;

		emit onImageSaved(p_buffer);
	}
	m_isStopped = true;
}
