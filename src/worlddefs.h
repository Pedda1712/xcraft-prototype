#ifndef WORLDDEFS
#define WORLDDEFS

#include <stdint.h>

#define WORLD_RANGE 12 
#define NUMBER_CHUNKS ((WORLD_RANGE*2+1)*(WORLD_RANGE*2+1))

#define CHUNK_SIZE_X 16
#define CHUNK_SIZE_Z 16
#define CHUNK_SIZE_Y 128
#define CHUNK_LAYER (CHUNK_SIZE_X * CHUNK_SIZE_Z)
#define CHUNK_MEM (CHUNK_LAYER * CHUNK_SIZE_Y)

#define BLOCK_SIZE 1.0f

#define MESH_LEVELS 2
#define WATER_LEVEL 48

#define GRASS_B  1
#define DIRT_B   2
#define STONE_B  3
#define GRAVEL_B 4
#define WATER_B  5

#define ATCHUNK(x,z) ((x) + ((z) * (WORLD_RANGE*2+1)))
#define ATBLOCK(x,y,z) ((x) + ((z) * CHUNK_SIZE_X) + ((y) * CHUNK_LAYER))

struct chunkspace_position {
	int32_t _x;
	int32_t _z;
};

struct chunk_t {
	uint8_t block_data [CHUNK_MEM];
};

#endif
