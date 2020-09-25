#ifndef _TRACKINGWINDOW_H_
#define _TRACKINGWINDOW_H_

#include "DevicePackage.h"
#include "ControlPanel.h"
#include "MyGLWidget.h"
#include <QtCore/QObject>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <QtGui/QIcon>
#include <QtCore/QTimer>

struct ToolBarContent{
	bool connected;
	bool isLive;
};

struct StatusBarContent{
	bool ready;
	int imageWidth;
	int imageHeight;
	int currentRow;
	int currentCol;
	int value;
	double temperature;
};

class StopDisplayThread: public QThread
{
	Q_OBJECT
public:
	StopDisplayThread(int);

signals:
	void HasStopDisplaySignal(int);

protected:
	void run() Q_DECL_OVERRIDE;

private:
	int windowFlag;
};

class TrackingWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit TrackingWindow(QWidget* parent = 0);
	~TrackingWindow();
	void DockFocusPanel();
	
public slots:
	void DisplayImageSlot(int);
	void StopDisplayImageSlot(int);
	void HasStopDisplaySlot();
	void ShowCurrentPositionAndValue();

protected:
	virtual void closeEvent(QCloseEvent*  event);
	virtual void resizeEvent(QResizeEvent * event);
	void CreateLayout(); 
	void CreateMenus();
	void CreateToolBars();
	void CreateStatusBars();

	void SetStatus(int index);
	void GetImageWindowMargins(int window_width, int window_height, int image_width, int image_height, int &x_offset, int &y_offset);

	void ClearHamamatsuCamera();
	void ConnectHamamatsuCamera(bool state);

protected slots:
	void UpdateHamamatsuTemperature();
	void OnFileSaveAction();
	void OnTabChanged(int index);
	void OnConnectAction();
	void OnCaptureAction();
	void OnLiveAction();
	void OnStopLiveAction();
	void OnResetAction();
	void OnObjectiveLensX10Action();
	void OnObjectiveLensX20Action();
	void OnObjectiveLensX40Action();

private:
	//Menus
	QMenu* fileMenu;
	QAction* openAction;
	QAction* saveAction;
	QAction* saveAsAction;
	QAction* exitAction;	
	QMenu* viewMenu;
	QMenu* toolMenu;
	QMenu* helpMenu;
	QAction* aboutAction;

	//ToolBar
	QAction* connectCameraAction;
	QAction* captureAction;
	QAction* liveAction;
	QAction* stopLiveAction;
	QAction* resetAction;
	QAction* cameraSeparator;
	QAction* objectiveLens40;
	QAction* objectiveLens20;
	QAction* objectiveLens10;

	QIcon connectIcon;
	QIcon disconnectIcon;
	QIcon liveIcon;
	QIcon stopLiveIcon;
	QIcon resetIcon;
	QIcon captureIcon;
	QIcon x40_onIcon;
	QIcon x40_offIcon;
	QIcon x20_onIcon;
	QIcon x20_offIcon;
	QIcon x10_onIcon;
	QIcon x10_offIcon;

	//Display region
	QTabWidget* displayTabs;
	QFrame* Hamamatsu_GCaMPFrame;
	QFrame* Hamamatsu_RFPFrame;
	MyGLWidget* Hamamatsu_GCaMPWindow;
	MyGLWidget* Hamamatsu_RFPWindow;
	//DisplayWindowFlag currentWindow; //current displaying content

	//StatusBar
	QLabel* stateLabel;
	QLabel* imageSizeLabel;
	QLabel* currentPositionLabel;
	QLabel* currentValueLabel;
	StatusBarContent statusBarContents;
	ToolBarContent toolBarContents;
	bool objectiveLensContents[3];//x10, x20, x40

	//temperature timer
	int timerInterval;
	QTimer tempTimer;

	//Control Panel
	ControlPanel* controlPanel;
	StopDisplayThread* hamamatsuStopDisplayThread;
};

#endif // _TRACKINGWINDOW_H_
