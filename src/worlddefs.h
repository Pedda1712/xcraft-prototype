#ifndef WORLDDEFS
#define WORLDDEFS
#include <stdint.h>

#define WORLD_RANGE 12
#define NUMBER_CHUNKS ((WORLD_RANGE*2+1)*(WORLD_RANGE*2+1))

#define CHUNK_SIZE 16
#define CHUNK_SIZE_Y 128
#define CHUNK_LAYER (CHUNK_SIZE * CHUNK_SIZE)
#define CHUNK_MEM (CHUNK_LAYER * CHUNK_SIZE_Y * 2)

#define BLOCK_SIZE 1.0f

#define MESH_LEVELS 6

#define WATER_LEVEL 28
#define SGRASS_LEVEL 96
#define SNOW_LEVEL 112

#define WATER_SURFACE_OFFSET (1.0f/8.0f)

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
#define LEAVES_B 17
#define LOG_B    18

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
