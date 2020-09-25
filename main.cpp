
#include "FluoImaging.h"
#include "Camera_Params.h"
#include <QtWidgets/QApplication>
#include <QtCore/QTime>

//相机参数设置
Hamamatsu_Camera* hamamatsuCamera= NULL;
int HAMAMATSU_DISPLAY_INTERVAL = 8;
ImageBuffer* HamamatsuImageBuffers = NULL;
int HamamatsuSaveImageNum = 0;
bool HamamatsuStartSaveImage = false;

WindowInfo hamamatsuWindowInfo = {0, HAMAMATSU_PARAMS::FULLIMAGE_WIDTH, HAMAMATSU_PARAMS::FULLIMAGE_HEIGHT, 
	                                                          HAMAMATSU_PARAMS::FULLIMAGE_WIDTH, 0,NULL, USHORT_TYPE, 0,SINGLE,Mono16, BIT_0, HAMAMATSU_WINDOW, NORMAL};
//current position and value for status bar
PositionStatus positionStatus = {HAMAMATSU_WINDOW, 0, 0, 0};
DisplayWindowFlag CurrentWindowFlag = HAMAMATSU_WINDOW;

int main(int argc, char* argv[])
{
	QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
	QApplication a(argc, argv);
	qRegisterMetaType<string>("string");
	qRegisterMetaType<ImageRegion>("ImageRegion");

	TrackingWindow w;
	//QIcon windowIcon(".\\Resources\\Icons\\window.png");
	//w.setWindowIcon(windowIcon);
	w.show();
	w.DockFocusPanel();
	return a.exec();
}