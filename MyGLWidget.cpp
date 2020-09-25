
#include "DevicePackage.h"
#include "MyGLWidget.h"
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <device_launch_parameters.h>
#include <QtGui/QOpenGLContext>
#include <QtWidgets/QMessageBox>
#include <QtGui/QPainter>

extern "C" int pixelConvert8(uchar3* output_data, uchar* original_data, int width, int height);
extern "C" int pixelConvert16(uchar3* dev_output_data, ushort* dev_original_data, int width, int height, int rightShiftBits);

MyGLWidget::MyGLWidget(WindowInfo windowInfo, QWidget* parent) :QOpenGLWidget(parent)
{
	readyDisplay = false;
	updateScaling = false;
	bShowCurrentPosition = false;
	bShowRect = false;
	bShowFocusRegion = false;
	bSetFocusRegion = false;

	scaleFactor = 1.0f;
	xTranslation = 0.0f;
	yTranslation = 0.0f;
	xCenter = 0.0;
	yCenter = 0.0;

	startDisplayImageRow = 0;
	startDisplayImageCol = 0;
	lastDisplayImageCol = 0;
	lastDisplayImageRow = 0;
	displayImageWidth = windowInfo.image_width;
	displayImageHeight = windowInfo.image_height;
	lastDisplayImageWidth = windowInfo.image_width;
	lastDisplayImageHeight = windowInfo.image_height;

	imageWidth = windowInfo.image_width;
	imageHeight = windowInfo.image_height;
	imageDataType = windowInfo.data_type;
	windowFlag = windowInfo.windowFlag;
	windowOrientation = windowInfo.windowOrientation;

	//get width and height of current window
	windowWidth = this->width();
	windowHeight = this->height();
}

MyGLWidget::~MyGLWidget()
{
	makeCurrent();
	clearObject();
	doneCurrent();
}

void MyGLWidget::makeObject()
{
	//create texture
	openglF->glGenTextures(1, &texture);
	openglF->glBindTexture(GL_TEXTURE_2D, texture);
	openglF->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	openglF->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	openglF->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//GL_CLAMP_TO_EDGE
	openglF->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);//GL_CLAMP_TO_EDGE
	openglF->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	//create pixel buffer object
	size_t numBytes = 3*imageWidth*imageHeight*sizeof(uchar);
	openglF->glGenBuffers(1, &pixBufferObj);
	openglF->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixBufferObj);
	openglF->glBufferData(GL_PIXEL_UNPACK_BUFFER, numBytes, NULL, GL_DYNAMIC_DRAW);
	//openglF->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void MyGLWidget::clearObject()
{
	openglF->glDeleteTextures(1, &texture);
	openglF->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	openglF->glDeleteBuffers(1, &pixBufferObj);
	cudaDeviceReset();
}

void MyGLWidget::initializeGL()
{
	openglF = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_2_Core>();
	openglF->initializeOpenGLFunctions();

	openglF->glViewport(0, 0, windowWidth, windowHeight);
	openglF->glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	openglF->glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	makeObject();
}

void MyGLWidget::resizeGL(int width, int height)
{
	 if (displayImageWidth*height > width*displayImageHeight){
        windowWidth = width;
		windowHeight = int(1.0*windowWidth*displayImageHeight/displayImageWidth);
	}
	else{
		windowWidth = int(1.0*windowHeight*displayImageWidth/displayImageHeight);
		windowHeight = height;
	}
	openglF->glViewport(0, 0, windowWidth, windowHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, windowWidth, windowHeight, 0.0f, 1.0, 1.0f);
}

