#pragma once
#include <string>

struct server_t
{
	server_t() {}

	int run(std::string address, std::string port);
};