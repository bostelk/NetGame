#include <cstdio>
#include <string>
#include "client.h"

int __cdecl main(int argc, char** argv)
{
    // Validate the parameters
    if (argc != 2) {
        printf("usage: %s server-name port\n", argv[0]);
        return 1;
    }

    std::string address = std::string(argv[1]);
    std::string port = "27016";
    if (argc > 2) {
        port = std::string(argv[2]);
    }

    printf("Start client: %s:%s\n", address.c_str(), port.c_str());

    client_t client;
    int ret = client.run(address, port);

    printf("Stopped client: %d\n", ret);

    return ret;
}