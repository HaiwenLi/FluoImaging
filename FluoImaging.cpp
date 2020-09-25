
#include "FluoImaging.h"
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtGui/QFont>
#include <QtGui/QKeySequence>

TrackingWindow::TrackingWindow(QWidget* parent) : QMainWindow(parent)
{
	hamamatsuCamera = NULL;
	statusBarContents.ready = false;

	timerInterval = 10000;//10s
	tempTimer.setSingleShot(true);//for hamamastu camera

	hamamatsuStopDisplayThread = new StopDisplayThread(HAMAMATSU_WINDOW);
	connect(hamamatsuStopDisplayThread, SIGNAL(HasStopDisplaySignal(int)), this, SLOT(HasStopDisplaySlot()));
	
	CreateLayout();
	controlPanel->show();
	this->setWindowTitle("Calcium Imaging");
}

TrackingWindow::~TrackingWindow()
{
	if (hamamatsuStopDisplayThread != NULL){
		delete hamamatsuStopDisplayThread;
		hamamatsuStopDisplayThread = NULL;
	}
	if (controlPanel != NULL){
		delete controlPanel;
		controlPanel = NULL;
	}
	ClearHamamatsuCamera();
}

void TrackingWindow::closeEvent(QCloseEvent*  event)
{
	tempTimer.stop();
	if (controlPanel != NULL){
		delete controlPanel;
		controlPanel = NULL;
	}
}

void TrackingWindow::resizeEvent(QResizeEvent * event){
	if (displayTabs != NULL && Hamamatsu_GCaMPFrame != NULL && Hamamatsu_RFPFrame != NULL){
		//adjust the windows sizes
		int displayWindowWidth = (displayTabs->geometry()).width();
		int displayWindowHeight = (displayTabs->geometry()).height();
		int x_offset, y_offset;

		//Hamamatsu GCaMP Window
		GetImageWindowMargins(displayWindowWidth, displayWindowHeight, hamamatsuWindowInfo.image_width,
				                                 hamamatsuWindowInfo.image_height, x_offset, y_offset);
		//cout<<"window width: "<<displayWindowWidth<<", window height: "<<displayWindowHeight<<endl;
		//cout<<"Hamamatsu x offset: "<<x_offset<<", y offset: "<<y_offset<<endl;
		Hamamatsu_GCaMPFrame->resize(displayWindowWidth, displayWindowHeight);
		Hamamatsu_GCaMPFrame->setContentsMargins(x_offset, y_offset, x_offset, y_offset);
		Hamamatsu_GCaMPWindow->resize(displayWindowWidth-2*x_offset, displayWindowHeight-2*y_offset);

		// Hamamatsu RFP Window
		GetImageWindowMargins(displayWindowWidth, displayWindowHeight, hamamatsuWindowInfo.image_width,
				                                 hamamatsuWindowInfo.image_height, x_offset, y_offset);
		//cout<<"window width: "<<displayWindowWidth<<", window height: "<<displayWindowHeight<<endl;
		//cout<<"Hamamatsu x offset: "<<x_offset<<", y offset: "<<y_offset<<endl;
		Hamamatsu_RFPFrame->resize(displayWindowWidth, displayWindowHeight);
		Hamamatsu_RFPFrame->setContentsMargins(x_offset, y_offset, x_offset, y_offset);
		Hamamatsu_RFPWindow->resize(displayWindowWidth-2*x_offset, displayWindowHeight-2*y_offset);
	}
}