void MyGLWidget::paintGL()
{
	if (readyDisplay){
		openglF->glEnable(GL_TEXTURE_2D);
		openglF->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		openglF->glBindBuffer(GL_PIXEL_PACK_BUFFER, pixBufferObj);
		openglF->glBindTexture(GL_TEXTURE_2D, texture);
		openglF->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

		if (windowFlag == HAMAMATSU_WINDOW){
			windowOrientation = hamamatsuWindowInfo.windowOrientation;
			TextureMapping(windowOrientation);
		}
		openglF->glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		openglF->glDisable(GL_TEXTURE_2D);

		if (bShowCurrentPosition){
			int length = 16;
			int col = endMousePoint.x();
			int row = endMousePoint.y();

			float p1[2], p2[2], p3[2], p4[2];
			p1[0] = 2.0f*col/(windowWidth-1) - 1.0f;
			p1[1] = -2.0f*(row-length)/(windowHeight-1) + 1.0f;
			p2[0] = 2.0f*col/(windowWidth-1) - 1.0f;
			p2[1] = -2.0f*(row+length)/(windowHeight-1) + 1.0f;
			p3[0] = 2.0f*(col-length)/(windowWidth-1) - 1.0f;
			p3[1] = -2.0f*row/(windowHeight-1) + 1.0f;
			p4[0] = 2.0f*(col+length)/(windowWidth-1) - 1.0f;
			p4[1] = -2.0f*row/(windowHeight-1) + 1.0f;

			glLoadIdentity();
			glBegin(GL_LINES);
			glLineWidth(1.0f);
			glVertex2f(p1[0], p1[1]);
			glVertex2f(p2[0], p2[1]);
			glVertex2f(p3[0], p3[1]);
			glVertex2f(p4[0], p4[1]);
			glEnd();
		}

		if (!updateScaling && bShowFocusRegion){
			QPoint startPoint, endPoint;
			double x_ratio = 1.0f*windowWidth/imageWidth,  y_ratio = 1.0f*windowHeight/imageHeight;
			startPoint.setX( int(currentImageRegion.x_offset*x_ratio) );
			startPoint.setY( int(currentImageRegion.y_offset*y_ratio) );
			endPoint.setX( int((currentImageRegion.x_offset + currentImageRegion.width)*x_ratio) );
			endPoint.setY( int((currentImageRegion.y_offset + currentImageRegion.height)*y_ratio) );
			paintRect(startPoint, endPoint);
		}
		
		if (bShowRect){
			paintRect(startMousePoint, currentMousePoint);
		}

		if (updateScaling){
			glLoadIdentity();
			glScalef(scaleFactor, scaleFactor, 1.0f);
			glTranslatef(xTranslation, yTranslation, 0.0f);
			bShowCurrentPosition = true;
			if (abs(scaleFactor - 1.0f)<1.0e-6){
				updateScaling = false;
			}
		}
	}
} 

void MyGLWidget::paintRect(QPoint& startPoint, QPoint& endPoint)
{
	int start_x = startPoint.x(), start_y = startPoint.y();
	int end_x = endPoint.x(), end_y = endPoint.y();

	float p1[2], p2[2], p3[2], p4[2];
	p1[0] = 2.0f*start_x/(windowWidth-1) - 1.0f;
	p1[1] = -2.0f*start_y/(windowHeight-1) + 1.0f;
	p2[0] = 2.0f*end_x/(windowWidth-1) - 1.0f;
	p2[1] = p1[1];
	p3[0] = p2[0];
	p3[1] = -2.0f*end_y/(windowHeight-1) + 1.0f;
	p4[0] = p1[0];
	p4[1] = p3[1];

	glBegin(GL_LINE_LOOP);
	glLineWidth(1.0f);
	glVertex2f(p1[0], p1[1]);
	glVertex2f(p2[0], p2[1]);
	glVertex2f(p3[0], p3[1]);
	glVertex2f(p4[0], p4[1]);
	glEnd();
}

