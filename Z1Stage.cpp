
#include "Z1Stage.h"
#include <Windows.h>

string Z1Stage::DEVICE_NAME = "Z1 Stage";
string Z1Stage::OBJECT_NAME = "Z1Stage";

Z1Stage::Z1Stage(Galil* controller)
{
	stage = controller;
	state = OPENED;
}

Z1Stage::~Z1Stage()
{
	Disconnect();
}

bool Z1Stage::Connect()
{
	bool success = false;
	char strCommand[32];
	try{
		stage->write("SHY\r");
		//stage->command("SHY","\r",":");
		sprintf(strCommand, "SPY=%ld", Z1_STAGE::Z1_SPEED);
		stage->command(string(strCommand));
		Sleep(100);

		success = true;
		state = CONNECTED;
	} catch (string e){
		throw QException(OBJECT_NAME, "Connect", e);
	}
	return success;
}

bool Z1Stage::IsConnected()
{
	return ((stage!=NULL) && (state==CONNECTED));
}

void Z1Stage::Disconnect()
{
	Stop();
	stage = NULL;
	state = DISCONNECTED;
}

//Z1轴升降台开环实时运动(不需要等待运动完成而直接向控制器发送下一条运动指令)
void Z1Stage::Move_Openloop_Realtime(double pulse)
{
	long step = long(pulse/Z1_STAGE::Z1_VERTICAL_TO_HORIZONTAL);
	char strCommand[32];
	if (step == 0 || !IsConnected()){return;}

	try{
		sprintf(strCommand,"IPY=%ld",step);
		stage->command(string(strCommand), "\r", ":", true);
	}catch(string e){
		throw QException(OBJECT_NAME, "Move_Openloop_Realtime", e);
	}
}

//Z1轴升降台开环非实时运动(需要等待运动完成后再发送下一条运动指令)
void Z1Stage::Move_Openloop_Unrealtime(double pulse)
{
	long step = long(pulse/Z1_STAGE::Z1_VERTICAL_TO_HORIZONTAL);
	char strCommand[32];
	if (step == 0 || !IsConnected()){return;}

	try{
		//Query whether the stage is busy
		while (true){
			if (stage->commandValue("MG _BGY")==0){break;}
			Sleep(Z1_STAGE::STAGE_WAITING);
		}

		//Send motion instruction
		sprintf(strCommand,"IPY=%ld",step);
		stage->command(string(strCommand), "\r", ":", true);
		double motionTime = Get_MotionTime(Z1_STAGE::Z1_SPEED, step);
		Sleep(long(2*motionTime));

		//Wait the instuction to finish
		while (true){
			if (stage->commandValue("MG _BGY")==0){break;}
			Sleep(Z1_STAGE::STAGE_WAITING);
		}
	}catch(string e){
		throw QException(OBJECT_NAME, "Move_Openloop_Unrealtime", e);
	}
}

//Z1轴升降台闭环实时运动(不需要等待运动完成而直接向控制器发送下一条运动指令)
void Z1Stage::Move_Closeloop_Realtime(double pulse)
{
}

//Z1轴升降台闭环非实时运动(需要等待运动完成后再发送下一条运动指令)
void Z1Stage::Move_Closeloop_Unrealtime(double pulse)
{
	long z_ideal = 0, z_input = 0, z_current = 0, z_start = 0;
	z_ideal = long(pulse);
	z_input = long(pulse/Z1_STAGE::Z1_VERTICAL_TO_HORIZONTAL);
	char strCommand[32];

	if (z_ideal == 0 || !IsConnected()){return;}

	try{
		//Query whether the stage is busy
		while (true){
			if (stage->commandValue("MG _BGY")==0){ break; }
			Sleep(Z1_STAGE::STAGE_WAITING);
		}

		//Get the initial position
		z_start = (long)stage->commandValue("MG _TPY");	

		while(true){
			/*
			  Get current z1 stage position and calculate the offset between current
			  position and the ideal position, then move the offset iteratively.
			*/
			sprintf(strCommand,"IPY=%ld",z_input);
			stage->command(string(strCommand), "\r", ":", true);
			double motionTime = Get_MotionTime(Z1_STAGE::Z1_SPEED, z_input);
			Sleep(long(2*motionTime));

			//Wait to finish motion instruction
			while (true){
				if (stage->commandValue("MG _BGY")==0){ break; }
				Sleep(Z1_STAGE::STAGE_WAITING);
			}

			z_current = (long)stage->commandValue("MG _TPY");
			z_input = long((z_ideal-(z_current-z_start))/Z1_STAGE::Z1_VERTICAL_TO_HORIZONTAL);
			if (abs(z_input) < Z1_STAGE::Z1_TOL){ break; }
		}
	}catch(string e){
		throw QException(OBJECT_NAME,"Move_Closeloop_Unrealtime",e);
	}
}

