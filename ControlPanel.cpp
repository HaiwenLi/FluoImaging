#include "ControlPanel.h"
#include "DevicePackage.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtCore/QString>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtGui/QFont>
#include <vector>
#include <algorithm>

#define STOP_LIMIT 0
#define SLIDER_INTERVAL 100
string ControlPanel::OBJECT_NAME = "ControlPanel";
 
template<typename T>
double ImageEvaluation(const T*, const ImageRegion&, const int, const char*, const DataRightShift);
template<typename T>
double FluoImageEvaluation_Sort(const T*, const ImageRegion&, const int);
template<typename T>
double FluoImageEvaluation_Direct(const T*, const ImageRegion&, const int);

ControlPanel::ControlPanel(QWidget* parent) : QWidget(parent)
{
	controller = NULL;
	z1_stage = NULL;
	z1_ref_point = AUTOFOCUS_INITIAL_POINT;
	z1MotionThread = NULL;
	hamamatsuImageSaveWidget = NULL;
	laser488 = NULL;
	laser561 = NULL;
	objectiveLens = NO_SELECTED;

	CreateLayout();
	Connect_Controller();

	InitCamera();
	InitZ1Stage();
	InitLasers();
}

ControlPanel::~ControlPanel()
{
	Disconect_Controller();
	if (z1MotionThread != NULL){
		delete z1MotionThread;
		z1MotionThread = NULL;
	}
	if (hamamatsuImageSaveWidget != NULL){
		delete hamamatsuImageSaveWidget;
		hamamatsuImageSaveWidget = NULL;
	}
	if (laser488 != NULL){
		laser488->Disconnect();
		delete laser488;
		laser488 = NULL;
	}
	if (laser561 != NULL){
		laser561->Disconnect();
		delete laser561;
		laser561 = NULL;
	}
}

void ControlPanel::CreateLayout()
{
	mainBox = new QTabWidget;
	QGroupBox* hamamatsuGroup = new QGroupBox;
	QGroupBox* laserGroup = Create_LaserSetting_Layout();

	/*************** Stage Axis Control** ************/
	//stageStopIcon = QIcon(".\\Resources\\Icons\\stageStop.png");	
	QGroupBox* z1SettingBox = Create_Z1Setting_Layout();

	/*************** Camera Control** ************/
	/***** Hamamastu Camera** ***/
	hamamatsuControlBox = new QGroupBox(tr("Hamamastu Setting"));
	hamamatsuExposureTimeEdit = new QLineEdit;
	hamamatsuFrameRateEdit = new QLineEdit;
	hamamatsuExposureTimeEdit->setMaximumWidth(80);
	hamamatsuExposureTimeEdit->setMinimumWidth(70);
	hamamatsuFrameRateEdit->setMaximumWidth(80);
	hamamatsuFrameRateEdit->setMinimumWidth(70);

	hamamatsuExposureTimeRangeLabel = new QLabel(tr("Min: 0 Max: 0"));
	hamamatsuExposureTimeRangeLabel->setMinimumWidth(180);
	hamamatsuExposureTimeRangeLabel->setMaximumWidth(220);
	hamamatsuExposureTimeRangeLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);

	hamamatsuFrameRateRangeLabel = new QLabel(tr("Min: 0 Max: 0"));
	hamamatsuFrameRateRangeLabel->setMinimumWidth(180);
	hamamatsuFrameRateRangeLabel->setMaximumWidth(220);
	hamamatsuFrameRateRangeLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);

	hamamatsuOrientationBox = new QComboBox;
	hamamatsuOrientationBox->setMaximumWidth(130);
	hamamatsuOrientationBox->setMinimumWidth(125);
	hamamatsuDataMapBox = new QComboBox;
	hamamatsuDataMapBox->setMaximumWidth(85);
	hamamatsuDataMapBox->setMinimumWidth(80);
	hamamatsuCaptureModeBox = new QComboBox;
	hamamatsuCaptureModeBox->setMaximumWidth(130);
	hamamatsuCaptureModeBox->setMinimumWidth(125);
	hamamatsuExternalTriggerPositiveButton = new QRadioButton( tr("Pos") );
	hamamatsuExternalTriggerNegativeButton = new QRadioButton( tr("Neg") );

	//Hamamatsu fov setting
	hamamatsuImageSizeBox = new QComboBox;
	hamamatsuImageSizeApplyButton = new QPushButton("Apply");
	hamamatsuFovWidth = new QLineEdit;
	hamamatsuFovHeight = new QLineEdit;
	hamamatsuFovXOffset = new QLineEdit;
	hamamatsuFovYOffset = new QLineEdit;
	hamamatsuImageSizeBox->setMinimumWidth(150);
	hamamatsuImageSizeBox->setMaximumWidth(160);
	hamamatsuImageSizeApplyButton->setMinimumWidth(120);
	hamamatsuImageSizeApplyButton->setMaximumWidth(130);

	QGroupBox* hamamatsuFovSettingBox = new QGroupBox( tr("FOV Setting") );
	QHBoxLayout* hamamatsuImageSizeLayout = new QHBoxLayout;
	hamamatsuImageSizeLayout->addWidget( new QLabel("Image Area") );
	hamamatsuImageSizeLayout->addWidget( hamamatsuImageSizeBox );
	hamamatsuImageSizeLayout->addWidget( hamamatsuImageSizeApplyButton );

	QGridLayout* hamamatsuImageSizeSettingLayout = new QGridLayout;
	hamamatsuImageSizeSettingLayout->addWidget( new QLabel("X Offset"), 0, 0 );
	hamamatsuImageSizeSettingLayout->addWidget( hamamatsuFovXOffset, 0, 1 );
	hamamatsuImageSizeSettingLayout->addWidget( new QLabel("Y Offset"), 0, 2 );
	hamamatsuImageSizeSettingLayout->addWidget( hamamatsuFovYOffset, 0, 3 );
	hamamatsuImageSizeSettingLayout->addWidget( new QLabel("Width"), 1, 0 );
	hamamatsuImageSizeSettingLayout->addWidget( hamamatsuFovWidth, 1, 1 );
	hamamatsuImageSizeSettingLayout->addWidget( new QLabel("Height"), 1, 2 );
	hamamatsuImageSizeSettingLayout->addWidget( hamamatsuFovHeight, 1, 3 );

	QVBoxLayout* hamamatsuFovSettingLayout = new QVBoxLayout;
	hamamatsuFovSettingLayout->addLayout(hamamatsuImageSizeLayout);
	hamamatsuFovSettingLayout->addLayout(hamamatsuImageSizeSettingLayout);
	hamamatsuFovSettingBox->setLayout(hamamatsuFovSettingLayout);

	//Set hamamatsu frame rate setting be disabled
	hamamatsuFrameRateEdit->setEnabled(false);
	hamamatsuFrameRateRangeLabel->setEnabled(false);

	//Hamamatsu exposure time layout
	QHBoxLayout* hamamatsuExposreTimeLayout = new QHBoxLayout;
	hamamatsuExposreTimeLayout->addWidget(new QLabel(tr("Exposure Time")));
	hamamatsuExposreTimeLayout->addWidget(hamamatsuExposureTimeEdit);
	hamamatsuExposreTimeLayout->addWidget(hamamatsuExposureTimeRangeLabel);

	//Hamamatsu frame rate layout
	QHBoxLayout* hamamatsuFrameRateLayout = new QHBoxLayout;
	hamamatsuFrameRateLayout->addWidget(new QLabel(tr("Frame Rate")));
	hamamatsuFrameRateLayout->addWidget(hamamatsuFrameRateEdit);
	hamamatsuFrameRateLayout->addWidget(hamamatsuFrameRateRangeLabel);

	QHBoxLayout* hamamatsuHLayout1 = new QHBoxLayout;
	hamamatsuHLayout1->addWidget( new QLabel( tr("Orientation ") ) );
	hamamatsuHLayout1->addWidget( hamamatsuOrientationBox );
	hamamatsuHLayout1->addWidget( new QLabel( tr("Valid Bits") ) );
	hamamatsuHLayout1->addWidget( hamamatsuDataMapBox );
	hamamatsuHLayout1->addStretch();

	QHBoxLayout* hamamatsuCaptureModeLayout = new QHBoxLayout;
	hamamatsuCaptureModeLayout->addWidget( new QLabel(tr("Trigger Mode")) );
	hamamatsuCaptureModeLayout->addWidget( hamamatsuCaptureModeBox );
	hamamatsuCaptureModeLayout->addWidget( new QLabel(tr("Option")) );
	hamamatsuCaptureModeLayout->addWidget(hamamatsuExternalTriggerPositiveButton);
	hamamatsuCaptureModeLayout->addWidget(hamamatsuExternalTriggerNegativeButton);
	hamamatsuCaptureModeLayout->addStretch();

	//Hamamatus Imaging Mode
	hamamatsuSingleChannelButton = new QRadioButton(tr("Single Channel"));
    hamamatsuCompositeChannelsButton = new QRadioButton (tr("Multi Channel"));
    hamamatsuImagingChannelsSeqBox = new QComboBox;
	hamamatsuAdjustImagingChannel = new QPushButton("Adjust Channel Offset");

	//Hamamatsu control layout
	QVBoxLayout* hamamatsuLayout = new QVBoxLayout;
	hamamatsuLayout->addLayout(hamamatsuFrameRateLayout);
	hamamatsuLayout->addLayout(hamamatsuExposreTimeLayout);
	hamamatsuLayout->addLayout(hamamatsuHLayout1);
	hamamatsuLayout->addLayout(hamamatsuCaptureModeLayout);
	hamamatsuLayout->setSpacing(5);
	hamamatsuLayout->addStretch();
	hamamatsuControlBox->setLayout(hamamatsuLayout);

	 //Hamamatsu image save setting
	QGroupBox* hamamatsuImageSaveBox = new QGroupBox("Hamamatsu Image Save Setting");
	hamamatsuSaveImagesButton = new QPushButton("Save Series");
	hamamatsuSaveOneImageButton = new QPushButton("Save One");

	QHBoxLayout* hamamatsuImageSaveLayout = new QHBoxLayout;
	hamamatsuImageSaveLayout->addWidget(hamamatsuSaveImagesButton);
	hamamatsuImageSaveLayout->addWidget(hamamatsuSaveOneImageButton);
	hamamatsuImageSaveLayout->setSpacing(20);
	hamamatsuImageSaveBox->setLayout(hamamatsuImageSaveLayout);

	//Hamamatsu imaging channel sequence setting
	QGroupBox* hamamatsuImagingChannelSeqBox = new QGroupBox("Imaging Channels");
	QHBoxLayout* hamamatsuImagingChannelSeqLayout = new QHBoxLayout;
	hamamatsuImagingChannelSeqLayout->addWidget(hamamatsuSingleChannelButton);
	hamamatsuImagingChannelSeqLayout->addWidget(hamamatsuCompositeChannelsButton);
	hamamatsuImagingChannelSeqLayout->addWidget(hamamatsuImagingChannelsSeqBox);

	QHBoxLayout* hamamatsuAdjustImagingChannelLayout = new QHBoxLayout;
	hamamatsuAdjustImagingChannelLayout->addWidget(hamamatsuAdjustImagingChannel);
	hamamatsuAdjustImagingChannelLayout->addStretch();

	QVBoxLayout* hamamatsuImagingChannelMainLayout = new QVBoxLayout;
	hamamatsuImagingChannelMainLayout->addLayout(hamamatsuImagingChannelSeqLayout);
	hamamatsuImagingChannelMainLayout->addLayout(hamamatsuAdjustImagingChannelLayout);

	hamamatsuImagingChannelSeqBox->setLayout(hamamatsuImagingChannelMainLayout);

	//connect signals to the slots
	QObject::connect( hamamatsuExposureTimeEdit, SIGNAL(editingFinished()), this, SLOT(On_HamamatsuExposureTimeEdit()) );
	QObject::connect( hamamatsuOrientationBox, SIGNAL( activated(int) ), this, SLOT(On_HamamatsuOrientationBox()) );
	QObject::connect( hamamatsuDataMapBox, SIGNAL( activated(int) ), this, SLOT(On_HamamatsuDataMapBox()) );
	QObject::connect( hamamatsuCaptureModeBox, SIGNAL( activated(int) ), this, SLOT(On_HamamatsuCaptureModeBox()) );
	QObject::connect( hamamatsuExternalTriggerPositiveButton, SIGNAL( pressed() ), this, SLOT(On_HamamatsuExternalTriggerOptionButton()) );
	QObject::connect( hamamatsuExternalTriggerNegativeButton, SIGNAL( pressed() ), this, SLOT(On_HamamatsuExternalTriggerOptionButton()) );
	QObject::connect( hamamatsuSaveImagesButton, SIGNAL( pressed() ), this, SLOT(On_HamamatsuSaveImagesButton()) );
	QObject::connect( hamamatsuSaveOneImageButton, SIGNAL( pressed() ), this, SLOT(On_HamamatsuSaveOneImageButton()) );

	QObject::connect( hamamatsuSingleChannelButton, SIGNAL(clicked()), this, SLOT( On_HamamatsuImagingChannelChanged() ) );
	QObject::connect( hamamatsuCompositeChannelsButton, SIGNAL(clicked()), this, SLOT( On_HamamatsuImagingChannelChanged() ) );
	QObject::connect( hamamatsuImagingChannelsSeqBox, SIGNAL( activated(int) ), this, SLOT( On_HamamatsuImagingChannelSeqBox() ) );
	QObject::connect( hamamatsuAdjustImagingChannel, SIGNAL(pressed()), this, SLOT( On_HamamatsuAdjustImagingChannel() ) );

	//Fill some boxes
	FillHamamatsuOrientationBox();
	FillHamamatsuDataMapBox();
	FillHamamatsuCaptureModeBox();
	FillHamamatsuImageSizeBox();
	FillHamamatsuImageChannelsSeqBox();

	FillLaserModeBox(laser488ModeBox);
	FillLaserModeBox(laser561ModeBox);

	hamamatsuSingleChannelButton->setChecked(true);
    hamamatsuImagingChannelsSeqBox->setEnabled(false);

	/*************** Main Layout** ************/
	stateBox = new QTextEdit;
	stateBox->setReadOnly(true);
	stateBox->setMinimumHeight(111);
	stateBox->setMaximumHeight(115);

	QVBoxLayout* hamamatsuMainLayout = new QVBoxLayout;
	hamamatsuMainLayout->addWidget(z1SettingBox);
	hamamatsuMainLayout->addWidget(hamamatsuFovSettingBox);
	hamamatsuMainLayout->addWidget(hamamatsuControlBox);
	hamamatsuMainLayout->addWidget(hamamatsuImagingChannelSeqBox);
	hamamatsuMainLayout->addWidget(hamamatsuImageSaveBox);
	hamamatsuMainLayout->setSpacing(5);
	hamamatsuMainLayout->addStretch();
	hamamatsuGroup->setLayout(hamamatsuMainLayout);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(mainBox);
	mainLayout->addWidget(stateBox);
	mainLayout->addStretch();

	setLayout(mainLayout);
	setMaximumWidth(500);
	setWindowTitle("Control Panel");
	//setFixedWidth(450);
	this->show();
	
	mainBox->addTab(hamamatsuGroup, tr("Hamamatsu"));
	mainBox->addTab(laserGroup, tr("Laser Setting"));
}