void MyGLWidget::TextureMapping(DisplayWindowOrientation orientation)
{
	switch(orientation)
	{
		case NORMAL:
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, -1.0f);
			glEnd();
			break;
		case FLIP_UP_DOWN:
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
			glEnd();
			break;
		case FLIP_LEFT_RIGHT:
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(1.0f, 1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
			glEnd();
			break;
		case  FLIP_BOTH:
		case ROT_180:
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(1.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(1.0f, -1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
			glEnd();
			break;
		case ROT_90:
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
			glEnd();
			break;
		case ROT_270:
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(1.0f, 1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
			glEnd();
			break;
	}
}

void MyGLWidget::ShowImage(void* image_data, int width, int height, DATATYPE data_type)
{
	if (image_data == NULL){ return; }

	readyDisplay = true;
	imageWidth = width;
	imageHeight = height;
	size_t numBytes = 3*imageWidth*imageHeight, original_data_size = 0;
	uchar3* output_data = NULL;
	DataRightShift dataRightShift;

	if (!updateScaling){
		lastDisplayImageWidth = imageWidth;
		lastDisplayImageHeight = imageHeight;

		displayImageWidth = imageWidth;
		displayImageHeight = imageHeight;
		startDisplayImageRow = 0;
		startDisplayImageCol = 0;
	}

	//Get start point position and value
	if (bShowCurrentPosition){
		positionStatus.windowFlag = windowFlag;
		int currentCol = startDisplayImageCol + int(1.0 * endMousePoint.x() * displayImageWidth/windowWidth);
		int currentRow = startDisplayImageRow + int(1.0 * endMousePoint.y() * displayImageHeight/windowHeight);
		positionStatus.currentCol = currentCol;
		positionStatus.currentRow = currentRow;

		if (windowFlag == HAMAMATSU_WINDOW || windowFlag == ANDOR_WINDOW){
			ushort* data = (ushort*)image_data;
			positionStatus.value = data[currentRow*width + currentCol];
			emit UpdatePositionStatus(); //update current position and value in status bar
		}
		else if (windowFlag == IO_WINDOW){
			uchar* data = (uchar*)image_data;
			positionStatus.value = data[currentRow*width + currentCol];
			emit UpdatePositionStatus(); //update current position and value in status bar
		}
	}

	if (windowFlag == HAMAMATSU_WINDOW){
		dataRightShift = hamamatsuWindowInfo.dataRightShift;
	}
	
	//Allocate the pixel buffer object
	openglF->glBufferData(GL_PIXEL_UNPACK_BUFFER, numBytes, NULL, GL_DYNAMIC_DRAW);

	//register the PBO and map the PBO to the CUDA
	cudaError error = cudaGraphicsGLRegisterBuffer(&cuda_pbo_resource, pixBufferObj, cudaGraphicsMapFlagsWriteDiscard);//register the PBO to the CUDA
	if (error != cudaSuccess){
		//QMessageBox::critical(NULL, "Warning", "Fail to register PBO to the CUDA");
		//exit(-1);
		cout<<"Fail to register PBO to the CUDA"<<endl;
		return;
	}
	error = cudaGraphicsMapResources(1, &cuda_pbo_resource, 0);
	if (error != cudaSuccess){
		//QMessageBox::critical(NULL, "Warning", "Fail to map resources");
		//exit(-1);
		cout<<"Fail to map resources"<<endl;
		return;
	}
	error = cudaGraphicsResourceGetMappedPointer((void* *)(&output_data), &numBytes, cuda_pbo_resource);
	if (error != cudaSuccess){
		//QMessageBox::critical(NULL, "Warning", "Fail to get mapped pointer");
		//exit(-1);
		cout<<"Fail to get mapped pointer"<<endl;
		return;
	}

	if (data_type == UCHAR_TYPE){	
		uchar* original_data = reinterpret_cast<uchar*>(image_data);

		//allocate memory in the device and copy data from host to the device
		uchar* dev_original_data;
		original_data_size = sizeof(uchar)*imageWidth*imageHeight;
		error = cudaMalloc(&dev_original_data, original_data_size);
		if (error != cudaSuccess){
			//QMessageBox::critical(NULL, "Warning", "Fail to malloc memory in the device");
			//exit(-1);
			cout<<"Fail to malloc memory in the device"<<endl;
			return;
		}

		error = cudaMemcpy(dev_original_data,original_data, original_data_size, cudaMemcpyHostToDevice);
		if (error != cudaSuccess){
			//QMessageBox::critical(NULL, "Warning", "Fail to memcpy data from host to device");
			//exit(-1);
			cout<<"Fail to memcpy data from host to device"<<endl;
			return;
		}

		//launch kernel function to display image
		pixelConvert8(output_data, dev_original_data, width, height);
		error = cudaFree(dev_original_data);
		if (error != cudaSuccess){
			//QMessageBox::critical(NULL, "Warning", "Fail to free memory");
			//exit(-1);
			cout<<"Fail to free memory"<<endl;
			return;
		}
	}
	else if (data_type == USHORT_TYPE){
		ushort* original_data = reinterpret_cast<ushort*>(image_data);

		//allocate memory in the device and copy data from host to the device
		ushort* dev_original_data;
		original_data_size = sizeof(ushort)*imageWidth*imageHeight;

		error = cudaMalloc(&dev_original_data, original_data_size);
		if (error != cudaSuccess){
			//QMessageBox::critical(NULL, "Warning", "Fail to malloc memory in the device");
			//exit(-1);
			cout<<"Fail to malloc memory in the device"<<endl;
			return;
		}

		error = cudaMemcpy(dev_original_data, original_data, original_data_size, cudaMemcpyHostToDevice);
		if (error != cudaSuccess){
			//QMessageBox::critical(NULL, "Warning", "Fail to memcpy data from host to device");
			//exit(-1);
			cout<<"Fail to memcpy data from host to device"<<endl;
			return;
		}

		//launch kernel function to display image
		pixelConvert16(output_data, dev_original_data, width, height, (int)dataRightShift);
		error = cudaFree(dev_original_data);
		if (error != cudaSuccess){
			//QMessageBox::critical(NULL, "Warning", "Fail to free memory");
			//exit(-1);
			cout<<"Fail to free memory"<<endl;
			return;
		}
	}

	//delete the map and unregister the pbo
	error = cudaGraphicsUnmapResources(1, &cuda_pbo_resource, 0);
	if (error != cudaSuccess){
		//QMessageBox::critical(NULL, "Warning", "Fail to unmap resource");
		//exit(-1);
		cout<<"Fail to unmap resource"<<endl;
		return;
	}

	error = cudaGraphicsUnregisterResource(cuda_pbo_resource);
	if (error != cudaSuccess){
		//QMessageBox::critical(NULL, "Warning", "Fail to unregister PBO from CUDA");
		//exit(-1);
		cout<<"Fail to unregister PBO from CUDA"<<endl;
		return;
	}

	update();
}

void MyGLWidget::keyPressEvent(QKeyEvent * event){
	int currentCol = positionStatus.currentCol;
	int currentRow = positionStatus.currentRow;
	if (readyDisplay){
		switch(event->key()){
			case Qt::Key_Up: currentRow--;
			                 break;
		    case Qt::Key_Down: currentRow++;
		                       break;
		    case Qt::Key_Left: currentCol--;
		                       break;
		    case Qt::Key_Right: currentCol++;
		                        break;
		}
		// update current row and column
		positionStatus.currentCol = currentCol;
		positionStatus.currentRow = currentRow;
	}
	event->accept();
}

void MyGLWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton || !readyDisplay){ 
		event->ignore();
		return;
	}
	startMousePoint = event->pos(); //save the start mouse point
}

void MyGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	if ((event->buttons() & Qt::LeftButton) && readyDisplay){
		currentMousePoint = event->pos();

		if ( (windowFlag == HAMAMATSU_WINDOW && hamamatsuWindowInfo.isLive==1) )
		{
			bShowRect = true;
		}
	}
}

