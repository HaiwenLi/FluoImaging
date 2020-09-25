/*****************************************************************
ControlPanel : The control panel in Tracking Window
******************************************************************/
#ifndef _CONTROL_PANEL_H_
#define _CONTROL_PANEL_H_

#include "Z1Stage.h"
#include "Laser.h"
#include "ImageSaveWidget.h"
#include <sstream>
#include <iomanip>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QCheckBox>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtGui/QIcon>

#define COM_488 "\\\\.\\COM10"
#define COM_561 "\\\\.\\COM11"

#define IMAGE_MAX 32
enum MOTION_METHOD{ MOTION, RETURN_ORIGIN};

class MotionThread: public QThread
{
	Q_OBJECT
public:
	MotionThread(Stage* stage);
	~MotionThread();

	inline void setMethod( MOTION_METHOD m){ method = m; }
	inline void setDistance(double d){ distance = d; }
	inline void setDescription(QString& str){ description  = str; }
	inline QString& getDescription() { return description; }
	inline MOTION_METHOD getMethod(){ return method; }

signals:
	void FinishMotion();

protected:
	void run() Q_DECL_OVERRIDE;

private:
	Stage* stage;
	MOTION_METHOD method;
	double distance;
	QString description;
};

class ControlPanel : public QWidget
{
	Q_OBJECT
public:
	static string OBJECT_NAME;
	explicit ControlPanel(QWidget* parent=0);
	~ControlPanel();

	void Connect_Controller();  //Connect to the stage controller
	void Disconect_Controller();//Disconnect the stage controller
	void ChangeTab(int index);
	void CameraConnected(int index); 
	void UpdateObjectiveLensContents(ObjectiveLens lens);

	void Hamamatsu_UpdateExposureTimeRange();
	void EnableHamamatsuGroup(bool ok);

signals:
	void Hamamatsu_UpdateDisplayWindow();
	void StopDisplayImagesSignal(int);

public slots:
	void On_Z1ReturnOrigin();
	void On_Z1MotionFinish();
	void StartSaveImage(int);
	void FinishSaveImage(int);

protected:
	void InitCamera(); 
	void InitZ1Stage();
	void InitLasers();
	void CreateLayout();
	QGroupBox* Create_LaserSetting_Layout();
	QGroupBox* Create_Z1Setting_Layout();
	void Z1MotionButtonsEnabled( bool ok); 
	void UpdateZ1FocusImageRegion();

	void EnableHamamatsuFrameRateGroup(bool ok);
	void EnableHamamatsuExposureTimeGroup(bool ok);
	void EnableHamamatsuFovCustomGroup(bool ok);
	void FillHamamatsuOrientationBox();
	void FillHamamatsuDataMapBox();
	void FillHamamatsuCaptureModeBox();
	void FillHamamatsuImageSizeBox();
	void FillHamamatsuImageChannelsSeqBox();
	void UpdateHamamatsuFovSetting();

	void EnableLaser488Group(bool ok);
	void EnableLaser488ModeBox(bool ok);
	void EnableLaser561Group(bool ok);
	void EnableLaser561ModeBox(bool ok);
	void FillLaserModeBox(QComboBox* box);
	QString PrecisionConvert(double data, int precision=2);

protected slots:
	void ShowState(const string & str);
	void On_Z1StepChanged();
	void On_Z1CurrentPositionButton();
	void On_Z1StopButton();
	void On_Z1RefPointButton();
	void On_Z1MoveUpButton();
	void On_Z1MoveDownButton();

	void On_HamamatsuExposureTimeEdit();
	void On_HamamatsuOrientationBox();
	void On_HamamatsuDataMapBox();
	void On_HamamatsuCaptureModeBox();
	void On_HamamatsuExternalTriggerOptionButton();
	void On_HamamatsuImageSizeBox();
	void On_HamamatsuFovChanged();
	void On_HamamatsuSaveImagesButton();
	void On_HamamatsuSaveOneImageButton();
	void On_HamamatsuImagingChannelSeqBox();
	void On_HamamatsuAdjustImagingChannel();
	void On_HamamatsuImagingChannelChanged();

	void On_LaserStartAll();
	void On_LaserStopAll();
	void On_Laser488Connect(); //toggle button
	void On_Laser488Start();
	void On_Laser488Stop();
	void On_Laser488ModeBox();
	void On_Laser488PowerEdit();
	void On_Laser488PowerSlider();
	void On_Laser488RefreshStatus();

