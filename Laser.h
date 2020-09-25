
#ifndef _LASER_H_
#define _LASER_H_

#include "serial.h"
#include "Util.h"
#include "QException.h"

enum LaserMode{CWP, DIGITAL};
enum LaserStatus{LASER_NO_CONNECTION, LASER_FAULT, LASER_EMMISION, LASER_READY, LASER_STANDBY, LASER_WARMUP};

class CLaser
{
public:
	static std::string OBJECT_NAME;
	static std::string DEVICE_NAME;

	CLaser();
	inline bool IsConnected(){ return serialState == SERIAL_OPENED; }
	inline void SetLaserDescription(string description){ laserDescription = description; }
	void SetPortName(char* port);
	inline void SetBaudRate(long rate = 9600) { baudRate = rate; }

	void Connect();
	void Disconnect();
	bool StartAll();
	bool StopAll();
	bool Start();
	bool Stop();
	CSerialPort* GetCurrentSerialPort(){ return serialPort; } // Only available for Start All or Stop All

	string GetCurrentState(LaserStatus &status);
	string GetFaultInfo();
	bool SetMode(LaserMode mode);
	bool GetMode(LaserMode &mode);
	bool SetCDRHDelay(int state);
	bool GetCDRHDelayState(int state); //state=1: ON, state=0: OFF
	bool SetAutoStartState(int state);  
	bool GetAutoStartState(int state);

	bool SetCurrentPower(double power);
	bool GetCurrentPower(double &power);
	bool GetMaxPower(double &max_power);
	bool GetMinPower(double &min_power);
	bool GetNomialPower(double &nomial_power);

protected:
	bool AnalyseData(char* data, int num);

private:
	SerialStatus serialState;
	long baudRate;
	char portName[16];
	string laserDescription;
	CSerialPort* serialPort;

	char LaserValue[256];
	double maxPower;
	double minPower;
	double nomialPower;
	double currentPower;
	LaserMode laserMode;
};

#endif //_LASER_H_