void TrackingWindow::CreateLayout()
{
	CreateMenus();
	CreateToolBars();
	CreateStatusBars();
	
	QString frameStyle = tr("background:#808080;");
	controlPanel = new ControlPanel(NULL);
	displayTabs = new QTabWidget(this);
	Hamamatsu_GCaMPFrame = new QFrame;
	Hamamatsu_GCaMPFrame->setStyleSheet(frameStyle);
	Hamamatsu_RFPFrame = new QFrame;
	Hamamatsu_RFPFrame->setStyleSheet(frameStyle);

	Hamamatsu_GCaMPWindow = new MyGLWidget(hamamatsuWindowInfo);
	Hamamatsu_RFPWindow = new MyGLWidget(hamamatsuWindowInfo);

	//add Hamamatsu_Window, Andor_Window and IO_Window into related frames, respectively.
	QHBoxLayout* hamamatsuGCaMPLayout = new QHBoxLayout;
	hamamatsuGCaMPLayout->addWidget(Hamamatsu_GCaMPWindow);
	hamamatsuGCaMPLayout->setMargin(0);
	hamamatsuGCaMPLayout->setSpacing(0);
	Hamamatsu_GCaMPFrame->setLayout(hamamatsuGCaMPLayout);

	QHBoxLayout* hamamatsuRFPLayout = new QHBoxLayout;
	hamamatsuRFPLayout->addWidget(Hamamatsu_RFPWindow);
	hamamatsuRFPLayout->setMargin(0);
	hamamatsuRFPLayout->setSpacing(0);
	Hamamatsu_RFPFrame->setLayout(hamamatsuRFPLayout);

	//connect the signals to the relative slots
	connect(saveAction, SIGNAL(triggered()), this, SLOT(OnFileSaveAction()));
	connect(saveAsAction, SIGNAL(triggered()), this, SLOT(OnFileSaveAction()));
	connect( displayTabs, SIGNAL(tabBarClicked(int)), this, SLOT(OnTabChanged(int)) );
	connect( connectCameraAction, SIGNAL(triggered()), this, SLOT(OnConnectAction()) );
	connect( captureAction, SIGNAL(triggered()), this, SLOT(OnCaptureAction()) );
	connect( liveAction, SIGNAL(triggered()), this, SLOT(OnLiveAction()) );
	connect( stopLiveAction, SIGNAL(triggered()), this, SLOT(OnStopLiveAction()) );
	connect(resetAction, SIGNAL(triggered()), this, SLOT(OnResetAction()));
	connect(objectiveLens10, SIGNAL(triggered()), this, SLOT(OnObjectiveLensX10Action()));
	connect(objectiveLens20, SIGNAL(triggered()), this, SLOT(OnObjectiveLensX20Action()));
	connect(objectiveLens40, SIGNAL(triggered()), this, SLOT(OnObjectiveLensX40Action()));

	connect( controlPanel, SIGNAL(Hamamatsu_UpdateDisplayWindow()), Hamamatsu_GCaMPWindow, SLOT(update()) );
	connect( controlPanel, SIGNAL(Hamamatsu_UpdateDisplayWindow()), Hamamatsu_RFPWindow, SLOT(update()) );
	connect( controlPanel, SIGNAL(StopDisplayImagesSignal(int)), this, SLOT(StopDisplayImageSlot(int)) );

	connect(Hamamatsu_GCaMPWindow, SIGNAL(UpdatePositionStatus()), this, SLOT(ShowCurrentPositionAndValue()));
	connect(Hamamatsu_RFPWindow, SIGNAL(UpdatePositionStatus()), this, SLOT(ShowCurrentPositionAndValue()));	
	
	setCentralWidget(displayTabs);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	this->show();
	this->resize(800,800);
	displayTabs->addTab(Hamamatsu_GCaMPFrame, tr("Channel 1"));
	displayTabs->addTab(Hamamatsu_RFPFrame, tr("Channel 2"));

	//adjust the windows sizes
	int displayWindowWidth = (displayTabs->geometry()).width();
	int displayWindowHeight = (displayTabs->geometry()).height();
	int x_offset, y_offset;

	//Hamamatsu GCaMP Window
	GetImageWindowMargins(displayWindowWidth, displayWindowHeight, hamamatsuWindowInfo.image_width,
			                                 hamamatsuWindowInfo.image_height, x_offset, y_offset);
	//cout<<"window width: "<<displayWindowWidth<<", window height: "<<displayWindowHeight<<endl;
	//cout<<"Hamamatsu x offset: "<<x_offset<<", y offset: "<<y_offset<<endl;
	Hamamatsu_GCaMPFrame->resize(displayWindowWidth, displayWindowHeight);
	Hamamatsu_GCaMPFrame->setContentsMargins(x_offset, y_offset, x_offset, y_offset);
	Hamamatsu_GCaMPWindow->resize(displayWindowWidth-2*x_offset, displayWindowHeight-2*y_offset);

	// Hamamatsu RFP Window
	GetImageWindowMargins(displayWindowWidth, displayWindowHeight, hamamatsuWindowInfo.image_width,
			                                 hamamatsuWindowInfo.image_height, x_offset, y_offset);
	//cout<<"window width: "<<displayWindowWidth<<", window height: "<<displayWindowHeight<<endl;
	//cout<<"Hamamatsu x offset: "<<x_offset<<", y offset: "<<y_offset<<endl;
	Hamamatsu_RFPFrame->resize(displayWindowWidth, displayWindowHeight);
	Hamamatsu_RFPFrame->setContentsMargins(x_offset, y_offset, x_offset, y_offset);
	Hamamatsu_RFPWindow->resize(displayWindowWidth-2*x_offset, displayWindowHeight-2*y_offset);
}

