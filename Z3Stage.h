/****************************************************************************
	Z1Stage: z3 axis stage
****************************************************************************/

#ifndef _Z3Stage_H_
#define _Z3Stage_H_

#include "Galil.h"
#include "Stage.h"
#include "Stage_Params.h"
#include "VirtualCoordinates.h"
using namespace VIRTUAL_COORDINATE;

class Z3Stage : public Stage
{
public:
	explicit Z3Stage(Galil* controller);
	~Z3Stage();

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
	bool ReturnOrigin();

	//Position acquisition
	double Get_CurrentPosition();
	void Set_CurrentPosition(double position);

	//Parameters settings and acquisition
	void Set_Speed(double speed);
	void Set_ACCSpeed(double acc);//set acceleration speed
	void Set_DECSpeed(double dec);//set deceleration speed
	double Get_Speed();
	double Get_ACCSpeed();
	double Get_DECSpeed();

protected:

private:
	DeviceStatus state;
	Galil* stage;
};

#endif

