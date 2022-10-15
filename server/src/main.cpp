#include "server.h"

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

    printf("Start server: %s:%s\n", address.c_str(), port.c_str());

	server_t server;
	int ret = server.run(address, port);

    printf("Stopped server: %d\n", ret);

    return ret;
}