void TrackingWindow::DockFocusPanel()
{
	QDesktopWidget desktop;
	int desktop_height = desktop.height();
	int start_x = this->x();
	int window_width = this->width();
	int window_height = this->height();
	int panel_width = controlPanel->width();
	int start_y = (desktop_height - window_height)/2 - 50;
	this->move(start_x-panel_width+100, start_y);
	start_x = start_x - panel_width + window_width+120;
	controlPanel->move(start_x, start_y);
}

void TrackingWindow::CreateMenus()
{
	fileMenu = menuBar()->addMenu(tr("&File"));
	openAction = fileMenu->addAction(tr("&Open..."));

	saveAction = fileMenu->addAction(tr("&Save"));
	saveAction->setShortcut(QKeySequence(tr("Ctrl+S")));

	saveAsAction = fileMenu->addAction(tr("Save &As"));
	saveAsAction->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
	fileMenu->addSeparator();

	exitAction = fileMenu->addAction(tr("E&xit"));
	exitAction->setShortcut(QKeySequence(tr("Ctrl+Q")));
	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

	viewMenu = menuBar()->addMenu(tr("V&iew"));
	toolMenu = menuBar()->addMenu(tr("&Tool"));

	helpMenu = menuBar()->addMenu(tr("&Help"));
	aboutAction = helpMenu->addAction(tr("About"));
}  

void TrackingWindow::CreateToolBars()
{
	QToolBar* toolBar = new QToolBar(this);
	connectIcon = QIcon(".\\Resources\\Icons\\connect.png");
	disconnectIcon = QIcon(".\\Resources\\Icons\\disconnect.png");
	captureIcon = QIcon(".\\Resources\\Icons\\capture.png");
	liveIcon = QIcon(".\\Resources\\Icons\\live.png");
	stopLiveIcon = QIcon(".\\Resources\\Icons\\stopLive.png");
	resetIcon = QIcon(".\\Resources\\Icons\\reset.png");
	x40_onIcon = QIcon(".\\Resources\\Icons\\x40_on.png");
	x40_offIcon = QIcon(".\\Resources\\Icons\\x40_off.png");
	x20_onIcon = QIcon(".\\Resources\\Icons\\x20_on.png");
	x20_offIcon = QIcon(".\\Resources\\Icons\\x20_off.png");
	x10_onIcon = QIcon(".\\Resources\\Icons\\x10_on.png");
	x10_offIcon = QIcon(".\\Resources\\Icons\\x10_off.png");

	toolBarContents.connected = false;
	toolBarContents.isLive = false;
	statusBarContents.currentCol = 0;
	statusBarContents.currentRow = 0;
	statusBarContents.value = 0;	
	for (int i=0; i<3; ++i){
		objectiveLensContents[i] = false;
	}

	connectCameraAction = toolBar->addAction(connectIcon,"");
	connectCameraAction->setToolTip("Connect camera");
	connectCameraAction->setEnabled(true);

	captureAction = toolBar->addAction(captureIcon,"");
	captureAction->setToolTip("Grab one image");
	captureAction->setEnabled(false);

	liveAction = toolBar->addAction(liveIcon,"");
	liveAction->setToolTip("Grab images continuously");
	liveAction->setEnabled(false);

	stopLiveAction = toolBar->addAction(stopLiveIcon,"");
	stopLiveAction->setToolTip("Stop grab images");
	stopLiveAction->setEnabled(false);

	resetAction = toolBar->addAction(resetIcon,"");
	resetAction->setToolTip("Reset display window");
	resetAction->setEnabled(false);

	cameraSeparator = toolBar->addSeparator();
	objectiveLens10 = toolBar->addAction(x10_offIcon, "");
	objectiveLens20 = toolBar->addAction(x20_offIcon, "");
	objectiveLens40 = toolBar->addAction(x40_offIcon, "");

	addToolBar(toolBar);
}

