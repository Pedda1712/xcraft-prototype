#ifndef WORLDDEFS
#define WORLDDEFS

#include <stdint.h>

#define WORLD_SIZE_X 16
#define WORLD_SIZE_Z 16

#define CHUNK_SIZE_X 16
#define CHUNK_SIZE_Z 16
#define CHUNK_SIZE_Y 128
#define CHUNK_LAYER (CHUNK_SIZE_X * CHUNK_SIZE_Z)
#define CHUNK_MEM (CHUNK_LAYER * CHUNK_SIZE_Y)

struct chunk_t {
	uint8_t block_data [CHUNK_MEM];
};

#endif