QGroupBox* ControlPanel::Create_LaserSetting_Layout()
{
	QGroupBox* laserSettingBox = new QGroupBox;
	laserStartIcon = QIcon(".\\Resources\\Icons\\laserStart.png");
	laserStopIcon = QIcon(".\\Resources\\Icons\\laserStop.png");
	QFont statusFont;
	statusFont.setPixelSize(11);

	QGroupBox* globalControlBox = new QGroupBox("Global Control");
	laserStartAllButton = new QPushButton(laserStartIcon, "Start All");
	laserStartAllButton->setMaximumWidth(110);
	laserStartAllButton->setMinimumWidth(90);
	laserStopAllButton = new QPushButton(laserStopIcon, "Stop All");
	laserStopAllButton->setMaximumWidth(110);
	laserStopAllButton->setMinimumWidth(90);
	QGridLayout* laserControlAllLayout = new QGridLayout;
	laserControlAllLayout->addWidget(laserStartAllButton, 0, 0);
	laserControlAllLayout->addWidget(laserStopAllButton, 0, 1);
	laserControlAllLayout->setSpacing(30);
	globalControlBox->setLayout(laserControlAllLayout);

	//Laser 488
	laser488Timer = new QTimer;
	QGroupBox* laser488Box = new QGroupBox("488nm Laser Setting");
	laser488ConnectButton = new QPushButton("Connect");
	laser488StartButton = new QPushButton(laserStartIcon, "Start");
	laser488StopButton = new QPushButton(laserStopIcon, "Stop");
	laser488Status = new QLabel;
	laser488Status->setStyleSheet("color: red");
	laser488Status->setFont(statusFont);
	laser488Status->setText("No Connection");

	QHBoxLayout* laser488StatusLayout = new QHBoxLayout;
	laser488StatusLayout->addWidget( new QLabel("Laser Status: ") );
	laser488StatusLayout->addWidget(laser488Status);
	laser488StatusLayout->addStretch();

	QHBoxLayout* laser488ButtonLayout = new QHBoxLayout;
	laser488ButtonLayout->addWidget(laser488ConnectButton);
	laser488ButtonLayout->addWidget(laser488StartButton);
	laser488ButtonLayout->addWidget(laser488StopButton);
	laser488ButtonLayout->setSpacing(10);
	//laser488ButtonLayout->addWidget( new QLabel("Comm Port: COM") );

	//Laser 488 Basic Setting
	laser488BasicBox = new QGroupBox("Operating Power");
	laser488PowerEdit = new QLineEdit;
	laser488PowerEdit->setMaximumWidth(45);
	laser488PowerSlider = new QSlider(Qt::Horizontal);
	laser488PowerSlider->setMaximum(110);
	laser488PowerSlider->setMinimum(0);
	laser488PowerSlider->setTickPosition(QSlider::TicksBelow);
	laser488PowerSlider->setTickInterval(10);
	laser488ModeBox = new QComboBox;
	laser488ModeBox->setMaximumWidth(135);
	laser488ModeBox->setMinimumWidth(125);
	laser488NomialPowerLabel = new QLabel("Nomial Power: mW");

	QHBoxLayout* laser488ModeLayout = new QHBoxLayout;
	laser488ModeLayout->addWidget( new QLabel("Operating Mode") );
	laser488ModeLayout->addWidget(laser488ModeBox);
	laser488ModeLayout->addStretch();

	QHBoxLayout* laser488PowerEditLayout = new QHBoxLayout;
	laser488PowerEditLayout->addWidget( new QLabel("Operating Power(mW)") );
	laser488PowerEditLayout->addWidget(laser488PowerEdit);
	laser488PowerEditLayout->addWidget(laser488NomialPowerLabel);
	laser488PowerEditLayout->addStretch();

	QHBoxLayout* laser488PowerSliderLayout = new QHBoxLayout;
	laser488PowerSliderLayout->addWidget( new QLabel("Operating Power(%): 0") );
	laser488PowerSliderLayout->addWidget(laser488PowerSlider);
	laser488PowerSliderLayout->addWidget(new QLabel("110%") );

	QVBoxLayout* laser488BasicLayout = new QVBoxLayout;
	laser488BasicLayout->addLayout(laser488ModeLayout);
	laser488BasicLayout->addLayout(laser488PowerEditLayout);
	laser488BasicLayout->addLayout(laser488PowerSliderLayout);
	laser488BasicLayout->addStretch();
	laser488BasicBox->setLayout(laser488BasicLayout);
	
	QVBoxLayout* laser488Layout = new QVBoxLayout;
	laser488Layout->addLayout(laser488ButtonLayout);
	laser488Layout->addLayout(laser488StatusLayout);
	laser488Layout->addWidget(laser488BasicBox);
	laser488Layout->insertSpacing(2, 15);
	laser488Layout->addStretch();
	laser488Box->setLayout(laser488Layout);
	laser488Box->setMaximumHeight(300);

	//Laser 561
	laser561Timer = new QTimer;
	QGroupBox* laser561Box = new QGroupBox("561nm Laser Setting");
	laser561ConnectButton = new QPushButton("Connect");
	laser561StartButton = new QPushButton(laserStartIcon, "Start");
	laser561StopButton = new QPushButton(laserStopIcon, "Stop");
	laser561Status = new QLabel;
	laser561Status->setStyleSheet("color:red");
	laser561Status->setFont(statusFont);
	laser561Status->setText("No Connection");

	QHBoxLayout* laser561StatusLayout = new QHBoxLayout;
	laser561StatusLayout->addWidget( new QLabel("Laser Status: ") );
	laser561StatusLayout->addWidget(laser561Status);
	laser561StatusLayout->addStretch();

	QHBoxLayout* laser561ButtonLayout = new QHBoxLayout;
	laser561ButtonLayout->addWidget(laser561ConnectButton);
	laser561ButtonLayout->addWidget(laser561StartButton);
	laser561ButtonLayout->addWidget(laser561StopButton);
	laser561ButtonLayout->setSpacing(10);
	//laser561ButtonLayout->addWidget( new QLabel("Comm Port: COM") );

	//Laser 561 Basic Setting
	laser561BasicBox = new QGroupBox("Operating Power");
	laser561PowerEdit = new QLineEdit;
	laser561PowerEdit->setMaximumWidth(45);
	laser561PowerSlider = new QSlider(Qt::Horizontal);
	laser561PowerSlider->setMaximum(110);
	laser561PowerSlider->setMinimum(0);
	laser561PowerSlider->setTickPosition(QSlider::TicksBelow);
	laser561PowerSlider->setTickInterval(10);
	laser561ModeBox = new QComboBox;
	laser561ModeBox->setMaximumWidth(135);
	laser561ModeBox->setMinimumWidth(125);
	laser561NomialPowerLabel = new QLabel("Nomial Power: mW");

	QHBoxLayout* laser561ModeLayout = new QHBoxLayout;
	laser561ModeLayout->addWidget( new QLabel("Operating Mode") );
	laser561ModeLayout->addWidget(laser561ModeBox);
	laser561ModeLayout->addStretch();

	QHBoxLayout* laser561PowerEditLayout = new QHBoxLayout;
	laser561PowerEditLayout->addWidget( new QLabel("Operating Power(mW)") );
	laser561PowerEditLayout->addWidget(laser561PowerEdit);
	laser561PowerEditLayout->addWidget(laser561NomialPowerLabel);
	laser561PowerEditLayout->addStretch();

	QHBoxLayout* laser561PowerSliderLayout = new QHBoxLayout;
	laser561PowerSliderLayout->addWidget( new QLabel("Operating Power(%): 0") );
	laser561PowerSliderLayout->addWidget(laser561PowerSlider);
	laser561PowerSliderLayout->addWidget(new QLabel("110%") );

	QVBoxLayout* laser561BasicLayout = new QVBoxLayout;
	laser561BasicLayout->addLayout(laser561ModeLayout);
	laser561BasicLayout->addLayout(laser561PowerEditLayout);
	laser561BasicLayout->addLayout(laser561PowerSliderLayout);
	laser561BasicLayout->addStretch();
	laser561BasicBox->setLayout(laser561BasicLayout);

	QVBoxLayout* laser561Layout = new QVBoxLayout;
	laser561Layout->addLayout( laser561ButtonLayout);
	laser561Layout->addLayout(laser561StatusLayout);
	laser561Layout->addWidget(laser561BasicBox);
	laser561Layout->insertSpacing(2, 15);
	laser561Layout->addStretch();
	laser561Box->setLayout(laser561Layout);
	laser561Box->setMaximumHeight(300);

	//Connect signals to slots
	connect( laserStartAllButton, SIGNAL(pressed()), this, SLOT(On_LaserStartAll()) );
	connect( laserStopAllButton, SIGNAL(pressed()), this, SLOT(On_LaserStopAll()) );
	connect( laser488ConnectButton, SIGNAL(pressed()), this, SLOT(On_Laser488Connect()) );
	connect( laser488StartButton, SIGNAL(pressed()), this, SLOT(On_Laser488Start()) );
	connect( laser488StopButton, SIGNAL(pressed()), this, SLOT(On_Laser488Stop()) );
	connect( laser488ModeBox, SIGNAL(activated(int)), this, SLOT(On_Laser488ModeBox()) );
	connect( laser488PowerEdit, SIGNAL(editingFinished()), this, SLOT(On_Laser488PowerEdit()) );
	connect( laser488PowerSlider, SIGNAL(sliderReleased()), this, SLOT(On_Laser488PowerSlider()) );

	connect( laser561ConnectButton, SIGNAL(pressed()), this, SLOT(On_Laser561Connect()) );
	connect( laser561StartButton, SIGNAL(pressed()), this, SLOT(On_Laser561Start()) );
	connect( laser561StopButton, SIGNAL(pressed()), this, SLOT(On_Laser561Stop()) );
	connect( laser561ModeBox, SIGNAL(activated(int)), this, SLOT(On_Laser561ModeBox()) );
	connect( laser561PowerEdit, SIGNAL(editingFinished()), this, SLOT(On_Laser561PowerEdit()) );
	connect( laser561PowerSlider, SIGNAL(sliderReleased()), this, SLOT(On_Laser561PowerSlider()) );

	EnableLaser488Group(false);
	EnableLaser561Group(false);

	//Laser Setting
	QVBoxLayout* laserLayout = new QVBoxLayout;
	laserLayout->addWidget(globalControlBox);
	laserLayout->addWidget(laser488Box);
	laserLayout->addWidget(laser561Box);
	laserLayout->addStretch();
	laserLayout->setSpacing(20);
	laserSettingBox->setLayout(laserLayout);
	return laserSettingBox;
}

