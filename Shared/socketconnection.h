#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

struct socket_connection_t
{
	SOCKET socket;
	SOCKADDR_STORAGE sockaddr;
	int sockaddrlen;

	socket_connection_t() :socket(INVALID_SOCKET) {}
	socket_connection_t(SOCKET socket) :socket(socket) {}

	size_t get_max_size_bytes();
	std::string get_address_name();
	int get_socket_type();
};