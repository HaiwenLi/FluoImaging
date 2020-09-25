/****************************************************************************
    DevicesVirtualCoordinates  规定了系统的虚拟坐标系，该坐标系规定：
	1、从下至上，垂直培养皿的方向为Z轴正方向
	2、在培养皿平面看，从后向前为X轴正方向（左右前后是站在目前系统前方而言的）
	3、在培养皿平面看，从左至右为Y轴正方向	
	注：虚拟坐标系中的变化矩阵均是对于正方向而言的
	
	二代系统中设备与虚拟坐标的关系：
	Z1(+) <-> Virtual Coordinate(Z+)
	Z3(+) <-> Virtual Coordinate(Z+)
****************************************************************************/
#ifndef _DEVICES_VIRTUAL_COORDINATES_H_
#define _DEVICES_VIRTUAL_COORDINATES_H_

#define STAGE_INITIAL_VALUE 10000000 //电机初始值

namespace VIRTUAL_COORDINATE{
	const int X_POSITIVE = 1;//虚拟坐标系X正轴
	const int Y_POSITIVE = 1;//虚拟坐标系Y正轴
	const int Z_POSITIVE = 1;//虚拟坐标系Z正轴
	
    const int Z1_POSITIVE = Z_POSITIVE;
    const int Z3_POSITIVE = Z_POSITIVE;
}

#endif