void MyGLWidget::mouseReleaseEvent(QMouseEvent * event)
{
	if (event->button() != Qt::LeftButton || !readyDisplay){
		event->ignore();
		return;
	}
	endMousePoint = event->pos();
	
	if (bShowRect){
		bShowRect = false;

		//Confirm the selected region
		int xMin = (startMousePoint.x() < endMousePoint.x() ? startMousePoint.x() : endMousePoint.x());
		int xMax = (startMousePoint.x() < endMousePoint.x() ? endMousePoint.x() : startMousePoint.x());
		int yMin = (startMousePoint.y() < endMousePoint.y() ? startMousePoint.y() : endMousePoint.y());
		int yMax = (startMousePoint.y() < endMousePoint.y() ? endMousePoint.y() : startMousePoint.y());
		
		if ((xMax-xMin)<10 || (yMax-yMin)<10){
			bShowCurrentPosition = true;
			return;
		}
		double x_scale = 1.0*windowWidth/(xMax - xMin);
		double y_scale = 1.0*windowHeight/(yMax - yMin);
		if (x_scale < y_scale){
			yMax = int(1.0*windowHeight/x_scale) + yMin;
			yMax = (yMax > windowHeight-1 ? windowHeight-1 : yMax);
		}
		else{
			xMax = int(1.0*windowWidth/y_scale) + xMin;
			xMax = (xMax > windowWidth-1 ? windowWidth-1 : xMax);
		}

		double x_center = 1.0*(xMax + xMin)/2, y_center = 1.0*(yMax + yMin)/2;
		if (abs(scaleFactor - 1.0f)<1.0e-6){
			xCenter = x_center;
			yCenter = y_center;
		}
		else{
			xCenter += (x_center-1.0*(windowWidth-1)/2)/scaleFactor;
			yCenter += (y_center-1.0*(windowHeight-1)/2)/scaleFactor;
		}

		lastDisplayImageCol = lastDisplayImageCol + int(1.0 * xMin * lastDisplayImageWidth/windowWidth);
		lastDisplayImageRow = lastDisplayImageRow + int(1.0 * yMin * lastDisplayImageHeight/windowHeight);

		displayImageWidth = int(displayImageWidth*(xMax - xMin)/windowWidth);
		displayImageHeight = int(displayImageHeight*(yMax-yMin)/windowHeight);
		startDisplayImageCol = lastDisplayImageCol;
		startDisplayImageRow = lastDisplayImageRow;

		lastDisplayImageWidth = displayImageWidth;
		lastDisplayImageHeight = displayImageHeight;
		lastDisplayImageCol = startDisplayImageCol; //update the start col and row of the current displaying image
		lastDisplayImageRow = startDisplayImageRow;

		xTranslation = - 2.0f*xCenter/(windowWidth-1) + 1.0f;
		yTranslation =  2.0f*yCenter/(windowHeight-1) - 1.0f;
		scaleFactor *= min(x_scale, y_scale);
		//cout<<"scale factor: "<<scaleFactor<<", display image width: "<<displayImageWidth<<", display image height: "<<displayImageHeight<<endl;

		endMousePoint.setX(windowWidth/2);
		endMousePoint.setY(windowHeight/2);
		bShowCurrentPosition = false;
		updateScaling = true;
	}
	else{
		bShowCurrentPosition = true;
	}

	//update the window
	if ( (windowFlag == HAMAMATSU_WINDOW && hamamatsuWindowInfo.isLive==0) )
	{
		updateCurrentPosition();
		update();
	}
}

