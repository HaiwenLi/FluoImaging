/****************************************************************************
	Stage: Abstract class which defines the operations of a stage
****************************************************************************/

#ifndef _STAGE_H_
#define _STAGE_H_

#include "Util.h"
#include "QException.h"

class Stage{
public:
	Stage():state(DISCONNECTED){}

	//General control
	virtual bool Connect() = 0;
	virtual bool IsConnected() = 0;
	virtual void Disconnect() = 0;
	inline DeviceStatus GetCurrentState() { return state;}

	//Motion control
	virtual void Move_Openloop_Realtime(double distance) = 0;
	virtual void Move_Openloop_Unrealtime(double distance) = 0;
	virtual void Move_Closeloop_Realtime(double distance) = 0;
	virtual void Move_Closeloop_Unrealtime(double distance) = 0;
	virtual void Stop() = 0;
	virtual bool ReturnOrigin() = 0;

	//Position acquisition
	virtual double Get_CurrentPosition() = 0;
	virtual void Set_CurrentPosition(double position) = 0;

	//Parameters getting and setting
	virtual void Set_Speed(double speed) = 0;
	virtual void Set_ACCSpeed(double acc) = 0; //set acceleration speed
	virtual void Set_DECSpeed(double dec) = 0; //set deceleration speed
	virtual double Get_Speed() = 0;
	virtual double Get_ACCSpeed() = 0;
	virtual double Get_DECSpeed() = 0;

private:
	DeviceStatus state;
};

#endif