	void On_Laser561Connect(); //toggle button
	void On_Laser561Start();
	void On_Laser561Stop();
	void On_Laser561ModeBox();
	void On_Laser561PowerEdit();
	void On_Laser561PowerSlider();
	void On_Laser561RefreshStatus();

private:
	QTabWidget* mainBox;
	/***** satge control** ***/
	QIcon stageStopIcon;
	QLineEdit* z1_StepEdit;
	QPushButton* z1_UpButton;
	QPushButton* z1_DownButton;
	QPushButton* z1_StopButton;
	QPushButton* z1_ReturnOrigin;
	QPushButton* z1_RefPointButton;
	QPushButton* z1_SetUpPointButton;
	QPushButton* z1_SetDownPointButton;
	QPushButton* z1_CurrentPositionButton;

	/***** camera control** ***/
	//hamamastu camera
	QGroupBox* hamamatsuControlBox;
	QLineEdit* hamamatsuExposureTimeEdit;
	QLabel* hamamatsuExposureTimeRangeLabel;
	QLineEdit* hamamatsuFrameRateEdit;
	QLabel* hamamatsuFrameRateRangeLabel;
	
	QComboBox* hamamatsuOrientationBox;
	QComboBox* hamamatsuDataMapBox;
	QComboBox* hamamatsuCaptureModeBox;
	QRadioButton* hamamatsuExternalTriggerPositiveButton;
	QRadioButton* hamamatsuExternalTriggerNegativeButton;
	QComboBox* hamamatsuImageSizeBox;
	QPushButton* hamamatsuImageSizeApplyButton;
	QLineEdit* hamamatsuFovWidth;
	QLineEdit* hamamatsuFovHeight;
	QLineEdit* hamamatsuFovXOffset;
	QLineEdit* hamamatsuFovYOffset;

	QRadioButton* hamamatsuSingleChannelButton;
	QRadioButton* hamamatsuCompositeChannelsButton;
	QComboBox* hamamatsuImagingChannelsSeqBox;
	QPushButton* hamamatsuAdjustImagingChannel;

	QPushButton* hamamatsuSaveImagesButton;
	QPushButton* hamamatsuSaveOneImageButton;
	ImageSaveWidget* hamamatsuImageSaveWidget;

	/***** Laser control ** ***/
	CLaser* laser488;
	CLaser* laser561;
	QPushButton* laserStartAllButton;
	QPushButton* laserStopAllButton;
	QIcon laserStartIcon;
	QIcon laserStopIcon;

	QTimer* laser488Timer;
	QGroupBox* laser488BasicBox;
	QPushButton* laser488ConnectButton;
	QPushButton* laser488StartButton;
	QPushButton* laser488StopButton;
	QComboBox* laser488ModeBox;
	QLineEdit* laser488PowerEdit;
	QSlider* laser488PowerSlider;
	QLabel* laser488Status;
	QLabel* laser488NomialPowerLabel;

	QTimer* laser561Timer;
	QGroupBox* laser561BasicBox;
	QPushButton* laser561ConnectButton;
	QPushButton* laser561StartButton;
	QPushButton* laser561StopButton;
	QComboBox* laser561ModeBox;
	QLineEdit* laser561PowerEdit;
	QSlider* laser561PowerSlider;
	QLabel* laser561Status;
	QLabel* laser561NomialPowerLabel;

	/***** others ** ***/
	QTextEdit* stateBox;//The textbox for show the states of all devices
	Galil* controller;
	Z1Stage* z1_stage;
	
	//private variables
	double z1_step;
	double z1_ref_point; 
	MotionThread* z1MotionThread;

	double hamamatsu_maxExposureTime;
	double hamamatsu_minExposureTime;
	double hamamatsu_exposureTime;
	int hamamatsu_imageWidth;
	int hamamatsu_imageHeight;
	int hamamatsu_imageXOffset;
	int hamamatsu_imageYOffset;
	ImagingChannelsSeq hamamatsu_imagingChannelSeq;

	double laser488MinPower;
	double laser488MaxPower;
	double laser488NomialPower;
	LaserMode laser488Mode;
	LaserStatus laser488StatusInfo;
	double laser488Power;
	
	double laser561MinPower;
	double laser561MaxPower;
	double laser561NomialPower;
	LaserMode laser561Mode;
	LaserStatus laser561StatusInfo;
	double laser561Power;

	ObjectiveLens objectiveLens; //current objective lens
};
#endif //_CONTROL_PANEL_H_