#pragma once
#include <string>
#include <vector>
#include <winsock2.h> // For SOCKET.
#include <queue>
#include <mutex>
#include <cassert>

struct socket_connection_t
{
	SOCKET socket;
	std::wstring address;
	socket_connection_t(SOCKET socket, std::wstring address) :socket(socket), address(address) {}
};

struct packet_t
{
	size_t kDefaultPacketSizeBytes = 512;

	uint8_t* bytes;
	size_t size_bytes;

	packet_t() : bytes(nullptr), size_bytes(kDefaultPacketSizeBytes) {}
	packet_t(size_t size_bytes) : bytes(nullptr), size_bytes(size_bytes) {}

	void alloc()
	{
		assert(bytes == nullptr); // Allocated.
		bytes = (uint8_t*)malloc(size_bytes);
		ZeroMemory(bytes, 0, size_bytes);
	}

	void release()
	{
		assert(bytes != nullptr); // Not allocated.
		free(bytes);
		bytes = nullptr;
	}
};

struct client_worker_t
{
	HANDLE thread;
	int threadId;
	socket_connection_t connection;

	std::queue<packet_t> sendQueue; // Todo switch to circle buffer.
	std::mutex sendMutex;

	client_worker_t() :thread(INVALID_HANDLE_VALUE), threadId(-1), connection({ INVALID_SOCKET, L""}) {}

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