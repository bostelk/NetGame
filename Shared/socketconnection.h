#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

struct socket_connection_t
{
	SOCKET socket;
	std::wstring address;

	socket_connection_t() :socket(INVALID_SOCKET), address(L"") {}
	socket_connection_t(SOCKET socket, std::wstring address) :socket(socket), address(address) {}

	size_t get_max_size_bytes();
};