QGroupBox* ControlPanel::Create_Z1Setting_Layout()
{
	QGroupBox* z1SettingBox = new QGroupBox;
	QFont font;
	font.setBold(true);

	z1_StepEdit = new QLineEdit;
	z1_UpButton = new QPushButton(tr("Move Up"));
	z1_DownButton = new QPushButton(tr("Move Down"));
	z1_CurrentPositionButton = new QPushButton("Current Position");
	z1_StopButton = new QPushButton(tr("Stop"));
	z1_ReturnOrigin = new QPushButton(tr("Return Origin"));
	z1_RefPointButton = new QPushButton("Go Ref Position");

	z1_UpButton->setFont(font);
	z1_UpButton->setStyleSheet("color:blue");
	z1_DownButton->setFont(font);
	z1_DownButton->setStyleSheet("color:blue");
	z1_StepEdit->setMaximumWidth(120);
	z1_StepEdit->setMinimumWidth(110);
	z1_CurrentPositionButton->setFixedWidth(155);
	z1_StopButton->setMinimumHeight(35);
	z1_StopButton->setStyleSheet("color: red");
	z1_StopButton->setFont(font);
	z1_ReturnOrigin->setMinimumHeight(35);
	z1_RefPointButton->setMinimumHeight(35);
	z1_ReturnOrigin->setStyleSheet("color: darkBlue");
	z1_ReturnOrigin->setFont(font);
	z1_RefPointButton->setStyleSheet("color: #ff008f");
	z1_RefPointButton->setFont(font);

	QObject::connect( z1_StepEdit, SIGNAL(returnPressed()), this, SLOT( On_Z1StepChanged() ));
	QObject::connect( z1_CurrentPositionButton, SIGNAL(pressed()), this, SLOT(On_Z1CurrentPositionButton() ));
	QObject::connect( z1_UpButton, SIGNAL(pressed()), this, SLOT( On_Z1MoveUpButton() ));
	QObject::connect( z1_DownButton, SIGNAL(pressed()), this, SLOT( On_Z1MoveDownButton() ));
	QObject::connect( z1_StopButton, SIGNAL(pressed()), this, SLOT( On_Z1StopButton() ));
	QObject::connect( z1_ReturnOrigin, SIGNAL(pressed()), this, SLOT( On_Z1ReturnOrigin() ));
	QObject::connect( z1_RefPointButton, SIGNAL(pressed()), this, SLOT( On_Z1RefPointButton() ));

	//z1 initial control
	QGroupBox* z1ControlBox = new QGroupBox(tr("Z1 Stage Control"));
	QVBoxLayout* z1ControlLayout = new QVBoxLayout;

	QHBoxLayout* z1MoveButtonsLayout = new QHBoxLayout;
	z1MoveButtonsLayout->addWidget(z1_UpButton);
	z1MoveButtonsLayout->addWidget(z1_DownButton);
	z1MoveButtonsLayout->setSpacing(20);

	QHBoxLayout* z1StepLayout = new QHBoxLayout;
	z1StepLayout->addWidget(new QLabel(tr("Step ")));
	z1StepLayout->addWidget(z1_StepEdit);
	z1StepLayout->addWidget(new QLabel( "um") );
	z1StepLayout->addWidget(z1_CurrentPositionButton);
	z1StepLayout->setSpacing(0);
	z1StepLayout->insertSpacing(3, 20);

	QHBoxLayout* z1ControlButtonsLayout = new QHBoxLayout;
	z1ControlButtonsLayout->addWidget(z1_ReturnOrigin);
	z1ControlButtonsLayout->addWidget(z1_StopButton);
	z1ControlButtonsLayout->addWidget(z1_RefPointButton);
	z1ControlButtonsLayout->setSpacing(20);

	z1ControlLayout->addLayout(z1MoveButtonsLayout);
	z1ControlLayout->addLayout(z1StepLayout);
	z1ControlLayout->addLayout(z1ControlButtonsLayout);	
	z1ControlLayout->insertSpacing(2, 10);
	z1ControlLayout->insertSpacing(1, 5);
	z1ControlBox->setLayout(z1ControlLayout);
	
	QVBoxLayout* z1SettingLayout = new QVBoxLayout;
	z1SettingLayout->addWidget(z1ControlBox);

	z1SettingBox->setLayout(z1SettingLayout);
	return z1SettingBox;
}