void MyGLWidget::Reset()
{
	int x = positionStatus.currentCol*windowWidth/imageWidth, 
	    y = positionStatus.currentRow*windowHeight/imageHeight;
	bShowCurrentPosition = false;
	endMousePoint.setX(x);
	endMousePoint.setY(y);

	startDisplayImageRow = 0;
	startDisplayImageCol = 0;
	lastDisplayImageCol = 0;
	lastDisplayImageRow = 0;
	displayImageWidth = imageWidth;
	displayImageHeight = imageHeight;
	lastDisplayImageWidth = imageWidth;
	lastDisplayImageHeight = imageHeight;

	scaleFactor = 1.0f;
	xTranslation = 0.0f;
	yTranslation = 0.0f;
}

void MyGLWidget::updateCurrentPosition()
{
	int currentCol = startDisplayImageCol + int(1.0 * endMousePoint.x() * displayImageWidth/windowWidth);
	int currentRow = startDisplayImageRow + int(1.0 * endMousePoint.y() * displayImageHeight/windowHeight);
	
	bShowCurrentPosition = false;
	//Update current position and value in status bar
	if (CurrentWindowFlag == HAMAMATSU_WINDOW && windowFlag == HAMAMATSU_WINDOW && hamamatsuWindowInfo.image_data!=NULL){
		positionStatus.windowFlag = windowFlag;
		positionStatus.currentCol = currentCol;
		positionStatus.currentRow = currentRow;

		bShowCurrentPosition = true;
		ushort* data = (ushort*)hamamatsuWindowInfo.image_data;
		positionStatus.value = data[currentRow*imageWidth + currentCol];
		emit UpdatePositionStatus(); 
	}
}

void MyGLWidget::SetFocusRegion(int window_flag, ImageRegion region)
{
	if (window_flag == this->windowFlag){
		startImageRegion = region;
		currentImageRegion = startImageRegion;
		bSetFocusRegion = true;
		bShowFocusRegion = true;
	}
}

void MyGLWidget::ShowFocusRegion(int window_flag, ImageRegion region)
{
	if (window_flag == this->windowFlag){
		bShowFocusRegion = true;
		startImageRegion = region;
		currentImageRegion = startImageRegion;
	}
}

void MyGLWidget::StopShowFocusRegion(int window_flag)
{
	if (window_flag == this->windowFlag){
		bShowFocusRegion = false;
	}
}