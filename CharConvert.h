/****************************************************************************
	CharConvert 用于宽字符与ASCII码之间转化
	日期：2015-03-15
****************************************************************************/
#ifndef _CHARCONVERT_
#define _CHARCONVERT_

#include <Windows.h>

//LPCWSTR为wchar_t *
void inline C2W(const char* str,  wchar_t * wstr, int len)
{
	//len为wstr所指向的缓冲区大小
	MultiByteToWideChar(CP_ACP, 0, str, strlen(str) + 1, wstr, len);
}
//len 表示str指向缓冲区的大小
void inline W2C(LPCWSTR wstr, char* str, int len)
{
	WideCharToMultiByte(CP_ACP, 0, wstr, -1/*null terminated*/, str, len, NULL, NULL);
}

#endif