
#include "Z3Stage.h"
#include <Windows.h>

string Z3Stage::DEVICE_NAME = "Z3 Stage";
string Z3Stage::OBJECT_NAME = "Z3Stage";

Z3Stage::Z3Stage(Galil* controller)
{
	stage = controller;
	state = OPENED;
}

Z3Stage::~Z3Stage()
{
	Disconnect();
}

bool Z3Stage::Connect()
{
	bool success = false;
	char strCommand[32];
	try{
		stage->write("SHX\r");
		//stage->command("SHX","\r",":");
		sprintf(strCommand, "SPX=%ld", Z3_STAGE::Z3_SPEED);
		stage->command(string(strCommand));
		Sleep(100);

		success = true;
		state = CONNECTED;
	} catch (string e){
		throw QException(OBJECT_NAME, "Connect", e);
	}
	return success;  
}

bool Z3Stage::IsConnected()
{
	if (stage != NULL && state == CONNECTED){
		return true;
	}
	else{ return false; }
}

void Z3Stage::Disconnect()
{
	Stop();
	stage = NULL;
	state = DISCONNECTED;
}

//Z3轴升降台开环实时运动(不需要等待运动完成而直接向控制器发送下一条运动指令)
void Z3Stage::Move_Openloop_Realtime(double pulse)
{
	long step = long(pulse);
	char strCommand[32];
	if (step == 0 || !IsConnected()){ return; }

	try{
		sprintf(strCommand, "IPX=%ld", step);
		stage->command(string(strCommand), "\r", ":", true);
	}
	catch (string e){
		throw QException(OBJECT_NAME, "Move_Openloop_Realtime", e);
	}
}

//Z3轴升降台开环非实时运动(需要等待运动完成后再发送下一条运动指令)
void Z3Stage::Move_Openloop_Unrealtime(double pulse)
{
	long step = long(pulse);
	char strCommand[32];
	if (step == 0 || !IsConnected()){ return; }

	try{
		//Query whether the stage is busy
		while (true){
			if (stage->commandValue("MG _BGX") == 0){ break; }
			Sleep(Z3_STAGE::STAGE_WAITING);
		}

		//Send motion instruction
		sprintf(strCommand, "IPX=%ld", step);
		stage->command(string(strCommand), "\r", ":", true);
		double motionTime = Get_MotionTime(Z3_STAGE::Z3_SPEED, step);
		Sleep(long(2*motionTime));

		//Wait the instuction to finish
		while (true){
			if (stage->commandValue("MG _BGX") == 0){ break; }
			Sleep(Z3_STAGE::STAGE_WAITING);
		}
	}
	catch (string e){
		throw QException(OBJECT_NAME, "Move_Openloop_Unrealtime", e);
	}
}

//Z3轴升降台闭环实时运动(不需要等待运动完成而直接向控制器发送下一条运动指令)
void Z3Stage::Move_Closeloop_Realtime(double pulse)
{
}

//Z3轴升降台闭环非实时运动(需要等待运动完成后再发送下一条运动指令)
void Z3Stage::Move_Closeloop_Unrealtime(double pulse)
{
	
}

void Z3Stage::Stop()
{
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "Stop", "No Stage Connection");
	}
	else{
		try{
			stage->write("STX\r");//控制器急停指令
		}
		catch (string e){
			throw QException(OBJECT_NAME, "Stop", e);
		}
	}
}

bool Z3Stage::ReturnOrigin()
{
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "ReturnOrigin", "No Stage Connection");
	}
	else{
		try{
		}
		catch (string e){
			throw QException(OBJECT_NAME, "ReturnOrigin", e);
		}
	}
	return true;
}

double Z3Stage::Get_CurrentPosition()
{
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "Get_CurrentPosition", "No Stage Connection");
	}
	else{
		try{
			return stage->commandValue("MG _TPX");//由于使用光栅尺，因此可直接使用TPZ指令读取当前位置。因此可直接使用DPZ指令设置当前位置
		}
		catch (string e){
			throw QException(OBJECT_NAME, "Get_CurrentPosition", e);
		}
	}
}

void Z3Stage::Set_CurrentPosition(double position)
{
	char strCommand[32];
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "Set_CurrentPosition", "No Stage Connection");
	}
	else{
		try{
			sprintf(strCommand, "DEX=%ld", long(position));
			stage->command(strCommand, "\r", ":", true);
		}
		catch (string e){
			throw QException(OBJECT_NAME, "Set_CurrentPosition", e);
		}
	}
}

void Z3Stage::Set_Speed(double speed)
{
	char strCommand[32];
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "Set_Speed", "No Stage Connection");
	}
	else{
		try{
			sprintf(strCommand, "SPX=%ld", long(speed));
			stage->command(string(strCommand), "\r", ":", true);
		}
		catch (string e){
			throw QException(OBJECT_NAME, "Set_Speed", e);
		}
	}
}

//set acceleration speed
void Z3Stage::Set_ACCSpeed(double acc)
{
	char strCommand[32];
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "Set_ACCSpeed", "No Stage Connection");
	}
	else{
		try{
			sprintf(strCommand, "ACX=%ld", long(acc));
			stage->command(string(strCommand), "\r", ":", true);
		}
		catch (string e){
			throw QException(OBJECT_NAME, "Set_ACCSpeed", e);
		}
	}
}

//set deceleration speed
void Z3Stage::Set_DECSpeed(double dec)
{
	char strCommand[32];
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "Set_DECSpeed", "No Stage Connection");
	}
	else{
		try{
			sprintf(strCommand, "DCX=%ld", long(dec));
			stage->command(string(strCommand), "\r", ":", true);
		}
		catch (string e){
			throw QException(OBJECT_NAME, "Set_DECSpeed", e);
		}
	}
}

double Z3Stage::Get_Speed()
{
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "Get_Speed", "No Stage Connection");
	}
	else{
		try{
			return stage->commandValue("MG _SPX");
		}
		catch (string e){
			throw QException(OBJECT_NAME, "Get_Speed", e);
		}
	}
}

double Z3Stage::Get_ACCSpeed()
{
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "Get_ACCSpeed", "No Stage Connection");
	}
	else{
		try{
			return stage->commandValue("MG _ACX");
		}
		catch (string e){
			throw QException(OBJECT_NAME, "Get_Speed", e);
		}
	}
}

double Z3Stage::Get_DECSpeed()
{
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "Get_DECSpeed", "No Stage Connection");
	}
	else{
		try{
			return stage->commandValue("MG _DCX");
		}
		catch (string e){
			throw QException(OBJECT_NAME, "Get_Speed", e);
		}
	}
}
