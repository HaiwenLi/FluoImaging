/*
	serial 使用windows api进行串口编程
	其中，串口波特率可调，8个数据位，无奇偶位，一个停止位，设置读写串口时无延迟
*/
#include "serial.h"
#include "CharConvert.h"
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>
#include <iostream>

CSerialPort::CSerialPort()
{
	hComm = INVALID_HANDLE_VALUE;
}

CSerialPort::~CSerialPort()
{
	ClosePort();
}

int CSerialPort::Init_Com(char *devname, UINT baud_rate)
{
	//调用windows api打开，连接串口
	WCHAR portName[MAX_PORT_NAME_LENGTH];
	memset(portName, 0, sizeof(portName));
	C2W(devname, portName, sizeof(portName)/sizeof(portName[0]));

    hComm = CreateFile(portName,	    // communication port string (COMX)
		GENERIC_READ | GENERIC_WRITE,	// read/write types
		0,								// comm devices must be opened with exclusive access
		NULL,						    // no security attributes
		OPEN_EXISTING,					// comm devices must use OPEN_EXISTING
		0,		                        // sync I/O
		NULL);							// template must be 0 for comm devices
	//wcout << portName << endl;
	SetupComm(hComm, 256, 256);//初始化缓冲区大小

	//如果串口打开失败则返回连接错误
	if (hComm == INVALID_HANDLE_VALUE)
	{
		std::cout << "CSerialPort::Init_Com() : cannot open serial port" << std::endl;
		return SERIAL_OPEN_PORT_ERROR;
	}
	sprintf_s(portname, "%s", devname);

	//设置串口的超时时间
	//由于设置间隔时间为MAXDWORD，而读时间系数和读时间常量都为0，
	//则在读一次输入缓冲区的内容后读操作就立即返回，而不管是否读入了要求的字符
	comTimeout.ReadIntervalTimeout = MAXDWORD;
	comTimeout.ReadTotalTimeoutMultiplier = 0;
	comTimeout.ReadTotalTimeoutConstant = 0;
	comTimeout.WriteTotalTimeoutMultiplier = 0;
	comTimeout.WriteTotalTimeoutConstant = 0;
	if (!SetCommTimeouts(hComm, &comTimeout))
	{ 
		ProcessErrorMessage("SetCommTimeouts()");
		std::cout << "CSerialPort::Init_Com() : cannot set port\'s parameters" << std::endl;
		return SERIAL_PARAMS_SETTING_ERROR;
	}

	//设置串口的数据位，奇偶位，停止位和波特率：有8个数据位，无奇偶位，一个停止位
	if (GetCommState(hComm, &dcb))
	{
		dcb.BaudRate = baud_rate;
		dcb.ByteSize = 8;
		dcb.StopBits = ONESTOPBIT;
		dcb.Parity = NOPARITY;
	}
	else
	{
		ProcessErrorMessage("GetCommState()");
		std::cout << "CSerialPort::Init_Com() : cannot set port\'s parameters" << std::endl;
		return SERIAL_PARAMS_SETTING_ERROR;
	}
	if (!SetCommState(hComm, &dcb)){
		ProcessErrorMessage("SetCommState()");
		std::cout << "CSerialPort::Init_Com() : cannot set port\'s parameters" << std::endl;
		return SERIAL_PARAMS_SETTING_ERROR;
	}

	PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);//flush port
	return SERIAL_CONNECTION_SUCCESS;
}

void CSerialPort::ClosePort()
{
	if (hComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hComm);
		hComm = INVALID_HANDLE_VALUE;
	}
}

//从串口读取数据
int CSerialPort::Receive_Data()
{
	DWORD readBytes = 0;
	memset(readBuffer, 0, sizeof(readBuffer)/sizeof(readBuffer[0]));
	if (!ReadFile(hComm, readBuffer, sizeof(readBuffer), &readBytes, NULL)){
		ProcessErrorMessage("Receive_Data()");
	}
	PurgeComm(hComm, PURGE_RXABORT | PURGE_RXCLEAR);
	return readBytes;
}

//向串口发送命令
void CSerialPort::Write_Data(char *command)
{
	DWORD dwError;
	COMSTAT commstat;
	DWORD dwWrittenBytes;//实际写串口的字符数
	DWORD writeSize;     //要写串口的字符数

	memset(writeBuffer, 0, sizeof(writeBuffer)/sizeof(writeBuffer[0]));//清空写数据缓冲区writeBuffer
	writeSize = strlen(command);
	strncpy(writeBuffer, command, sizeof(writeBuffer) / sizeof(writeBuffer[0]));
	ClearCommError(hComm, &dwError, &commstat);
	if (!WriteFile(hComm, writeBuffer, writeSize, &dwWrittenBytes, NULL)){	
		ProcessErrorMessage("Write_Data()");
	}
	PurgeComm(hComm, PURGE_TXABORT | PURGE_TXCLEAR);
}

//清空readBuffer
void CSerialPort::ClearReadBuffer()
{
	memset(readBuffer, 0, sizeof(readBuffer) / sizeof(readBuffer[0]));
}

//清空writeBuffer
void CSerialPort::ClearWriteBuffer()
{
	memset(writeBuffer, 0, sizeof(writeBuffer) / sizeof(writeBuffer[0]));
}

void CSerialPort::ProcessErrorMessage(char* ErrorText)
{
	char error_text[MAX_ERROR_BUFFER_SIZE];
	sprintf(error_text, "ERROR:  %s Failed , port: %s\n",ErrorText, portname);
}

void CSerialPort::flush()
{
	PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);//flush port
}