#ifndef IMAGESAVETHREAD_H
#define IMAGESAVETHREAD_H

#include "Util.h"
#include <QThread>
#include <QMutex>
#include <QVector>

class ImageSaveThread : public QThread
{
	Q_OBJECT

public:
	ImageSaveThread(QVector<ImageBuffer *> buffers, QString imageFolder, QString prefix, QString imageFormat, QObject *parent);
	ImageSaveThread(ImageBuffer *imageBuffer, unsigned int count, QString imageFolder, QString prefix, QString imageFormat, QObject *parent);
	~ImageSaveThread();
	void stop();
	bool isStoped();
signals:
	void onImageSaved(ImageBuffer *pBuffer);
protected:
	void run();
private:
	bool m_stopFlag;
	bool m_isStopped;
	QMutex mutex;
	QVector<ImageBuffer *> buffers;
	QString prefix;
	QString imageFolder;
	QString imageFormat;
};

#endif // IMAGESAVETHREAD_H