void Z1Stage::Stop()
{
	if (!IsConnected()){
    	throw QException(OBJECT_NAME, "Stop", "No Stage Connection");
	}
	else{
    	try{
    		stage->write("STY\r");//控制器急停指令
    	}catch(string e){
    		throw QException(OBJECT_NAME,"Stop",e);
    	}
	}
}

bool  Z1Stage::InOrigin()
{
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "ReturnOrigin", "No Stage Connection");
	}
	try{
		return ((int)stage->commandValue("MG _LRY") == 0);
	} catch (string e){
		throw QException(OBJECT_NAME, "InOrigin", e);
		return false;
	}
}

bool Z1Stage::ReturnOrigin()
{
	if (!IsConnected()){
		throw QException(OBJECT_NAME, "ReturnOrigin", "No Stage Connection");
	}

	int isOrigin = 1;
	try{
		while (true){
			isOrigin = (int)stage->commandValue("MG _LRY");//Z1 stage下限位！
			if (isOrigin == 0){
				break;
			}
			stage->command("IPY=-5000"); 
			double motionTime = Get_MotionTime(Z1_STAGE::Z1_SPEED, 5000);
			Sleep(long(2*motionTime));
		}
	} catch (string e){
		throw QException(OBJECT_NAME, "ReturnOrigin", e);
	}
	return true;
}

double Z1Stage::Get_CurrentPosition()
{
	if (!IsConnected()){
    	throw QException(OBJECT_NAME, "Get_CurrentPosition", "No Stage Connection");
	}
	else{
        try{
            return stage->commandValue("MG _TPY");
        }
        catch(string e){
            throw QException(OBJECT_NAME, "Get_CurrentPosition", e);
        }
    }
}

void Z1Stage::Set_CurrentPosition(double position)
{
    char strCommand[32];
    if (!IsConnected()){
    	throw QException(OBJECT_NAME, "Set_CurrentPosition", "No Stage Connection");
	}
	else{
    	try{
			sprintf(strCommand, "DEY=%ld", long(position));
    		stage->command(string(strCommand), "\r", ":", true);
    	}catch (string e){
        	throw QException(OBJECT_NAME, "Set_CurrentPosition", e);
    	}
	}
}

void Z1Stage::Set_Speed(double speed)
{
	char strCommand[32];
	if (!IsConnected()){
    	throw QException(OBJECT_NAME, "Set_Speed", "No Stage Connection");
	}
	else{
    	try{
    		sprintf(strCommand,"SPY=%ld", long(speed));
    		stage->command(string(strCommand), "\r", ":", true);
    	}catch(string e){
    		throw QException(OBJECT_NAME, "Set_Speed", e);
    	}
	}
}

//set acceleration speed
void Z1Stage::Set_ACCSpeed(double acc)
{
	char strCommand[32];
	if (!IsConnected()){
    	throw QException(OBJECT_NAME, "Set_ACCSpeed", "No Stage Connection");
	}
	else{
    	try{
    		sprintf(strCommand,"ACY=%ld",long(acc));
    		stage->command(string(strCommand), "\r", ":", true);
    	}catch(string e){
    		throw QException(OBJECT_NAME, "Set_ACCSpeed", e);
    	}
	}
}

//set deceleration speed
void Z1Stage::Set_DECSpeed(double dec)
{
	char strCommand[32];
	if (!IsConnected()){
    	throw QException(OBJECT_NAME, "Set_DECSpeed", "No Stage Connection");
	}
	else{
    	try{
    		sprintf(strCommand,"DCY=%ld",long(dec));
    		stage->command(string(strCommand), "\r", ":", true);
    	}catch(string e){
    		throw QException(OBJECT_NAME, "Set_DECSpeed", e);
    	}
	}
}

double Z1Stage::Get_Speed()
{
	if (!IsConnected()){
    	throw QException(OBJECT_NAME, "Get_Speed", "No Stage Connection");
	}
	else{
    	try{
    		return stage->commandValue("MG _SPY");
    	}catch (string e){
    		throw QException(OBJECT_NAME, "Get_Speed", e);
    	}
	}
}

double Z1Stage::Get_ACCSpeed()
{
	if (!IsConnected()){
    	throw QException(OBJECT_NAME, "Get_ACCSpeed", "No Stage Connection");
	}
	else{
    	try{
    		return stage->commandValue("MG _ACY");
    	}catch (string e){
    		throw QException(OBJECT_NAME, "Get_Speed", e);
    	}
	}
}

double Z1Stage::Get_DECSpeed()
{
	if (!IsConnected()){
    	throw QException(OBJECT_NAME, "Get_DECSpeed", "No Stage Connection");
	}
	else{
    	try{
    		return stage->commandValue("MG _DCY");
    	}catch (string e){
    		throw QException(OBJECT_NAME, "Get_Speed", e);
    	}
	}
}
