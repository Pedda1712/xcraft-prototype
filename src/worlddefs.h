#ifndef WORLDDEFS
#define WORLDDEFS
#include <stdint.h>

#define WORLD_RANGE 8
#define NUMBER_CHUNKS ((WORLD_RANGE*2+1)*(WORLD_RANGE*2+1))

#define CHUNK_SIZE 16
#define CHUNK_SIZE_Y 128
#define CHUNK_LAYER (CHUNK_SIZE * CHUNK_SIZE)
#define CHUNK_MEM (CHUNK_LAYER * CHUNK_SIZE_Y)

#define BLOCK_SIZE 1.0f

#define MESH_LEVELS 4
#define WATER_LEVEL 44

#define WATER_SURFACE_OFFSET (1.0f/8.0f)

#define MAX_LIGHT 12

#define BLOCK_TYPE_COUNT 6

#define AIR_B    0
#define GRASS_B  1
#define DIRT_B   2
#define STONE_B  3
#define GRAVEL_B 4
#define WATER_B  5

#define ATCHUNK(x,z) ((x) + ((z) * (WORLD_RANGE*2+1)))
#define ATBLOCK(x,y,z) ((x) + ((z) * CHUNK_SIZE) + ((y) * CHUNK_LAYER))

struct chunkspace_position {
	int32_t _x;
	int32_t _z;
};

struct chunk_t {
	uint8_t block_data [CHUNK_MEM];
};

#endif