void TrackingWindow::CreateStatusBars()
{
	stateLabel = new QLabel(tr("OFF"));
	stateLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	stateLabel->setLineWidth(2);
	stateLabel->setMidLineWidth(2);
	stateLabel->setStyleSheet("color: #FFFFFF; background-color: #0000FF;font-size:14;font-weight:bold;");
	stateLabel->setMinimumWidth(80);
	stateLabel->setMaximumWidth(90);
	stateLabel->setAlignment(Qt::AlignCenter);

	imageSizeLabel = new QLabel(tr("Image Size"));
	imageSizeLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	imageSizeLabel->setLineWidth(1);
	imageSizeLabel->setMidLineWidth(3);
	imageSizeLabel->setMinimumWidth(80);
	imageSizeLabel->setMaximumWidth(120);
	imageSizeLabel->setAlignment(Qt::AlignCenter);

	currentPositionLabel = new QLabel(tr("(0, 0)"));
	currentPositionLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	currentPositionLabel->setLineWidth(1);
	currentPositionLabel->setMidLineWidth(3);
	currentPositionLabel->setMinimumWidth(100);
	currentPositionLabel->setMaximumWidth(120);
	currentPositionLabel->setAlignment(Qt::AlignCenter);
	
	currentValueLabel = new QLabel(tr("Value"));
	currentValueLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	currentValueLabel->setLineWidth(1);
	currentValueLabel->setMidLineWidth(3);
	currentValueLabel->setMinimumWidth(100);
	currentValueLabel->setMaximumWidth(120);
	currentValueLabel->setAlignment(Qt::AlignCenter);

	statusBar()->addWidget(stateLabel);
	statusBar()->addWidget(imageSizeLabel);
	statusBar()->addWidget(currentPositionLabel);
	statusBar()->addWidget(currentValueLabel);
}

void TrackingWindow::ShowCurrentPositionAndValue()
{
	int index = (int)positionStatus.windowFlag;
	// For GCaMP and RFP Window
	statusBarContents.currentRow = positionStatus.currentRow;
	statusBarContents.currentCol = positionStatus.currentCol;
	statusBarContents.value = positionStatus.value;

	// Update position and value of selected pixel in statusbar
	QString positionText = tr("x: ") + QString::number(statusBarContents.currentCol) + tr(", y: ") + QString::number(statusBarContents.currentRow);
	currentPositionLabel->setText(positionText);
	if (statusBarContents.currentCol == 0){
		currentValueLabel->setText(tr("Data: Value"));
	}
	else{
		currentValueLabel->setText(tr("Data: ")+QString::number(statusBarContents.value));
	}
}