void ControlPanel::InitCamera()
{
	//initialise camera's parameters
	hamamatsu_maxExposureTime = 0.0;
	hamamatsu_minExposureTime = 0.0;
	hamamatsu_exposureTime = 0.0;
	EnableHamamatsuGroup(false);
	EnableHamamatsuFovCustomGroup(false);
}

void ControlPanel::InitZ1Stage()
{
}

void ControlPanel::ShowState(const string & str)
{
	stateBox->append(QString::fromStdString(str));
}

QString ControlPanel::PrecisionConvert(double data, int precision)
{
	ostringstream Convert;
	Convert<<setprecision(precision)<<data;
	return QString::fromStdString(Convert.str());
}

void ControlPanel::ChangeTab(int index)
{
	mainBox->setCurrentIndex(index);
}

void ControlPanel::UpdateObjectiveLensContents(ObjectiveLens lens)
{
	if (lens == X10){
	}
	else if (lens == X20){
	}
	else if (lens == X40){
	}
}

void ControlPanel::StartSaveImage(int windowFlag)
{
	ImageSize size;
	if (windowFlag == (int)HAMAMATSU_WINDOW && hamamatsuImageSaveWidget != NULL){
		if (hamamatsuCamera == NULL || !hamamatsuCamera->IsConnected()){
			return; 
		}
		if (HamamatsuStartSaveImage){
			hamamatsuCamera->Get_ImageSize(size);
			HamamatsuSaveImageNum = hamamatsuImageSaveWidget->Get_ImageNum();
			HamamatsuImageBuffers = hamamatsuImageSaveWidget->Allocate(size, USHORT_TYPE);
		}
	}
}

void ControlPanel::FinishSaveImage(int window)
{
	if (window== (int)HAMAMATSU_WINDOW && hamamatsuImageSaveWidget != NULL){
		hamamatsuImageSaveWidget->SaveImages();
	}
	
	emit StopDisplayImagesSignal(window);
}

/**********************************************************************************************************
	Camera Control
************************************************************************************************************/
/********************************** Hamamatsu Camera **********************************/
void ControlPanel::EnableHamamatsuGroup(bool ok)
{
	hamamatsuControlBox->setEnabled(ok);
}

void ControlPanel::EnableHamamatsuFrameRateGroup(bool ok)
{
	hamamatsuFrameRateEdit->setEnabled(ok);
	hamamatsuFrameRateRangeLabel->setEnabled(ok);
}

void ControlPanel::EnableHamamatsuExposureTimeGroup(bool ok)
{
	hamamatsuExposureTimeEdit->setEnabled(ok);
	hamamatsuExposureTimeRangeLabel->setEnabled(ok);
}

void ControlPanel::EnableHamamatsuFovCustomGroup(bool ok)
{
	hamamatsuImageSizeApplyButton->setEnabled(ok);
	hamamatsuFovWidth->setReadOnly(!ok);
	hamamatsuFovHeight->setReadOnly(!ok);
	hamamatsuFovXOffset->setReadOnly(!ok);
	hamamatsuFovYOffset->setReadOnly(!ok);
}

void ControlPanel::Hamamatsu_UpdateExposureTimeRange()
{
	try{
		if (hamamatsuCamera!=NULL && hamamatsuCamera->IsConnected()){
			Range range;
			hamamatsuCamera->Get_ExposureTimeRange(range);
			hamamatsuCamera->Get_ExposureTime(hamamatsu_exposureTime);
			hamamatsu_maxExposureTime = range.max;
			hamamatsu_minExposureTime = range.min;

			QString exposureTimeRange = tr("Min: ") + PrecisionConvert(hamamatsu_minExposureTime) + tr(" Max: ") + PrecisionConvert(hamamatsu_maxExposureTime);
			hamamatsuExposureTimeRangeLabel->setText(exposureTimeRange);
			hamamatsuExposureTimeEdit->setText(PrecisionConvert(hamamatsu_exposureTime));
		}
	} catch(QException e){
		cout<<e.getMessage()<<endl;
	}
}

void ControlPanel::On_HamamatsuExposureTimeEdit()
{
	double exposureTime = hamamatsuExposureTimeEdit->text().toDouble();
	if (exposureTime > hamamatsu_maxExposureTime){
		exposureTime = hamamatsu_maxExposureTime;
	}
	else if(exposureTime < hamamatsu_minExposureTime){
		exposureTime = hamamatsu_minExposureTime;
	}
	hamamatsu_exposureTime = exposureTime;
	hamamatsuExposureTimeEdit->setText(PrecisionConvert(hamamatsu_exposureTime));
    
	try{
		if (hamamatsuCamera != NULL && hamamatsuCamera->IsConnected()){
			hamamatsuCamera->Set_ExposureTime(exposureTime);
		}
	} catch (QException e){
		cout<<e.getMessage()<<endl;
	}
}

void ControlPanel::FillHamamatsuImageSizeBox()
{
	hamamatsuImageSizeBox->addItem( "Full Image" );
	hamamatsuImageSizeBox->addItem( "512x512" );
	hamamatsuImageSizeBox->addItem( "Custom" );
}

void ControlPanel::FillHamamatsuOrientationBox()
{
	hamamatsuOrientationBox->addItem( tr("Normal") );
	hamamatsuOrientationBox->addItem( tr("Flip Up Down") );
	hamamatsuOrientationBox->addItem( tr("Flip Left Right") );
	hamamatsuOrientationBox->addItem( tr("Flip Both") );
	hamamatsuOrientationBox->addItem( tr("Rotation 90") );
	hamamatsuOrientationBox->addItem( tr("Rotation 180") );
	hamamatsuOrientationBox->addItem( tr("Rotation 270") );
	emit Hamamatsu_UpdateDisplayWindow();
}

void ControlPanel::FillHamamatsuDataMapBox()
{
	hamamatsuDataMapBox->addItem( tr(" 0-7 bits") );
	hamamatsuDataMapBox->addItem( tr(" 1-8 bits") );
	hamamatsuDataMapBox->addItem( tr(" 2-9 bits") );
	hamamatsuDataMapBox->addItem( tr(" 3-10 bits") );
	hamamatsuDataMapBox->addItem( tr(" 4-11 bits") );
	hamamatsuDataMapBox->addItem( tr(" 5-12 bits") );
	hamamatsuDataMapBox->addItem( tr(" 6-13 bits") );
	hamamatsuDataMapBox->addItem( tr(" 7-14 bits") );
	hamamatsuDataMapBox->addItem( tr(" 8-15 bits") );
	hamamatsuDataMapBox->setCurrentIndex(0);
}

void ControlPanel::FillHamamatsuImageChannelsSeqBox(){
	hamamatsuImagingChannelsSeqBox->addItem( tr("G1_R1_G1_R1_G1_R1") );
	hamamatsuImagingChannelsSeqBox->addItem( tr("G1_R1_G1_R1_G1") );
	hamamatsuImagingChannelsSeqBox->addItem( tr("G1_R1_G1_R1") );
}

void ControlPanel::FillHamamatsuCaptureModeBox()
{
	hamamatsuCaptureModeBox->addItem( tr("Internal") );
	hamamatsuCaptureModeBox->addItem( tr("External Trigger") ); //External Level Trigger
	hamamatsuCaptureModeBox->addItem( tr("Global Reset") );
	//hamamatsuCaptureModeBox->addItem( tr("Synchronous Readout") );
	hamamatsuCaptureModeBox->setCurrentIndex(0);
	hamamatsuExternalTriggerPositiveButton->setEnabled(false);
	hamamatsuExternalTriggerNegativeButton->setEnabled(false);
}

