#pragma once
#include <string>
#include <vector>
#include <winsock2.h> // For SOCKET.
#include <queue>
#include <mutex>
#include <cassert>
#include "../../Shared/packet.h"
#include "../../Shared/socketconnection.h"


struct client_worker_t
{
	HANDLE thread;
	int threadId;
	socket_connection_t connection;

	client_worker_t() :thread(INVALID_HANDLE_VALUE), threadId(-1), connection({ INVALID_SOCKET }) {}

	int do_work();
};

DWORD WINAPI client_thread_do_work(LPVOID lpParam);

struct server_t
{
	const int kMaxiumClientConnections = 10;
	std::vector<socket_connection_t> connections;
	std::vector<std::unique_ptr<client_worker_t>> client_workers;

	server_t() {}
	int run(std::string address, std::string port);
	int send_packet_to_clients(packet_t packet);
};