/*
	Set current toolbar and statusbar contents
*/
void TrackingWindow::SetStatus(int index)
{
	//set toolbar content
	if (toolBarContents.connected){
		connectCameraAction->setIcon(disconnectIcon);
		captureAction->setEnabled(true);
		if (toolBarContents.isLive){
			liveAction->setEnabled(false);
			stopLiveAction->setEnabled(true);
		}
		else{
			liveAction->setEnabled(true);
			stopLiveAction->setEnabled(false);
		}
		resetAction->setEnabled(true);
	}
	else{
		connectCameraAction->setIcon(connectIcon);
		captureAction->setEnabled(false);
		liveAction->setEnabled(false);
		stopLiveAction->setEnabled(false);
		resetAction->setEnabled(false);
	}

	//set statusbar content
	if (statusBarContents.ready){
		//if (index == HAMAMATSU_WINDOW){
		//	stateLabel->setText("Ready");
		//}
		//else{
		//	stateLabel->setText(QString::number(statusBarContents.temperature) + QString::fromLocal8Bit(" ℃"));
		//}
		stateLabel->setText("Ready");

		//stateLabel->setStyleSheet("color: #FFFFFF; background-color: #FF0000;font-size:14;font-weight:bold;");
		stateLabel->setStyleSheet("color: #FFFFFF; background-color: #0000FF;font-size:14;font-weight:bold;");
		QString imageSizeText = QString::number(statusBarContents.imageWidth) +tr("x")+ QString::number(statusBarContents.imageHeight);
		imageSizeLabel->setText(imageSizeText);
		
		//update position and value of selected pixel in statusbar
		QString positionText = tr("x: ") + QString::number(statusBarContents.currentCol) + tr(", y: ") + QString::number(statusBarContents.currentRow);
		currentPositionLabel->setText(positionText);
		if (statusBarContents.currentCol == 0){
			currentValueLabel->setText("Value");
		}
		else {
			currentValueLabel->setText(tr("Data: ")+QString::number(statusBarContents.value));
		}

		tempTimer.start(timerInterval); //start the temperature timer
	}
	else{
		stateLabel->setText(tr("OFF"));
		stateLabel->setStyleSheet("color: #FFFFFF; background-color: #0000FF;font-size:14;font-weight:bold;");
		imageSizeLabel->setText("ImageSize");
		currentPositionLabel->setText("x: 0, y: 0");
		currentValueLabel->setText("Value");
		
		tempTimer.stop(); //stop the temperature timer
	}
}

void TrackingWindow::OnTabChanged(int index)
{
	CurrentWindowFlag = (DisplayWindowFlag)index;
	SetStatus(CurrentWindowFlag);
}

void TrackingWindow::ClearHamamatsuCamera()
{
	if (hamamatsuCamera != NULL){
		hamamatsuCamera->Disconnect();
		delete hamamatsuCamera;
		hamamatsuCamera = NULL;
	}
	if (hamamatsuWindowInfo.image_data != NULL){
		hamamatsuWindowInfo.image_data = NULL;
	}
}

/*
	Connect or Disconnect Hamamatsu Camera
	state: true corresponds to connection and false corresponds to disconnection
*/
void TrackingWindow::ConnectHamamatsuCamera(bool state)
{
	if (state){
		try{
			if (hamamatsuCamera == NULL){
				hamamatsuCamera = new Hamamatsu_Camera();
			}
			connect( hamamatsuCamera, SIGNAL(DisplayImageSignal(int)), this, SLOT(DisplayImageSlot(int)), Qt::QueuedConnection);
			hamamatsuCamera->Connect();
			if ( !hamamatsuCamera->IsConnected() ){
				ClearHamamatsuCamera();
				QMessageBox::critical(this, "Initialization Error", "Cannot detect Hamamatsu camera. Please connect Hamamatsu camera and restart software.");
				return;
			}

			//Get camera parameters
			ImageSize imageSize;
			hamamatsuCamera->Get_ImageSize(imageSize);

			//Update toolbar and statusbar
			toolBarContents.connected = true;
			statusBarContents.imageWidth = imageSize.width;
			statusBarContents.imageHeight = imageSize.height;
			statusBarContents.ready = true;
			connectCameraAction->setIcon(disconnectIcon);
			connectCameraAction->setToolTip("Disconnect camera");

			//Enable Hamamatsu camera settings
			controlPanel->EnableHamamatsuGroup(true);

		} catch (QException e){
			ClearHamamatsuCamera();
			QMessageBox::critical(this, "Initialization Error", QString::fromStdString(e.getMessage()));
			return;
		}
	}
	else {
		disconnect( hamamatsuCamera, SIGNAL(DisplayImageSignal(int)), this, SLOT(DisplayImageSlot(int)));
		hamamatsuCamera->Disconnect();

		//Update toolbar and statusbar
		connectCameraAction->setIcon(connectIcon);
		toolBarContents.connected = false;
		statusBarContents.ready = false;

		//Disable Hamamatsu camera settings
		controlPanel->EnableHamamatsuGroup(false);
	}

	OnTabChanged(HAMAMATSU_WINDOW);
}

