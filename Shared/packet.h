#pragma once
#include <cstdint>
#include <string>

struct packet_t
{
	uint8_t* bytes;
	size_t size_bytes;

	packet_t(size_t size_bytes) : bytes(nullptr), size_bytes(size_bytes) {}

	void alloc();
	void release();

	// Caller must release.
	static packet_t from_string(std::string message);
};