void ControlPanel::UpdateHamamatsuFovSetting()
{
	hamamatsuFovWidth->setText( QString::number(hamamatsu_imageWidth) );
	hamamatsuFovHeight->setText( QString::number(hamamatsu_imageHeight) );
	hamamatsuFovXOffset->setText( QString::number(hamamatsu_imageXOffset) );
	hamamatsuFovYOffset->setText( QString::number(hamamatsu_imageYOffset) );
}

void ControlPanel::On_HamamatsuImageSizeBox()
{
	QString strImageSize = hamamatsuImageSizeBox->currentText();
	if (strImageSize == "Full Image"){
		hamamatsu_imageWidth = HAMAMATSU_PARAMS::FULLIMAGE_WIDTH;
		hamamatsu_imageHeight = HAMAMATSU_PARAMS::FULLIMAGE_HEIGHT;
		hamamatsu_imageXOffset = (HAMAMATSU_PARAMS::FULLIMAGE_WIDTH -  hamamatsu_imageWidth)/2;
		hamamatsu_imageYOffset = (HAMAMATSU_PARAMS::FULLIMAGE_HEIGHT - hamamatsu_imageHeight)/2;

		//Update fov setting and set camera fov
		UpdateHamamatsuFovSetting();
		EnableHamamatsuFovCustomGroup(false);
		On_HamamatsuFovChanged();
	}
	else if(strImageSize == "512x512"){
		hamamatsu_imageWidth = 512;
		hamamatsu_imageHeight = 512;
		hamamatsu_imageXOffset = (HAMAMATSU_PARAMS::FULLIMAGE_WIDTH -  hamamatsu_imageWidth)/2;
		hamamatsu_imageYOffset = (HAMAMATSU_PARAMS::FULLIMAGE_HEIGHT - hamamatsu_imageHeight)/2;

		//Update fov setting and set camera fov
		UpdateHamamatsuFovSetting();
		EnableHamamatsuFovCustomGroup(false);
		On_HamamatsuFovChanged();
	}
	else if(strImageSize == "Custom"){
		EnableHamamatsuFovCustomGroup(true);
	}
}

void ControlPanel::On_HamamatsuFovChanged()
{
}

void ControlPanel::On_HamamatsuOrientationBox()
{
	int index = hamamatsuOrientationBox->currentIndex();
	hamamatsuWindowInfo.windowOrientation = (DisplayWindowOrientation) index;
	emit Hamamatsu_UpdateDisplayWindow();
}

void ControlPanel::On_HamamatsuDataMapBox()
{
	int index = hamamatsuDataMapBox->currentIndex();
	hamamatsuWindowInfo.dataRightShift = (DataRightShift) index;
}

void ControlPanel::On_HamamatsuImagingChannelChanged(){
	if (hamamatsuSingleChannelButton->isChecked()){
		hamamatsuWindowInfo.imagingChannelSeq = SINGLE;
		hamamatsuImagingChannelsSeqBox->setEnabled(false);
		hamamatsuWindowInfo.channelOffset = 0;
	} else if (hamamatsuCompositeChannelsButton->isChecked()){
		hamamatsuImagingChannelsSeqBox->setEnabled(true);
	}
}

void ControlPanel::On_HamamatsuAdjustImagingChannel(){
	++hamamatsuWindowInfo.channelOffset;
}

void ControlPanel::On_HamamatsuImagingChannelSeqBox(){
	QString itemText = hamamatsuImagingChannelsSeqBox->currentText();
	if (itemText == "G1_R1_G1_R1_G1_R1"){
		hamamatsu_imagingChannelSeq = COMP_G1_R1_G1_R1_G1_R1;
		hamamatsuWindowInfo.imagingChannelSeq = hamamatsu_imagingChannelSeq;
	} else if (itemText == "G1_R1_G1_R1_G1"){
		hamamatsu_imagingChannelSeq = COMP_G1_R1_G1_R1_G1;
		hamamatsuWindowInfo.imagingChannelSeq = hamamatsu_imagingChannelSeq;
	} else if (itemText == "G1_R1_G1_R1"){
		hamamatsu_imagingChannelSeq = COMP_G1_R1_G1_R1;
		hamamatsuWindowInfo.imagingChannelSeq = hamamatsu_imagingChannelSeq;
	}
}

void ControlPanel::On_HamamatsuCaptureModeBox()
{
	QString text = hamamatsuCaptureModeBox->currentText();
	try{
		if ( hamamatsuCamera != NULL && hamamatsuCamera->IsConnected() && text.compare("Internal") == 0 ){
			hamamatsuCamera->Set_TriggerMode("Internal");//DCAM_TRIGMODE_INTERNAL
			hamamatsuExternalTriggerPositiveButton->setEnabled(false);
			hamamatsuExternalTriggerNegativeButton->setEnabled(false);
			EnableHamamatsuExposureTimeGroup(true);
			Hamamatsu_UpdateExposureTimeRange();
			HAMAMATSU_DISPLAY_INTERVAL = 2;
		}
		else if( hamamatsuCamera != NULL && hamamatsuCamera->IsConnected() && text.compare("External Trigger") == 0 ){
			EnableHamamatsuExposureTimeGroup(false);
			hamamatsuCamera->Set_TriggerMode("External Level");//DCAM_TRIGMODE_LEVEL
			hamamatsuExternalTriggerPositiveButton->setEnabled(true);
			hamamatsuExternalTriggerNegativeButton->setEnabled(true);
			hamamatsuExternalTriggerPositiveButton->setChecked(true);
			On_HamamatsuExternalTriggerOptionButton();
			HAMAMATSU_DISPLAY_INTERVAL = 1;
		}
		else if( hamamatsuCamera != NULL && hamamatsuCamera->IsConnected() && text.compare("Global Reset") == 0 ){
			EnableHamamatsuExposureTimeGroup(false);
			hamamatsuCamera->Set_TriggerMode("Global Reset");//DCAM_TRIGMODE_LEVEL & 
			hamamatsuExternalTriggerPositiveButton->setEnabled(true);
			hamamatsuExternalTriggerNegativeButton->setEnabled(true);
			hamamatsuExternalTriggerPositiveButton->setChecked(true);
			On_HamamatsuExternalTriggerOptionButton();
			HAMAMATSU_DISPLAY_INTERVAL = 1;
		}
	} catch(QException e){
		cout<<e.getMessage()<<endl;
	}
}

void ControlPanel::On_HamamatsuExternalTriggerOptionButton()
{
	if (hamamatsuExternalTriggerPositiveButton->isChecked()){
		hamamatsuCamera->Set_TriggerPolarity(DCAM_TRIGPOL_POSITIVE);
	}
	else if (hamamatsuExternalTriggerNegativeButton->isChecked()){
		hamamatsuCamera->Set_TriggerPolarity(DCAM_TRIGPOL_NEGATIVE);
	}
}

void ControlPanel::On_HamamatsuSaveImagesButton()
{
	if (hamamatsuImageSaveWidget == NULL){
		hamamatsuImageSaveWidget = new ImageSaveWidget(HAMAMATSU_WINDOW, this);
		connect( hamamatsuImageSaveWidget, SIGNAL(StartSaveImageSignal(int)), this, SLOT(StartSaveImage(int)) );

		hamamatsuImageSaveWidget->setWindowTitle("Hamamatus Save Images");
		hamamatsuImageSaveWidget->show();
	}
	else{
		hamamatsuImageSaveWidget->raise();
		hamamatsuImageSaveWidget->show();
	}
}

void ControlPanel::On_HamamatsuSaveOneImageButton()
{
	if (hamamatsuCamera != NULL && hamamatsuCamera->IsConnected()){
		if (hamamatsuWindowInfo.image_data != NULL){
			QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), "E:\\", tr("Images (*.tif *.tiff)"));
			if (fileName.isEmpty()){ return; }

			int width = hamamatsuWindowInfo.image_width;
			int height = hamamatsuWindowInfo.image_height;
			uchar* data_copy = new uchar[width*height*sizeof(ushort)];
			CopyData(USHORT_TYPE, (uchar*)hamamatsuWindowInfo.image_data, data_copy, width, height);
			ImageSaveWidget::SaveOneImage(fileName.toStdString(), data_copy, USHORT_TYPE, width, height);

			delete data_copy;
		}
	}
}

void ControlPanel::CameraConnected(int index)
{
	try{
		if (index == 0){      //Hamamastu camera
			Hamamatsu_UpdateExposureTimeRange();
		}
		else if(index == 1){//Andor Camera
		}
		else if(index == 2){//IO Camera
		}
	}
	catch (QException e){
	}
}

/**********************************************************************************************************
	Stage Control
************************************************************************************************************/
void ControlPanel::Connect_Controller()
{
	try{
		controller = new Galil("192.168.1.11");
		controller->timeout_ms = 500;//time out is 500ms
	}
	catch (string e){
		ShowState(e);
		return;
	}
	//Connect the z1 and z3 stages
	z1_stage = new Z1Stage(controller);
	z1_stage->Connect();
	z1MotionThread = new MotionThread(z1_stage); //create motion thread
	connect(z1MotionThread, SIGNAL(FinishMotion()), this, SLOT(On_Z1MotionFinish()) );
}

//Disconnect the stage controller
void ControlPanel::Disconect_Controller()
{
	if (z1_stage != NULL){
		delete z1_stage;
		z1_stage = NULL;
	}
	if (controller != NULL){
		delete controller;
		controller = NULL;
	}
}

/********************************** Z1 Axis **********************************/
void ControlPanel::On_Z1MotionFinish()
{
	if (z1MotionThread->getMethod() == RETURN_ORIGIN){
		Sleep(500);//wait for setting current position to be 0
		z1_stage->Set_CurrentPosition(0);
	}
	Z1MotionButtonsEnabled(true);
	stateBox->append(z1MotionThread->getDescription());
}