/*
	Connect or Disconnect cameras when clicking relative icon
	state: true corresponds to connection and false corresponds to disconnection
*/
void TrackingWindow::OnConnectAction()
{
	// Connect hamamatsu camera
	static bool hamamastu_work = false;
	if (hamamastu_work)
		hamamastu_work = false;
	else
		hamamastu_work = true;
	ConnectHamamatsuCamera( hamamastu_work );
	controlPanel->CameraConnected(0);// enable hamamatsu camera in control panel
}

void TrackingWindow::OnCaptureAction()
{
	try{
		hamamatsuCamera->Capture();
	}
	catch (QException e){
		QMessageBox::critical(NULL, "Warning", QString::fromStdString(e.getMessage()));
	}
}

void TrackingWindow::OnLiveAction()
{
	int index = displayTabs->currentIndex();
	try{
		hamamatsuCamera->Live();
		toolBarContents.isLive = true;
		hamamatsuWindowInfo.isLive = 1;
		connect( hamamatsuCamera->acquireImageThread, SIGNAL(FinishSaveImageSignal(int)), controlPanel, SLOT(FinishSaveImage(int)) );

		liveAction->setEnabled(false);
		stopLiveAction->setEnabled(true);
	}
	catch (QException e){
		QMessageBox::critical(NULL, "Warning", QString::fromStdString(e.getMessage()));
	}
}

void TrackingWindow::OnStopLiveAction()
{
	int index = displayTabs->currentIndex();
	try{
		disconnect( hamamatsuCamera->acquireImageThread, SIGNAL(FinishSaveImageSignal(int)), controlPanel, SLOT(FinishSaveImage(int)) );
		hamamatsuCamera->StopLive();
		toolBarContents.isLive = false;
		hamamatsuWindowInfo.isLive = 0;

		liveAction->setEnabled(true);
		stopLiveAction->setEnabled(false);
	}
	catch (QException e){
		QMessageBox::critical(NULL, "Warning", QString::fromStdString(e.getMessage()));
	}
}

void TrackingWindow::DisplayImageSlot(int windowFlag)
{
	char channelArray[IMAGING_CHANNEL_LEN];
	int channel_len = 0;
	int offset = hamamatsuWindowInfo.channelOffset;
	ConvertImagingChannelSeqToArray(hamamatsuWindowInfo.imagingChannelSeq, channelArray, channel_len);

	if (hamamatsuWindowInfo.imagingChannelSeq == SINGLE || channelArray[(hamamatsuWindowInfo.image_num+offset)%channel_len] == GCAMP_CHANNEL){
		// Show GCaMP image
		Hamamatsu_GCaMPWindow->ShowImage(hamamatsuWindowInfo.image_data, hamamatsuWindowInfo.image_width, hamamatsuWindowInfo.image_height,
				hamamatsuWindowInfo.data_type);
	} else if (channelArray[(hamamatsuWindowInfo.image_num+offset)%channel_len] == RFP_CHANNEL){
		// Show RFP image
		Hamamatsu_RFPWindow->ShowImage(hamamatsuWindowInfo.image_data, hamamatsuWindowInfo.image_width, hamamatsuWindowInfo.image_height,
				hamamatsuWindowInfo.data_type);
	}
}

