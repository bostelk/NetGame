#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#include "server.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../Shared/src/WinError.h"
#include <cassert>


// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512

int server_t::run(std::string address, std::string port)
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // UDP is connectionless.
    // hints.ai_socktype = SOCK_DGRAM;
    // hints.ai_protocol = IPPROTO_UDP;

    //hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(address.c_str(), port.c_str(), &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    char buffer[2048];
    inet_ntop(result->ai_family, result->ai_addr, buffer, 2048);
    printf("Bind socket: %s\n", buffer);

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {

        WinError systemError;
        printf("bind failed with error: %s\n", systemError.to_string().c_str());
        
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    while (true)
    {
        printf("Waiting for client...\n");

        sockaddr result;
        int resultLen = sizeof(sockaddr);

        // Accept a client socket
        ClientSocket = accept(ListenSocket, &result, &resultLen);
        if (ClientSocket == INVALID_SOCKET) {
            WinError systemError;
            printf("accept failed with error: %s\n", systemError.to_string().c_str());
            closesocket(ListenSocket);
            WSACleanup();
            continue; //return 1;
        }

        wchar_t buffer[2048];
        int bufferLen = 2048;
        int ret = WSAAddressToStringW(&result, resultLen, NULL, buffer, (LPDWORD) & bufferLen);
        assert(ret == 0);

        socket_connection_t connection(ClientSocket, std::wstring(buffer));
        connections.push_back(connection);

        auto worker = std::make_unique<client_worker_t>();
        worker->connection = connection;
        worker->thread = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size  
            client_thread_do_work,       // thread function name
            worker.get(),          // argument to thread function 
            0,                      // use default creation flags 
            (LPDWORD)&worker->threadId);   // returns the thread identifier 
        client_workers.push_back(std::move(worker));

        std::string message = std::string(connection.address.begin(), connection.address.end()) + " connected.";
        packet_t packet(message.size() + 1); // Add 1 for null terminator.
        packet.alloc();
        memcpy(packet.bytes, message.c_str(), packet.size_bytes);
        send_packet_to_clients(packet);
        packet.release();
    }

    // cleanup
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}

int server_t::send_packet_to_clients(packet_t packet)
{
    for (auto& worker : client_workers)
    {
        assert(packet.size_bytes < worker->connection.get_max_size_bytes());
        // send will block when there is no buffer space left within the transport system.
        int iSendResult = send(worker->connection.socket, (const char*)packet.bytes, packet.size_bytes, 0);
        if (iSendResult == SOCKET_ERROR) {
            printf("Client send failed with error: %d\n", WSAGetLastError());
            closesocket(worker->connection.socket);
            WSACleanup();
            CloseHandle(worker->thread);
            return 1;
        }
        else
        {
            printf("Client bytes sent: %d\n", iSendResult);
        }
    }
    return 0;
}

int client_worker_t::do_work()
{
    printf("Client %ls connected!\n", connection.address.c_str());

    int iResult;
    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

	// Receive until the peer shuts down the connection
	do {
		ZeroMemory(recvbuf, 0, recvbuflen);

		iResult = recv(connection.socket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Client bytes received: %d\n", iResult);
			printf("Server received message: %s\n", recvbuf);
		}
		else if (iResult == 0)
			printf("Client connection closing...\n");
		else {
			printf("Client recv failed with error: %d\n", WSAGetLastError());
			closesocket(connection.socket);
			WSACleanup();
			CloseHandle(thread);
			return 1;
		}
	} while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(connection.socket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("Client shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(connection.socket);
        WSACleanup();
        CloseHandle(thread);
        return 1;
    }

    CloseHandle(thread);
    return 0;
}

DWORD __stdcall client_thread_do_work(LPVOID lpParam)
{
    client_worker_t* worker = (client_worker_t*)lpParam;
    int ret = worker->do_work();
    return ret;
}
