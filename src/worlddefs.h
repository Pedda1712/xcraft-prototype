#ifndef WORLDDEFS
#define WORLDDEFS
#include <stdint.h>

#define WORLD_RANGE 16
#define NUMBER_CHUNKS ((WORLD_RANGE*2+1)*(WORLD_RANGE*2+1))

#define CHUNK_SIZE 16
#define CHUNK_SIZE_Y 160
#define CHUNK_LAYER (CHUNK_SIZE * CHUNK_SIZE)
#define CHUNK_MEM (CHUNK_LAYER * CHUNK_SIZE_Y * 2)

#define MESH_LEVELS 6

#define WATER_LEVEL 28
#define SGRASS_LEVEL 96
#define SNOW_LEVEL 112

#define WATER_SURFACE_OFFSET (1.0f/8.0f)

#define SMOOTH_LIGHTING_ENABLED 1

#define MAX_LIGHT 15
#define MIN_LIGHT 1

#define BLOCK_TYPE_COUNT 19

#define BLOCK_ID(x) ((uint8_t)((x) & 0xFF))
#define SOLID_FLAG  (1 << 8)
#define X_FLAG      (2 << 8)
#define TRANS_FLAG  (3 << 8)
#define P_FLAG      (4 << 8)
#define IS_SOLID(x) (((x) >> 8) == 1)
#define IS_X(x)     (((x) >> 8) == 2)
#define IS_P(x)		(((x) >> 8) == 4)
#define IS_TRANS(x) (((x) >> 8) == 3)

enum BLOCK_INDICES {
	AIR_B=0, 
	GRASS_B, 
	DIRT_B, 
	STONE_B, 
	GRAVEL_B, 
	WATER_B, 
	LIGHT_B, 
	SGRASS_B, 
	SNOW_B, 
	XGRASS_B, 
	XGRAST_B, 
	XFLOW1_B, 
	XFLOW2_B,
	XFLOW3_B, 
	XFLOW4_B, 
	XFLOW5_B, 
	XFLOW6_B, 
	LEAVES_B, 
	LOG_B
};

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
	/*
		bits 0 to 7 are the block id
		bits 8 to 15 is the flag register for possible block types (solid block, x mesh)
	 */
	uint16_t block_data [CHUNK_MEM];
};

#include <pthread.h>
#include <dynamicarray.h>
#include <stdbool.h>

#include <genericlist.h>

struct sync_chunk_t {
	int32_t _x;
	int32_t _z;

	/*
	 * data_unique: all blocks INCLUDING water AND plants // weird solution, but was easy to implement with existing code :P
	 * light: lightlevel at block
	 */
	struct chunk_t data_unique;
	struct chunk_t light;

	bool updatemesh;
	bool vbo_update[MESH_LEVELS/2]; // Flags
	uint32_t mesh_vbo [MESH_LEVELS/2]; // VBO Objects
	uint32_t verts [MESH_LEVELS/2]; // Vertex Count
	struct DFA mesh_buffer[MESH_LEVELS]; // Raw Triangle Information

	/*
		List of all Light-Emitting Blocks
	 */
	struct GLL lightlist;

	pthread_mutex_t c_mutex;
	bool initialized;
};

#endif