void TrackingWindow::StopDisplayImageSlot(int window_flag)
{
	if (window_flag == (int)HAMAMATSU_WINDOW){
		disconnect( hamamatsuCamera->acquireImageThread, SIGNAL(FinishSaveImageSignal(int)), controlPanel, SLOT(FinishSaveImage(int)) );
		hamamatsuStopDisplayThread->start();
	}
}

void TrackingWindow::HasStopDisplaySlot()
{
	toolBarContents.isLive = false;
	hamamatsuWindowInfo.isLive = 0;
	liveAction->setEnabled(true);
	stopLiveAction->setEnabled(false);	
}

void TrackingWindow::OnResetAction()
{
	int windowFlag = displayTabs->currentIndex();
	if (windowFlag == (int)HAMAMATSU_WINDOW){
		Hamamatsu_GCaMPWindow->Reset();
		Hamamatsu_RFPWindow->Reset();
	}
}

void TrackingWindow::UpdateHamamatsuTemperature()
{
}

//0: x10
void TrackingWindow::OnObjectiveLensX10Action()
{
	objectiveLensContents[0] = true;
	objectiveLens10->setIcon(x10_onIcon);
	objectiveLensContents[1] = false;
	objectiveLens20->setIcon(x20_offIcon);
	objectiveLensContents[2] = false;
	objectiveLens40->setIcon(x40_offIcon);
	controlPanel->UpdateObjectiveLensContents(X10);
}

//1: x20
void TrackingWindow::OnObjectiveLensX20Action()
{
	objectiveLensContents[0] = false;
	objectiveLens10->setIcon(x10_offIcon);
	objectiveLensContents[1] = true;
	objectiveLens20->setIcon(x20_onIcon);
	objectiveLensContents[2] = false;
	objectiveLens40->setIcon(x40_offIcon);
	controlPanel->UpdateObjectiveLensContents(X20);
}

//2: x40
void TrackingWindow::OnObjectiveLensX40Action()
{
	objectiveLensContents[0] = false;
	objectiveLens10->setIcon(x10_offIcon);
	objectiveLensContents[1] = false;
	objectiveLens20->setIcon(x20_offIcon);
	objectiveLensContents[2] = true;
	objectiveLens40->setIcon(x40_onIcon);
	controlPanel->UpdateObjectiveLensContents(X40);
}

void TrackingWindow::OnFileSaveAction()
{
	if (hamamatsuWindowInfo.image_data != NULL){
		QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), "E:\\", tr("Images (*.tif *.tiff)"));
		int width = hamamatsuWindowInfo.image_width;
		int height = hamamatsuWindowInfo.image_height;
		//uchar* image_data = new uchar[width*height*sizeof(ushort)];
		//CopyData(USHORT_TYPE, (uchar*)hamamatsuWindowInfo.image_data, image_data, width, height);
		if (fileName.isEmpty()){ return; }
		ImageSaveWidget::SaveOneImage(fileName.toStdString(), hamamatsuWindowInfo.image_data, USHORT_TYPE, width, height);
	}
}

void TrackingWindow::GetImageWindowMargins(int window_width, int window_height, int image_width, int image_height, int &x_offset, int &y_offset)
{
	int display_width = window_width;
	int display_height = window_height;
	x_offset = 0, y_offset = 0;  
    if (image_width*window_height > window_width*image_height){
        display_width = window_width;
		display_height = window_width*image_height/image_width;
		x_offset = 0;
		y_offset = (window_height-display_height)/2; //实际图像在显示窗口中的y偏移
	}
	else{
		display_width = image_width*window_height/image_height;
		display_height = window_height;
		x_offset = (window_width-display_width)/2; //实际图像在显示窗口中的x偏移
		y_offset = 0;
	}
}

StopDisplayThread::StopDisplayThread(int window_flag)
{
	windowFlag = window_flag;
}

void StopDisplayThread::run()
{
	try{
		if (windowFlag == (int)HAMAMATSU_WINDOW){
			hamamatsuCamera->StopLive();
			emit HasStopDisplaySignal(windowFlag);
		}
	}catch (QException e){
		QMessageBox::critical(NULL, "Warning", QString::fromStdString(e.getMessage()));
	}
}