void ControlPanel::On_Z1CurrentPositionButton()
{
	if (z1_stage == NULL || !z1_stage->IsConnected()){
		stateBox->setText(tr("Get current position: z3 stage no connection"));
		return;
	}
	double position = z1_stage->Get_CurrentPosition();
	stateBox->append("Current position: "+QString::number(position*Z1_STAGE::Z1_PRECISION)+"um ("+ QString::number(position) +" pulse)");
}

void ControlPanel::Z1MotionButtonsEnabled( bool ok)
{
	z1_UpButton->setEnabled(ok);
	z1_DownButton->setEnabled(ok);
	//z1_ReturnOrigin->setEnabled(ok);
	z1_RefPointButton->setEnabled(ok);
}

void ControlPanel::On_Z1MoveUpButton()
{
	On_Z1StepChanged();
	if (z1_stage == NULL || !z1_stage->IsConnected()){
		stateBox->setText(tr("Move Up: z1 stage no connection"));
		return;
	}
	try{
		if (STOP_LIMIT){
			stateBox->setText("Z1 move up: start");
			Z1MotionButtonsEnabled(false);
			z1MotionThread->setMethod(MOTION);
			z1MotionThread->setDistance(z1_step*Z_POSITIVE);
			z1MotionThread->setDescription( tr("Z1 move up: finish") );
			z1MotionThread->start();
		}
		else{
			double current_position = z1_stage->Get_CurrentPosition();
			if (current_position + z1_step > Z1_STAGE::Z1_UPLIMIT){
				double max_valid_distance = (Z1_STAGE::Z1_UPLIMIT - current_position)*Z1_STAGE::Z1_PRECISION;
				stateBox->setText("Move up: will reach up limit (" + QString::number(Z1_STAGE::Z1_UPLIMIT) +" pulse), max valid distance " + QString::number(max_valid_distance));
			}
			else{
				stateBox->setText("Z1 move up: start");
				Z1MotionButtonsEnabled(false);
				z1MotionThread->setMethod(MOTION);
				z1MotionThread->setDistance(z1_step*Z_POSITIVE);
				z1MotionThread->setDescription( tr("Z1 move up: finish") );
				z1MotionThread->start();
				//z1_stage->Move_Openloop_Unrealtime(z1_step*Z_POSITIVE);//Throw some exception from Galil because of limit
			}
		}
	}
	catch (QException e){
		stateBox->setText(QString::fromStdString(e.getMessage()));
	}
}

void ControlPanel::On_Z1MoveDownButton()
{
	On_Z1StepChanged();
	if (z1_stage == NULL || !z1_stage->IsConnected()){
		stateBox->setText(tr("Move Down: z1 stage no connection"));
		return;
	}
	try{
		stateBox->setText("Z1 move down: start");
		Z1MotionButtonsEnabled(false);
		z1MotionThread->setMethod(MOTION);
		z1MotionThread->setDistance(z1_step*(-Z_POSITIVE));
		z1MotionThread->setDescription( tr("Z1 move down: finish") );
		z1MotionThread->start();
		//z1_stage->Move_Openloop_Unrealtime(z1_step*(-Z_POSITIVE));//Throw some exception from Galil because of limits
	}
	catch (QException e){
		stateBox->setText(QString::fromStdString(e.getMessage()));
	}
}

void ControlPanel::On_Z1StopButton()
{
	if (z1_stage != NULL && z1_stage->IsConnected()){
		z1_stage->Stop();
	}
}

//z1 axis moves the origin (bottom limit)
void ControlPanel::On_Z1ReturnOrigin()
{
	if (z1_stage != NULL && z1_stage->IsConnected()){
		double start_point = z1_stage->Get_CurrentPosition();
		double end_point = 0;
		int ret = QMessageBox::warning(this, "Warning", "Are you sure to Return Origin?", QMessageBox::Ok | QMessageBox::Cancel);
		if (ret == QMessageBox::Cancel)
			return;
		try{
			stateBox->setText("Z1 return origin: start");
			Z1MotionButtonsEnabled(false);
			z1MotionThread->setMethod(RETURN_ORIGIN);
			z1MotionThread->setDescription( tr("Z1 return origin: finish") );
			z1MotionThread->start();
		} catch (QException e){
			QMessageBox::critical(this, "Error", QString::fromStdString(e.getMessage()));
		}
	}
}

void ControlPanel::On_Z1RefPointButton()
{
	if (z1_stage != NULL && z1_stage->IsConnected()){
		try{
			if (!z1_stage->InOrigin()){
				QMessageBox::critical(this, "Error", "Current position is not origin, please Return Origin.");
				return;
			}
		} catch (QException e){
			stateBox->setText(QString::fromStdString(e.getMessage()));
			return;
		}

		bool ok = false;
		z1_ref_point = (double)QInputDialog::getInt(this, "Z1 Reference Position", "Z1 Reference Position (pulse): ", Z1_STAGE::Z1_INITIAL_REF_POSITION, 0,  Z1_STAGE::Z1_INITIAL_REF_POSITION, 1, &ok);
		if (!ok){
			return; 
		}
		int ret = QMessageBox::warning(this, "Warning", "Are you sure to Go Ref Position("+QString::number(z1_ref_point)+" pulse)?", QMessageBox::Ok|QMessageBox::Cancel);
		if (ret != QMessageBox::Ok){
			return;
		}
		stateBox->setText("Z1 go reference position: start");
		Z1MotionButtonsEnabled(false);
		z1MotionThread->setMethod(MOTION);
		z1MotionThread->setDistance(z1_ref_point);
		z1MotionThread->setDescription( tr("Z1 go reference position: finish") );
		z1MotionThread->start();
	}
}

void ControlPanel::On_Z1StepChanged()
{
	double step = z1_StepEdit->text().toDouble();
	if (step<0 || step>Z1_STAGE::Z1_MAXSTEP){
		z1_StepEdit->setText(QString::number(Z1_STAGE::Z1_MAXSTEP));
		QMessageBox::critical(this, "Error", "Invalid step, max step = "+QString::number(Z1_STAGE::Z1_MAXSTEP) + "um");
		return;
	}
	z1_step = step/Z1_STAGE::Z1_PRECISION;//convert to pulse
}


/*********************************************** Motion Thread ***********************************************/
MotionThread::MotionThread(Stage* stage) : stage(stage)
{
	method = MOTION;
	distance = 0;
}

MotionThread::~MotionThread()
{
	stage = NULL;
}

void MotionThread::run()
{
	try{
		if (method == MOTION){
			stage->Move_Openloop_Unrealtime(distance);
		}
		else if (method == RETURN_ORIGIN){
			stage->ReturnOrigin();
		}
	} catch (QException e){
		cout<<e.getMessage()<<endl;
	}
	if ( method == MOTION || (method == RETURN_ORIGIN) )
		emit FinishMotion();
}

/**********************************************************************************************************
	Laser Control
************************************************************************************************************/
void ControlPanel::FillLaserModeBox(QComboBox* box)
{
	box->addItem("CW Power");
	box->addItem("Digital Modulation");
}

void ControlPanel::InitLasers()
{
	laser488NomialPower = 0;
	laser488Mode = CWP;
	laser488Power = 0;
	laser488StatusInfo = LASER_NO_CONNECTION;
	laser561NomialPower = 0;
	laser561Mode = CWP;
	laser561Power = 0;
	laser561StatusInfo = LASER_NO_CONNECTION;

	laser488StartButton->setEnabled(false);
	laser488StopButton->setEnabled(false);
	laser561StartButton->setEnabled(false);
	laser561StopButton->setEnabled(false);
}

void ControlPanel::On_LaserStartAll()
{
	int success = 0;
	if (laser488 != NULL && laser488->IsConnected() && laser488StatusInfo == LASER_READY){
		for (int i=0; i<3; ++i){
			if (laser488->Start()){
				++success;
				break;
			}
			else{ Sleep(100); }
		}
		if (success == 0){
			stateBox->append("StartAll: Cannot start 488nm laser");
		}
	}
	if (laser561 != NULL && laser561->IsConnected() && laser561StatusInfo == LASER_READY){
		success = 0;
		for (int i=0; i<3; ++i){
			if (laser561->Start()){
				++success;
				break;
			}
			else{ Sleep(100); }
		}
		if (success == 0){
			stateBox->append("StartAll: Cannot start 561nm laser");
		}
	}
}

void ControlPanel::On_LaserStopAll()
{
	int success = 0;
	if (laser488 != NULL && laser488->IsConnected() && laser488StatusInfo == LASER_EMMISION){
		for (int i=0; i<3; ++i){
			if (laser488->Stop()){
				++success;
				break;
			}
			else{ Sleep(100); }
		}
		if (success == 0){
			stateBox->append("StartAll: Cannot stop 488nm laser");
		}
	}
	if (laser561 != NULL && laser561->IsConnected() && laser561StatusInfo == LASER_EMMISION){
		success = 0;
		for (int i=0; i<3; ++i){
			if (laser561->Stop()){
				++success;
				break;
			}
			else{ Sleep(100); }
		}
		if (success == 0){
			stateBox->append("StartAll: Cannot stop 561nm laser");
		}
	}
}

