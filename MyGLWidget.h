
#ifndef _MYGLWIDGET_H_
#define _MYGLWIDGET_H_

#include "Util.h"
#include <driver_types.h>
#include <QtWidgets/QOpenGLWidget>
#include <QtGui/QOpenGLFunctions_3_2_Core>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>

class MyGLWidget : public QOpenGLWidget
{
	Q_OBJECT
public:
	explicit MyGLWidget(WindowInfo imageInfo, QWidget* parent = 0);
	~MyGLWidget();

	void ShowImage(void* image_data, int width, int height, DATATYPE data_type);
	void Reset();

public slots:
	void SetFocusRegion(int, ImageRegion);
	void ShowFocusRegion(int, ImageRegion);
	void StopShowFocusRegion(int);

signals:
	void UpdatePositionStatus();
	void UpdateFocusRegionSignal(int, ImageRegion);

protected:
	void paintRect(QPoint& startPoint, QPoint& endPoint);
	void updateCurrentPosition();
	void initializeGL() Q_DECL_OVERRIDE;
	void paintGL() Q_DECL_OVERRIDE;
	void resizeGL(int width, int height) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
	void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
	void keyPressEvent(QKeyEvent * event) Q_DECL_OVERRIDE;

private:
	void makeObject();
	void clearObject();
	void TextureMapping(DisplayWindowOrientation orientation);

	//display window property
	int imageWidth, imageHeight;
	DATATYPE imageDataType;
	int windowWidth, windowHeight;
	DisplayWindowFlag windowFlag;
	DisplayWindowOrientation windowOrientation;

	bool readyDisplay;
	bool updateScaling, bShowRect, bShowCurrentPosition, bShowFocusRegion;
	double scaleFactor;
	double xTranslation, yTranslation;
	QPoint startMousePoint, endMousePoint, currentMousePoint;
	bool bSetFocusRegion;
	ImageRegion startImageRegion, currentImageRegion;

	int startDisplayImageRow, startDisplayImageCol;
	int displayImageWidth, displayImageHeight;
	int lastDisplayImageWidth, lastDisplayImageHeight;
	int lastDisplayImageCol, lastDisplayImageRow;
	double xCenter, yCenter;
		
	QOpenGLFunctions_3_2_Core* openglF;
	GLuint texture;
	GLuint pixBufferObj;
	struct cudaGraphicsResource* cuda_pbo_resource;
};

#endif //_MYGLWIDGET_H_