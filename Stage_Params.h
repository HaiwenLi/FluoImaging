
#ifndef _STAGE_PARAMETERS_H_
#define _STAGE_PARAMETERS_H_

#define AUTOFOCUS_INITIAL_POINT 10000000

namespace Z1_STAGE{
	const long STAGE_WAITING = 5;                         //等待时间 ms

	// Control and Motion Parameters
	const long Z1_SPEED = 5000;
	const double Z1_PRECISION = 0.05;                    //光栅尺精度：0.05um/pulse
	const double Z1_VERTICAL_TO_HORIZONTAL = 0.57; //Z1轴竖直与水平之间的比例
	const long Z1_TOL = 3;
	
	const long Z1_UPLIMIT = 158000;                          //Z1轴上限位, pulse
	const long Z1_INITIAL_REF_POSITION = 140000;  //Z1轴成像参考位置, pulse
	const long Z1_MAXSTEP = 1000;                            //单步最大步长, um

	// Focus Parameters
	const int Z1_COARSEFOCUS_TIMES = 15;           //粗对焦步数
	const int Z1_FINEFOCUS_TIMES = 9;                   //精对焦步数
	const int Z1_COARSEFOCUS_STEP = 5;               //粗对焦步长, um
	const int Z1_FINEFOCUS_STEP = 1;                     //精对焦步长, um
}

namespace Z3_STAGE{
	const long STAGE_WAITING = 5;                        //等待时间 ms

	// Control and Motion Parametrs
	const long Z3_SPEED = 5000;
	const double Z3_PRECISION = 0.05;                   //光栅尺精度：0.05um/pulse
	const long Z3_TOL = 3;

	// Focus Parameters
	const int Z3_COARSEFOCUS_TIMES = 15;           //粗对焦步数
	const int Z3_FINEFOCUS_TIMES = 9;                   //精对焦步数
	const int Z3_FINEFOCUS_STEP = 0;
}

#endif
