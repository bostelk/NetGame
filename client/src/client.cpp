#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../Shared/src/WinError.h"
#include "client.h"
#include <cassert>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512

int client_t::run(std::string address, std::string port)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    char recvbuf[DEFAULT_BUFLEN];

    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

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

    // Resolve the server address and port
    iResult = getaddrinfo(address.c_str(), port.c_str(), &hints, &result);
    if (iResult != 0) {
        WinError systemError;
        printf("getaddrinfo failed with error: %s\n", systemError.to_string().c_str());

        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        printf("Connecting to server...\n");

        connection = socket_connection_t(ConnectSocket);
        memcpy((uint8_t*)&connection.sockaddr, (uint8_t*)ptr->ai_addr, ptr->ai_addrlen); // Copy.
        connection.sockaddrlen = ptr->ai_addrlen;

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            WinError systemError;
            printf("Connection to server failed: %s\n%s\n", connection.get_address_name().c_str(), systemError.to_string().c_str());
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }

        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    printf("Connected! to server: %s\n", connection.get_address_name().c_str());

    packet_t packet = packet_t::from_string("hello world!");
    send_to_server(packet);
    packet.release();

    // Receive until the peer closes the connection
    do {
        ZeroMemory(recvbuf, 0, recvbuflen);

        SOCKADDR_STORAGE from;
        int fromlen = sizeof(SOCKADDR_STORAGE);

        int socket_type = connection.get_socket_type();
        switch (socket_type) {
        case SOCK_STREAM: {
            iResult = recv(connection.socket, recvbuf, recvbuflen, 0);
            break;
        }
        case SOCK_DGRAM: {
            iResult = recvfrom(connection.socket, recvbuf, recvbuflen, 0, (SOCKADDR*)&from, &fromlen);
            break;
        }
        default: {
            assert(false);
            break;
        }
        }
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);
            printf("Client received message: %s\n", recvbuf);
        }
        else if (iResult == 0)
            printf("Connection closed\n");
        else {
            WinError systemError;
            if (systemError.code == WSAECONNRESET)
            {
                iResult = 1; // Fake bytes received.
                // Ignore ICMP unreachable.
                continue;
            }
            printf("recv failed with error: %s\n", systemError.to_string().c_str());
        }
    } while (iResult > 0);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}

int client_t::send_to_server(packet_t packet)
{
    assert(packet.size_bytes < connection.get_max_size_bytes());
    int iResult;
    int socket_type = connection.get_socket_type();
    switch (socket_type) {
    case SOCK_STREAM: {
        iResult = send(connection.socket, (char*)packet.bytes, packet.size_bytes, 0);
        break;
    }
    case SOCK_DGRAM: {
        iResult = sendto(connection.socket, (char*)packet.bytes, packet.size_bytes, 0, (const SOCKADDR*)&connection.sockaddr, connection.sockaddrlen);
        break;
    }
    default: {
        assert(false);
        break;
    }
    }
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connection.socket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);
}
