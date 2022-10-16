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