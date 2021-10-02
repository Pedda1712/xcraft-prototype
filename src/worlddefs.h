#ifndef WORLDDEFS
#define WORLDDEFS
#include <stdint.h>

#define WORLD_RANGE 12
#define NUMBER_CHUNKS ((WORLD_RANGE*2+1)*(WORLD_RANGE*2+1))

#define CHUNK_SIZE 16
#define CHUNK_SIZE_Y 128
#define CHUNK_LAYER (CHUNK_SIZE * CHUNK_SIZE)
#define CHUNK_MEM (CHUNK_LAYER * CHUNK_SIZE_Y)

#define BLOCK_SIZE 1.0f

#define MESH_LEVELS 6

#define WATER_LEVEL 28
#define SGRASS_LEVEL 96
#define SNOW_LEVEL 112

#define WATER_SURFACE_OFFSET (1.0f/8.0f)

#define MAX_LIGHT 10
#define MIN_LIGHT 1

#define BLOCK_TYPE_COUNT 17

#define AIR_B    0
#define GRASS_B  1
#define DIRT_B   2
#define STONE_B  3
#define GRAVEL_B 4
#define WATER_B  5
#define LIGHT_B  6
#define SGRASS_B 7
#define SNOW_B   8   
#define XGRASS_B 9
#define XGRAST_B 10
#define XFLOW1_B 11
#define XFLOW2_B 12
#define XFLOW3_B 13
#define XFLOW4_B 14
#define XFLOW5_B 15
#define XFLOW6_B 16

#define ATCHUNK(x,z) ((x) + ((z) * (WORLD_RANGE*2+1)))
#define ATBLOCK(x,y,z) ((x) + ((z) * CHUNK_SIZE) + ((y) * CHUNK_LAYER))

struct chunkspace_position {
	int32_t _x;
	int32_t _z;
};

struct ipos3 {
	int _x;
	int _y;
	int _z;
};

struct chunk_t {
	uint8_t block_data [CHUNK_MEM];
};

#include <pthread.h>
#include <pthread.h>
#include <dynamicarray.h>
#include <stdbool.h>

#include <genericlist.h>

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
	struct chunk_t plant;
	struct chunk_t light;

	/*
		0 -> Chunk Mesh 
		1 -> Chunk Mesh (float buffer)
		2 -> Water Mesh
		3 -> Water Mesh (float buffer)
	 */
	
	bool updatemesh;
	bool vbo_update[MESH_LEVELS/2];
	uint32_t mesh_vbo [MESH_LEVELS/2];
	uint32_t verts [MESH_LEVELS/2];
	struct DFA mesh_buffer[MESH_LEVELS];
	
	/*
		List of all Light-Emitting Blocks
	 */
	struct GLL lightlist;
	
	pthread_mutex_t c_mutex;
	bool initialized;
};

#endif
