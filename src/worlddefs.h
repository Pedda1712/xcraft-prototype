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

#define MAX_LIGHT 9
#define MIN_LIGHT 1

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

#include <pthread.h>
#include <pthread.h>
#include <dynamicarray.h>
#include <stdbool.h>

struct sync_chunk_t {
	int32_t _x;
	int32_t _z;
	
	/*
	 * data: all blocks EXCEPT water
	 * water: all blocks INCLUDING water // weird solution, but was easy to implement with existing code :P
	 * light: lightlevel at block
	 * light_temp: temporary buffer used for running light calculations
	 */
	struct chunk_t data;
	struct chunk_t water;
	struct chunk_t light;

	/*
		0 -> Chunk Mesh 
		1 -> Chunk Mesh (float buffer)
		2 -> Water Mesh
		3 -> Water Mesh (float buffer)
	 */
	bool rendermesh;
	struct DFA vertex_array[MESH_LEVELS]; 
	struct DFA texcrd_array[MESH_LEVELS];
	struct DFA lightl_array[MESH_LEVELS];
	
	pthread_mutex_t c_mutex;
	bool initialized;
};

#endif
