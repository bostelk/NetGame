#pragma once
#include <string>
#include <vector>
#include <winsock2.h> // For SOCKET.

struct socket_connection_t
{
	SOCKET socket;
	std::wstring address;
	socket_connection_t(SOCKET socket, std::wstring address) :socket(socket), address(address) {}
};

struct client_worker_t
{
	HANDLE thread;
	int threadId;
	socket_connection_t connection;

	client_worker_t() :thread(INVALID_HANDLE_VALUE), threadId(-1), connection({ INVALID_SOCKET, L""}) {}
	int do_work();
};

DWORD WINAPI client_thread_do_work(LPVOID lpParam);

struct server_t
{
	const int kMaxiumClientConnections = 10;
	std::vector<socket_connection_t> connections;
	std::vector<client_worker_t> client_workers;

	server_t() {}
	int run(std::string address, std::string port);
};