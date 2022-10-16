#include "packet.h"
#include <cstdint>
#include <cassert>
#include <stdlib.h>
#include <string.h>

void packet_t::alloc()
{
	assert(bytes == nullptr); // Allocated.
	bytes = (uint8_t*)malloc(size_bytes);
	memset(bytes, 0, size_bytes);
}

void packet_t::release()
{
	assert(bytes != nullptr); // Not allocated.
	free(bytes);
	bytes = nullptr;
}