//Laser 488 Control
void ControlPanel::On_Laser488RefreshStatus()
{
	if (laser488!=NULL && laser488->IsConnected()){
		LaserStatus status;
		string state = laser488->GetCurrentState(status);
		laser488StatusInfo = status;

		switch (status){
		case LASER_NO_CONNECTION:
		case LASER_FAULT:
			laser488Status->setStyleSheet("color:red");
			break;
		case LASER_WARMUP:
			laser488Status->setStyleSheet("color:green");
			break;
		case LASER_STANDBY:
			laser488StartButton->setEnabled(false);
			laser488StopButton->setEnabled(false);
			laser488Status->setStyleSheet("color:blue");
			break;
		case LASER_EMMISION:
			laser488StartButton->setEnabled(false);
			laser488StopButton->setEnabled(true);
			EnableLaser488ModeBox(false);
			laser488Status->setStyleSheet("color:black");
			break;
		case LASER_READY:
			laser488StartButton->setEnabled(true);
			laser488StopButton->setEnabled(false);
			laser488Status->setStyleSheet("color:black");
			break;
		}
		if (status != LASER_EMMISION){
			EnableLaser488ModeBox(true);
		}

		//cout<<"Laser488 current status: "<<state<<endl;
		laser488Status->setText(QString::fromStdString(state));
		laser488Timer->setInterval(2000);
		laser488Timer->start();
	}
}

void ControlPanel::On_Laser488Connect() //toggle button
{  
	static bool work = false;
	if (work){
		laser488ConnectButton->setText("Connect");
		laser488->Disconnect();
		laser488StartButton->setEnabled(false);
		laser488StopButton->setEnabled(false);
		EnableLaser488Group(false);
		work = false;

		//update status
		laser488Status->setStyleSheet("color:red");
		laser488Status->setText("No Connection");

		//stop timer
		laser488Timer->stop();
		disconnect( laser488Timer, SIGNAL(timeout()), this, SLOT(On_Laser488RefreshStatus()) );
	}
	else{
		if (laser488 == NULL){
			laser488 = new CLaser();
		}
		laser488->SetPortName(COM_488);
		laser488->SetBaudRate(115200);
		try{
			laser488->Connect();
			Sleep(500); //wait for 500ms

			work = true;
			EnableLaser488Group(true);
			laser488ConnectButton->setText("Disconnect");
			laser488->GetNomialPower(laser488NomialPower);
			laser488->GetMinPower(laser488MinPower);
			laser488->GetMaxPower(laser488MaxPower);
			laser488->GetCurrentPower(laser488Power);
			laser488NomialPowerLabel->setText("Nomial Power: " + QString::number(laser488NomialPower*1.0e3) + "mW");
			laser488PowerEdit->setText( QString::number(laser488Power*1.0e3) );

			//set laser488 power slider
			int value = int(1.0e2*laser488Power/laser488NomialPower);
			laser488PowerSlider->setValue(value);

			//get laser mode
			laser488->GetMode(laser488Mode);
			laser488ModeBox->setCurrentIndex((int)laser488Mode); //CWP: 0, DIGITAL: 1

			laser488StartButton->setEnabled(true);
			laser488StopButton->setEnabled(true);

			//start timer
			laser488Timer->setInterval(2000);//2 seconds
			connect( laser488Timer, SIGNAL(timeout()), this, SLOT(On_Laser488RefreshStatus()) );
			laser488Timer->start();
		}
		catch (QException e){
			stateBox->setText(QString::fromStdString(e.getMessage()));
		}
	}
}

void ControlPanel::On_Laser488Start()
{
	if (laser488 != NULL && laser488->IsConnected()){
		/*if (!laser488->Start()){
			stateBox->setText( tr("Fail to Start 488nm Laser") );
			return;
		}*/
		while (!laser488->Start()){
			Sleep(50);
		}
		laser488StartButton->setEnabled(false);
		laser488StopButton->setEnabled(true);
	}
}

void ControlPanel::On_Laser488Stop()
{
	if (laser488 != NULL && laser488->IsConnected()){
		/*if (!laser488->Stop()){
			stateBox->setText( tr("Fail to Stop 488nm Laser") );
			return;
		}*/
		while (!laser488->Stop()){
			Sleep(50);
		}
		laser488StartButton->setEnabled(true);
		laser488StopButton->setEnabled(false);
	}
}

void ControlPanel::On_Laser488ModeBox()
{
	LaserMode mode;
	QString strMode;
	if (laser488ModeBox->currentIndex() ==0){
		mode = CWP;
		strMode = tr("CW Power");
	}
	else if (laser488ModeBox->currentIndex() == 1){
		mode = DIGITAL;
		strMode = tr("Digital Modulation");
	}
	if (laser488 != NULL && laser488->IsConnected()){
		if (!laser488->SetMode(mode)) {
    		QMessageBox::critical(this, "Error", tr("Fail to set 488nm Laser Mode ") + strMode);
			//stateBox->setText( tr("Fail to set 488nm Laser Mode ") + strMode );
			return;
		}
	}
	laser488Mode = mode;
}

void ControlPanel::EnableLaser488Group(bool ok)
{
	laser488BasicBox->setEnabled(ok);
}

void ControlPanel::EnableLaser488ModeBox(bool ok)
{
	laser488ModeBox->setEnabled(ok);
}

void ControlPanel::On_Laser488PowerEdit()
{
	if (laser488 != NULL && laser488->IsConnected()){
		double power = (laser488PowerEdit->text()).toDouble();
		power = power*1.0e-3; //convert to mW
		if (power > laser488MaxPower|| power < laser488MinPower){
			QMessageBox::critical(this, "Error", "Invalid power input.\nMin power is "+QString::number(laser488MinPower*1.0e3)+
				"mW and max power is " + QString::number(laser488MaxPower*1.0e3)+"mW.");
			//stateBox->setText("Invalid power input");
			return;
		}
		int value = int(power*1.0e2 / laser488NomialPower);
		laser488PowerSlider->setValue(value);
		if (laser488->SetCurrentPower(power)){
			laser561Power = power;
		}
		else{
			//stateBox->setText("Fail to set 488nm laser power");
			QMessageBox::critical(this, "Error", "Fail to set 488nm laser power");
		}
	}
}

void ControlPanel::On_Laser488PowerSlider()
{
	if (laser488 != NULL && laser488->IsConnected()){
		int value = laser488PowerSlider->value();
		laser488Power = laser488NomialPower*value*1.0e-2; //value: 100%, unit: W
		laser488PowerEdit->setText(PrecisionConvert(laser488Power*1.0e3, 4)); //unit: mW
		if (!laser488->SetCurrentPower(laser488Power)){
			//stateBox->setText("Fail to set 488nm laser power");
			QMessageBox::critical(this, "Error", "Fail to set 488nm laser power");
		}
	}
}

//Laser 561 Control
void ControlPanel::On_Laser561RefreshStatus()
{  
	if (laser561!=NULL && laser561->IsConnected()){
		LaserStatus status;
		string state = laser561->GetCurrentState(status);
		laser561StatusInfo = status;

		switch (status){
		case LASER_NO_CONNECTION:
		case LASER_FAULT:
			laser561Status->setStyleSheet("color:red");
			break;
		case LASER_WARMUP:
			laser561Status->setStyleSheet("color:green");
			break;
		case LASER_STANDBY:
			laser561StartButton->setEnabled(false);
			laser561StopButton->setEnabled(false);
			laser561Status->setStyleSheet("color:blue");
			break;
		case LASER_EMMISION:
			laser561StartButton->setEnabled(false);
			laser561StopButton->setEnabled(true);
			EnableLaser561ModeBox(false);
			laser561Status->setStyleSheet("color:black");
			break;
		case LASER_READY:
			laser561StartButton->setEnabled(true);
			laser561StopButton->setEnabled(false);
			laser561Status->setStyleSheet("color:black");
			break;
		}
		if (status != LASER_EMMISION){
			EnableLaser561ModeBox(true);
		}
		//cout<<"Laser561 current status: "<<state<<endl;
		laser561Status->setText(QString::fromStdString(state));
		laser561Timer->setInterval(2000);
		laser561Timer->start();
	}
}

void ControlPanel::On_Laser561Connect() //toggle button
{
	static bool work = false;
	if (work){
		laser561ConnectButton->setText("Connect");
		laser561->Disconnect();
		laser561StartButton->setEnabled(false);
		laser561StopButton->setEnabled(false);
		EnableLaser561Group(false);
		work = false;

		//update status
		laser561Status->setStyleSheet("color:red");
		laser561Status->setText("No Connection");

		//stop laser561 timer
		laser561Timer->stop();
		disconnect( laser561Timer, SIGNAL(timeout()), this, SLOT(On_Laser561RefreshStatus()) );
	}
	else{
		if (laser561 == NULL){
			laser561 = new CLaser();
		}
		laser561->SetPortName(COM_561);
		try{
			laser561->Connect();
			Sleep(500); //wait for 500ms
			
			work = true;
			EnableLaser561Group(true);
			laser561ConnectButton->setText("Disconnect");
			laser561->GetNomialPower(laser561NomialPower);
			laser561->GetMinPower(laser561MinPower);
			laser561->GetMaxPower(laser561MaxPower);
			laser561->GetCurrentPower(laser561Power);
			laser561NomialPowerLabel->setText("Nomial Power: " + QString::number(laser561NomialPower*1.0e3) + "mW");
			laser561PowerEdit->setText( QString::number(laser561Power*1.0e3) );

			//set laser488 power slider
			int value = int(1.0e2*laser561Power/laser561NomialPower);
			laser561PowerSlider->setValue(value);

			//get laser mode
			laser561->GetMode(laser561Mode);
			laser561ModeBox->setCurrentIndex((int)laser561Mode); //CWP: 0, DIGITAL: 1

			laser561StartButton->setEnabled(true);
			laser561StopButton->setEnabled(true);

			//start laser561 timer
			laser561Timer->setInterval(2000);//2 seconds
			connect( laser561Timer, SIGNAL(timeout()), this, SLOT(On_Laser561RefreshStatus()) );
			laser561Timer->start();
		}
		catch (QException e){
			stateBox->setText(QString::fromStdString(e.getMessage()));
		}
	}
}

