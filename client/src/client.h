#pragma once
#include <string>
#include "../../Shared/socketconnection.h"
#include "../../Shared/packet.h"

struct client_t
{
	socket_connection_t connection;

	client_t() {}

	int run(std::string address, std::string port);
	int send_to_server(packet_t packet);
};