/****************************************************************************
	Z1Stage: z1 axis stage
****************************************************************************/

#ifndef _Z1Stage_H_
#define _Z1Stage_H_

#include "Galil.h"
#include "Stage.h"
#include "Stage_Params.h"
#include "VirtualCoordinates.h"
using namespace VIRTUAL_COORDINATE;

class Z1Stage : public Stage
{
public:
	explicit Z1Stage(Galil* controller);
	~Z1Stage();

	static string DEVICE_NAME;
	static string OBJECT_NAME;

	//General control
	bool Connect();
	bool IsConnected();
	void Disconnect();

	//Motion control
	void Move_Openloop_Realtime(double distance);
	void Move_Openloop_Unrealtime(double distance);
	void Move_Closeloop_Realtime(double distance);
	void Move_Closeloop_Unrealtime(double distance);
	void Stop();
	bool ReturnOrigin(); //Returned state: true indicates reaching origin and false indicates stop
	bool InOrigin();

	//Position acquisition
	double Get_CurrentPosition();
    void Set_CurrentPosition(double position);
    
	//Parameters settings and acquisition
	void Set_Speed(double speed);
	void Set_ACCSpeed(double acc);//Set acceleration speed
	void Set_DECSpeed(double dec);//Set deceleration speed
	double Get_Speed();
	double Get_ACCSpeed();
	double Get_DECSpeed();

protected:

private:
	DeviceStatus state;
	Galil* stage;
};

#endif