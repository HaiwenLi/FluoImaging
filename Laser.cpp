
#include "Laser.h"
#include <stdlib.h>

std::string CLaser::OBJECT_NAME = "CLaser";
std::string CLaser::DEVICE_NAME = "Laser";

CLaser::CLaser()
{
	baudRate = 9600;
	serialPort = NULL;
	serialState = SERIAL_CLOSED;
	laserMode = CWP;
}

void CLaser::Connect()
{
	serialPort = new CSerialPort();
	if (serialPort->Init_Com(portName, baudRate) != SERIAL_CONNECTION_SUCCESS){
		//cout<<GetErrorString(OBJECT_NAME, "InitCom()", "Fail to conect laser")<<endl;
		throw QException(OBJECT_NAME, "InitCom()", "Fail to conect laser");
	}
	serialState = SERIAL_OPENED;
}

void CLaser::Disconnect()
{
	if (serialPort != NULL){
		serialPort->ClosePort();
		delete serialPort;
		serialPort = NULL;
	}
	serialState = SERIAL_CLOSED;
}

void CLaser::SetPortName(char* port)
{
	strncpy(portName, port, 16);
}

//Send broadcast message for all devices to start
bool CLaser::StartAll()
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SOURce255:AM:STATe ON\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		return AnalyseData(serialPort->readBuffer, numBytes);
	}
	return false;
}

//Send broadcast message for all devices to stop
bool CLaser::StopAll()
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SOURce255:AM:STATe OFF\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		return AnalyseData(serialPort->readBuffer, numBytes);
	}
	return false;
}
	
bool CLaser::Start()
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SOURce:AM:STATe ON\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		return AnalyseData(serialPort->readBuffer, numBytes);
	}
	return false;
}

bool CLaser::Stop()
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SOURce:AM:STATe OFF\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		return AnalyseData(serialPort->readBuffer, numBytes);
	}
	return false;
}

bool CLaser::SetMode(LaserMode mode)
{
	if (mode != CWP && mode != DIGITAL){ return false; }
	if (serialPort != NULL && IsConnected()){
		if (mode == CWP){
			serialPort->Write_Data("SOURce:AM:INTernal CWP\n\r");
		} else{
			serialPort->Write_Data("SOURce:AM:EXTernal DIGital\n\r");
		}
		Sleep(SERIAL_QUERY_DELAY);

		//receive the returned message
		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		bool success = AnalyseData(serialPort->readBuffer, numBytes);
		if (success){
			laserMode = mode;
		}
		return success;
	}
	return false;
}

bool CLaser::GetMode(LaserMode &mode)
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SOURce:AM:SOURce?\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		bool success = false;
		if (AnalyseData(serialPort->readBuffer, numBytes)){
			if (strncmp(LaserValue, "CWP", 3) == 0){
				mode = CWP;     //constant power
				laserMode = mode;
				success = true;
			}
			else if (strncmp(LaserValue, "DIGITAL", 6) == 0){
				mode = DIGITAL; //Digital modulation
				laserMode = mode;
				success = true;
			}
			return success;
		}
		else { return false; }
	}
	return false;
}

string CLaser::GetCurrentState(LaserStatus &status)
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SYSTem:STATus?\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		if (AnalyseData(serialPort->readBuffer, numBytes)){
			//generally, the size of status code is 8
			if (strlen(LaserValue) < 8){
				status = LASER_FAULT;
				return "Invalid Status Code";
			}
			long statusCode = strtol(LaserValue+4, NULL, 16);
			if (statusCode & 0x1){ 
				status = LASER_FAULT;
				return "Fault"; 
			}
			else if (statusCode & 0x2){ 
				status = LASER_EMMISION;
				return "Emission"; 
			}
			else if ((statusCode & 0x4) || (statusCode & 0x1000 && statusCode & 0x08)){ 
				status = LASER_READY;
				return "Ready";
			}
			else if (statusCode & 0x8){ 
				status = LASER_STANDBY;
				return "Key Switch Standby";
			}
			else if (statusCode & 0x100){ 
				status = LASER_WARMUP;
				return "Warm Up"; 
			}
		}
		else { 
			status = LASER_FAULT;
			return "No Returned Status"; 
		}
	}
	status = LASER_NO_CONNECTION;
	return "No Connection";
}

string CLaser::GetFaultInfo()
{
	if (serialPort != NULL && IsConnected()){
	}
	return "No Connection";
}

