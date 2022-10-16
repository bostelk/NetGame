#pragma once
#include <cstdint>

struct packet_t
{
	uint8_t* bytes;
	size_t size_bytes;

	packet_t(size_t size_bytes) : bytes(nullptr), size_bytes(size_bytes) {}

	void alloc();
	void release();
};
