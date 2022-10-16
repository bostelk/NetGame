#include "socketconnection.h"
#include <cassert>


size_t socket_connection_t::get_max_size_bytes()
{
    char buffer[sizeof(int)];
    int bufferLen = sizeof(int);
    int ret = getsockopt(socket, SOL_SOCKET, SO_MAX_MSG_SIZE, buffer, &bufferLen);
    assert(ret == 0);
    int value = buffer[0];
    if (value == -1) { // No message size limit.
        return std::numeric_limits<size_t>::max();
    }
    return value;
}

std::string socket_connection_t::get_address_name()
{
    wchar_t buffer[2048];
    int bufferLen = 2048;
    int ret = WSAAddressToStringW((SOCKADDR*)&sockaddr, sockaddrlen, NULL, buffer, (LPDWORD)&bufferLen);
    assert(ret == 0);
    std::wstring name(buffer, bufferLen);
    return std::string(name.begin(), name.end());
}

int socket_connection_t::get_socket_type()
{
    int optval = -1;
    int optvallen = sizeof(int);
    int ret = getsockopt(socket, SOL_SOCKET, SO_TYPE, (char*)&optval, &optvallen);
    assert(ret == 0);
    return optval;
}