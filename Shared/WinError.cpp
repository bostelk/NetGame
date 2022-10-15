#define WIN32_LEAN_AND_MEAN

#include "WinError.h"
#include <windows.h>
#include <winsock2.h>
#include <cassert>

WinError::WinError()
{
	code = WSAGetLastError();
}

std::string WinError::GetMessageA()
{
	char buffer[2048];
	int ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, NULL, buffer, 2048, NULL);
	assert(ret > 0);

	return std::string(buffer);
}

std::string WinError::to_string()
{
	return "(" + std::to_string(code) + ") " + GetMessageA();
}
