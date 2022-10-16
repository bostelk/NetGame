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

    int DefaultSocketType = SOCK_DGRAM;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = DefaultSocketType;
    switch (hints.ai_socktype)
    {
    case SOCK_STREAM:
        hints.ai_protocol = IPPROTO_TCP;
        break;
    case SOCK_DGRAM:
        hints.ai_protocol = IPPROTO_UDP;
        break;
    default:
        assert(false);
        break;
    }
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

    if (DefaultSocketType == SOCK_STREAM)
    {
        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            printf("listen failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
    }

    while (true)
    {
        if (DefaultSocketType == SOCK_STREAM)
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

            socket_connection_t connection(ClientSocket);
            memcpy((uint8_t*)&connection.sockaddr,(uint8_t*) &result, resultLen); // Copy.
            connection.sockaddrlen = resultLen;
            connections.push_back(connection);

            auto worker = std::make_unique<server_worker_t>();
            worker->connection = connection;
            worker->thread = CreateThread(
                NULL,                   // default security attributes
                0,                      // use default stack size  
                server_thread_do_work,       // thread function name
                worker.get(),          // argument to thread function 
                0,                      // use default creation flags 
                (LPDWORD)&worker->threadId);   // returns the thread identifier 
            server_workers.push_back(std::move(worker));

            packet_t packet = packet_t::from_string(connection.get_address_name() + " connected.");
            send_packet_to_clients(packet);
            packet.release();
        }
        // UDP is connectionless.
        else if (DefaultSocketType == SOCK_DGRAM)
        {
            SOCKADDR_STORAGE from;
            int fromlen = sizeof(SOCKADDR_STORAGE);
            int ret;
            char recvbuf[DEFAULT_BUFLEN];
            int recvbuflen = DEFAULT_BUFLEN;

            socket_connection_t connection(ListenSocket);

            iResult = recvfrom(ListenSocket, recvbuf, recvbuflen, 0, (SOCKADDR*)&from, &fromlen);
            if (iResult == SOCKET_ERROR)
            {
                WinError systemError;
                printf("recvfrom socket error: %s\n", systemError.to_string().c_str());
                continue;
            }
            else
            {
                memcpy((uint8_t*)&connection.sockaddr, (uint8_t*)&from, fromlen);
                connection.sockaddrlen = fromlen;

                printf("Client bytes received: %d\n", iResult);
                printf("Server received message: %s from %s\n", recvbuf, connection.get_address_name().c_str());
            }
        }
        else
        {
            assert(false);
        }
    }

    // cleanup
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}

int server_t::send_packet_to_clients(packet_t packet)
{
    for (auto& worker : server_workers)
    {
        assert(packet.size_bytes < worker->connection.get_max_size_bytes());
        // send will block when there is no buffer space left within the transport system.
        int iSendResult;
        int socket_type = worker->connection.get_socket_type();
        switch (socket_type) {
        case SOCK_STREAM: {
            iSendResult = send(worker->connection.socket, (char*)packet.bytes, packet.size_bytes, 0);
            break;
        }
        case SOCK_DGRAM: {
            iSendResult = sendto(worker->connection.socket, (char*)packet.bytes, packet.size_bytes, 0, (const SOCKADDR*)&worker->connection.sockaddr, worker->connection.sockaddrlen);
            break;
        }
        default: {
            assert(false);
            break;
        }
        }
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

int server_worker_t::do_work()
{
    printf("Client %s connected!\n", connection.get_address_name().c_str());

    int iResult;
    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

	// Receive until the peer shuts down the connection
	do {
		ZeroMemory(recvbuf, 0, recvbuflen);

        int socket_type = connection.get_socket_type();
        switch (socket_type) {
        case SOCK_STREAM: {
            iResult = recv(connection.socket, recvbuf, recvbuflen, 0);
            break;
        }
        case SOCK_DGRAM: {
            iResult = recvfrom(connection.socket, recvbuf, recvbuflen, 0, (SOCKADDR*)&connection.sockaddr, &connection.sockaddrlen);
            break;
        }
        default: {
            assert(false);
            break;
        }
        }
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

DWORD __stdcall server_thread_do_work(LPVOID lpParam)
{
    server_worker_t* worker = (server_worker_t*)lpParam;
    int ret = worker->do_work();
    return ret;
}
