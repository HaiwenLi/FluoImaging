/****************************************************************************
	serial 主要用于串口通信，使用windows api进行串口编程
	其中，串口波特率可调，8个数据位，无奇偶位，一个停止位，设置读写串口时无延迟
	日期：2015-03-15
****************************************************************************/
#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <Windows.h>
#define MAX_PORT_NAME_LENGTH 16          //串口大小
#define MAX_BUFFER_SIZE 256              //有关串口数据发送，接收缓冲区大小
#define MAX_ERROR_BUFFER_SIZE 1024       //有关串口错误信息缓冲区大小
#define SERIAL_ISBUSY_DELAY 5            //查询串口是否Busy延迟 ms
#define SERIAL_QUERY_DELAY 50            //查询串口状态延迟 ms

#define SERIAL_CONNECTION_SUCCESS    0 //成功打开串口
#define SERIAL_OPEN_PORT_ERROR      -1 //打开串口错误
#define SERIAL_PARAMS_SETTING_ERROR -2 //设置串口参数错误

class CSerialPort
{
public:
	CSerialPort();
	~CSerialPort();

	char portname[MAX_PORT_NAME_LENGTH];
	HANDLE hComm;//串口
	DCB dcb;     //包含了串口的各项参数设置，使用该结构配置串口
	COMMTIMEOUTS comTimeout;
	char readBuffer[MAX_BUFFER_SIZE];

	int Init_Com(char *devname, UINT baud_rate);//初始化com口，只需要配置波特率
	void ClosePort();
	void Write_Data(char *command);
	int Receive_Data();
	void flush();
	void ClearReadBuffer();
	void ProcessErrorMessage(char *);

private:
	char writeBuffer[MAX_BUFFER_SIZE];
	void ClearWriteBuffer();
};

#endif