bool CLaser::SetCDRHDelay(int state)
{
	if (serialPort != NULL && IsConnected()){
		if (state == 1){
			serialPort->Write_Data("SYSTem:CDRH ON\n\r");
		}
		else if (state == 0){
			serialPort->Write_Data("SYSTem:CDRH OFF\n\r");
		}
		else {return false;}
		Sleep(SERIAL_QUERY_DELAY);

		//receive the returned message
		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		return AnalyseData(serialPort->readBuffer, numBytes);
	}
	return false;
}

//state=1: ON, state=0: OFF
bool CLaser::GetCDRHDelayState(int state)
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SYSTem:CDRH?\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		bool success = false;
		if (AnalyseData(serialPort->readBuffer, numBytes)){
			if (strncmp(LaserValue, "ON", 2) == 0){
				state = 1; // ON
				success = true;
			}
			else if (strncmp(LaserValue, "OFF", 3) == 0){
				state = 0; //OFF
				success = true;
			}
			return success;
		}
		else { return false; }
	}
	return false;
}

//state=1: ON, state=0: OFF
bool CLaser::SetAutoStartState(int state)
{
	if (serialPort != NULL && IsConnected()){
		if (state == 1){
			serialPort->Write_Data("SYSTem:AUTostart ON\n\r");
		}
		else if (state == 0){
			serialPort->Write_Data("SYSTem:AUTostart OFF\n\r");
		}
		else{ return false; }
		Sleep(SERIAL_QUERY_DELAY);

		//receive the returned message
		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		return AnalyseData(serialPort->readBuffer, numBytes);
	}
	return false;
}

bool CLaser::GetAutoStartState(int state)
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SYSTem:AuTostart?\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		bool success = false;
		if (AnalyseData(serialPort->readBuffer, numBytes)){
			if (strncmp(LaserValue, "ON", 2) == 0){
				state = 1; // ON
				success = true;
			}
			else if (strncmp(LaserValue, "OFF", 3) == 0){
				state = 0; //OFF
				success = true;
			}
			return success;
		}
		else { return false; }
	}
	return false;
}

bool CLaser::GetMaxPower(double &max_power)
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SOURce:POWer:LIMit:HIGH?\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		if (AnalyseData(serialPort->readBuffer, numBytes)){
			maxPower = atof(LaserValue);
			max_power = maxPower;
			return true;
		}
		else { return false; }
	}
	return false;
}

bool CLaser::GetMinPower(double &min_power)
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SOURce:POWer:LIMit:LOW?\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		if (AnalyseData(serialPort->readBuffer, numBytes)){
			minPower = atof(LaserValue);
			min_power = minPower;
			return true;
		}
		else { return false; }
	}
	return false;
}

bool CLaser::GetNomialPower(double &nomial_power)
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SOURce:POWer:NOMinal?\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		if (AnalyseData(serialPort->readBuffer, numBytes)){
			nomialPower = atof(LaserValue);
			nomial_power = nomialPower;
			return true;
		}
		else { return false; }
	}
	return false;
}

bool CLaser::SetCurrentPower(double power)
{
	if (serialPort != NULL && IsConnected()){
		sprintf(LaserValue, "SOURce:POWer:LEVel:IMMediate:AMPLitude %f\n\r", power);
		serialPort->Write_Data(LaserValue);
		Sleep(SERIAL_QUERY_DELAY);

		//receive the returned message
		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		return AnalyseData(serialPort->readBuffer, numBytes);
	}
	return false;
}

bool CLaser::GetCurrentPower(double &power)
{
	if (serialPort != NULL && IsConnected()){
		serialPort->Write_Data("SOURce:POWer:LEVel:IMMediate:AMPLitude?\n\r");
		Sleep(SERIAL_QUERY_DELAY);

		serialPort->ClearReadBuffer();
		int numBytes = serialPort->Receive_Data();
		if (AnalyseData(serialPort->readBuffer, numBytes)){
			currentPower = atof(LaserValue);
			power = currentPower;
			return true;
		}
		else { return false; }
	}
	return false;
}

bool CLaser::AnalyseData(char* data, int num)
{
	bool success = false;
	int index = 0;
	memset(LaserValue, 0, sizeof(LaserValue));
	for (int i=0; i<num-3; ++i){
		if (strncmp(data+i, "OK\r\n", 4) == 0){
			success = true;
			LaserValue[index] = '\0';
			break;
		}
		else if (data[i] != ' '){
			LaserValue[index++] = data[i];
		}
	}
	return success;
}