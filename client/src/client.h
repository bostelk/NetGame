#pragma once
#include <string>

struct client_t
{
	client_t() {}

	int run(std::string address, std::string port);
};