void ControlPanel::On_Laser561Start()
{
	if (laser561 != NULL && laser561->IsConnected()){
		/*if (!laser561->Start()){
			stateBox->setText( tr("Fail to Start 561nm Laser") );
			return;
		}*/
		while (!laser561->Start()){
			Sleep(50);
		}
		laser561StartButton->setEnabled(false);
		laser561StopButton->setEnabled(true);
	}
}

void ControlPanel::On_Laser561Stop()
{
	if (laser561 != NULL && laser561->IsConnected()){
		/*if (!laser561->Stop()){
			stateBox->setText( tr("Fail to Stop 561nm Laser") );
			return;
		}*/
		while (!laser561->Stop()){
			Sleep(50);
		}
		laser561StartButton->setEnabled(true);
		laser561StopButton->setEnabled(false);
	}
}

void ControlPanel::On_Laser561ModeBox()
{
	LaserMode mode;
	QString strMode;
	if (laser561ModeBox->currentIndex() ==0){
		mode = CWP;
		strMode = tr("CW Power");
	}
	else if (laser561ModeBox->currentIndex() == 1){
		mode = DIGITAL;
		strMode = tr("Digital Modulation");
	}
	if (laser561 != NULL && laser561->IsConnected()){
		if (!laser561->SetMode(mode)) {
    		QMessageBox::critical(this, "Error", "Fail to set 561nm laser mode" + strMode);
			//stateBox->setText( tr("Fail to set 561nm Laser Mode ") + strMode );
			return;
		}
	}
	laser561Mode = mode;
}

void ControlPanel::EnableLaser561Group(bool ok)
{
	laser561BasicBox->setEnabled(ok);
}

void ControlPanel::EnableLaser561ModeBox(bool ok)
{
	laser561ModeBox->setEnabled(ok);
}

void ControlPanel::On_Laser561PowerEdit()
{
	if (laser561 != NULL && laser561->IsConnected()){
		double power = (laser561PowerEdit->text()).toDouble();
		power = power*1.0e-3; //convert to mW
		if (power > laser561MaxPower || power < laser561MinPower){
			QMessageBox::critical(this, "Error", "Invalid power input.\nMin power is "+QString::number(laser561MinPower*1.0e3)+
				"mW and max power is " + QString::number(laser561MaxPower*1.0e3)+"mW.");
			return;
		}
		int value = int(power*1.0e2 / laser561NomialPower);
		laser561PowerSlider->setValue(value);
		if (laser561->SetCurrentPower(power)){
			laser561Power = power;
		}
		else{
    		QMessageBox::critical(this, "Error", "Fail to set 561nm laser power");
		}
	}
}

void ControlPanel::On_Laser561PowerSlider()
{
	if (laser561 != NULL && laser561->IsConnected()){
		int value = laser561PowerSlider->value();
		laser561Power = laser561NomialPower*value*1.0e-2; //value: 100%, unit: W
		laser561PowerEdit->setText(PrecisionConvert(laser561Power*1.0e3, 4)); //unit: mW
		if (!laser561->SetCurrentPower(laser561Power)){
			//stateBox->setText("Fail to set 561nm laser power");
			QMessageBox::critical(this, "Error", "Fail to set 561nm laser power");
		}
	}
}

//Calculate the image evaluation
template<typename T>
double ImageEvaluation(const T* image, const ImageRegion& region, const int ImageWidth, const char* method, const DataRightShift shift)
{
	double evaluation = 0, temp = 0;
	const T *Mi_Prev, *Mi_Current, *Mi_Next;
	int x_offset = region.x_offset, y_offset = region.y_offset, width = region.width, height = region.height;

	if (strncmp(method, "GrandientSquare", 128) == 0){
		for(int i=y_offset; i<y_offset+height-1; ++i){
			Mi_Current = image + i*ImageWidth;
			Mi_Next = image + (i+1)*ImageWidth;
			for (int j=x_offset; j<x_offset+width-1; ++j){
				evaluation += ( Square( (double)(Mi_Next[j]>>shift) - (double)(Mi_Current[j]>>shift) ) + Square( (double)(Mi_Current[j+1]>>shift) - (double)(Mi_Next[j]>>shift) ) );
			}
		}
	}
	else if (strncmp(method, "Laplacian", 128) == 0){
		for (int i = y_offset+1; i < y_offset+height-1; ++i){
			Mi_Prev = image + (i-1)*ImageWidth;
			Mi_Current = image + i*ImageWidth;
			Mi_Next = image + (i+1)*ImageWidth;
			for(int j = x_offset+1; j < x_offset+width-1; ++j){
				evaluation += Square( 4*(double)(Mi_Current[j]>>shift) - (double)(Mi_Current[j+1]>>shift) - 
					(double)(Mi_Current[j-1]>>shift) - (double)(Mi_Prev[j]>>shift) - (double)(Mi_Next[j]>>shift) );
			}
		}
	}
	else if (strncmp(method, "LaplacianA", 128) == 0){
		for (int i = y_offset+1; i < y_offset+height-1; ++i){
			Mi_Prev = image + (i-1)*ImageWidth;
			Mi_Current = image + i*ImageWidth;
			Mi_Next = image + (i+1)*ImageWidth;
			for(int j = x_offset+1;  j < x_offset+width-1; ++j){
				evaluation += ( Square( (double)(Mi_Current[j]>>shift) - (double)(Mi_Current[j+1]>>shift) ) + Square( (double)(Mi_Current[j]>>shift) - (double)(Mi_Current[j-1]>>shift) )+
					Square( (double)(Mi_Current[j]>>shift) - (double)(Mi_Prev[j]>>shift) ) + Square( (double)(Mi_Current[j]>>shift) - (double)(Mi_Next[j]>>shift) ) );
			}
		}
	}
	else if (strncmp(method, "Tenengrad", 128) == 0){
		for (int i = y_offset+1; i < y_offset+height-1; ++i){
			Mi_Prev = image + (i-1)*ImageWidth;
			Mi_Current = image + i*ImageWidth;
			Mi_Next = image + (i+1)*ImageWidth;
			for(int j = x_offset+1;  j < x_offset+width-1; ++j){
				temp = (double)(Mi_Prev[j-1]>>shift) - (double)(Mi_Next[j-1]>>shift) + 2*( (double)(Mi_Prev[j]>>shift) - (double)(Mi_Next[j]>>shift) ) + 
					(double)(Mi_Prev[j+1]>>shift) - (double)(Mi_Next[j+1]>>shift);
				evaluation += Square(temp);
				temp = (double)(Mi_Prev[j+1]>>shift) - (double)(Mi_Prev[j-1]>>shift) + 2*( (double)(Mi_Current[j+1]>>shift) - (double)(Mi_Current[j-1]>>shift) ) + 
					(double)(Mi_Next[j+1]>>shift) - (double)(Mi_Next[j-1]>>shift);
				evaluation += Square(temp);
			}
		}
	}

	return evaluation;
}

//Calculate the fluorescent image evaluation
template<typename T>
double FluoImageEvaluation_Sort(const T* image, const ImageRegion& region, const int ImageWidth)
{
	vector<T> image_data;
	ushort Fluo_Threshold = 200;
	int x_offset = region.x_offset, y_offset = region.y_offset, width = region.width, height = region.height;

	for (int i=y_offset; i<y_offset+height; ++i){
		const ushort* Mi = image+i*ImageWidth;
		for (int j=x_offset; j<x_offset+width; ++j){
			if (Mi[j] > Fluo_Threshold)
				image_data.push_back(Mi[j]);
		}
	}

	std::sort(image_data.begin(), image_data.end());
	double Range_Threshold = 0.85, FluoValue = 0, Max_Value = image_data[image_data.size()-1];
	long size = 0;
	for (size_t pos = image_data.size()-1; pos >= 0; --pos){
		if (image_data[pos]>Range_Threshold*Max_Value){
			FluoValue += (double)image_data[pos];
			++size;
		}
		else
			break;
	}
	FluoValue /= size;

	// Calculate the depth
	double depth = 0, Binary_Threshold = 0.9, temp = 0;
	size = 0;
	for (int i=y_offset; i<y_offset+height; ++i){
		const ushort* Mi = image+i*ImageWidth;  
		for (int j=x_offset; j<x_offset+width; ++j){
			temp = (double)Mi[j];
			if (temp > FluoValue*Binary_Threshold){
				++size;
				depth += temp*temp;
			}
		}
	}

	return depth/size;
}

template<typename T>
double FluoImageEvaluation_Direct(const T* image, const ImageRegion& region, const int ImageWidth)
{
	ushort Fluo_Threshold = 0;
	int x_offset = region.x_offset, y_offset = region.y_offset, width = region.width, height = region.height;
	double depth = 0, temp = 0;
	long size = 0;

	for (int i=y_offset; i<y_offset+height; ++i){
		const ushort* Mi = image+i*ImageWidth;  
		for (int j=x_offset; j<x_offset+width; ++j){
			temp = (double)Mi[j];
			if (temp > Fluo_Threshold){
				++size;
				depth += temp*temp*temp*temp*temp;
			}
		}
	}

	cout<<"Points num: "<<size<<endl;
	return depth/size;
}