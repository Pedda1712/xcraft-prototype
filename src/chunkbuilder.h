#ifndef CHUNKBUILDER
#define CHUNKBUILDER

#include <stdbool.h>
#include <worlddefs.h>
#include <pthread.h>
#include <dynamicarray.h>

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
	struct chunk_t light_temp;

	/*
		0 -> Chunk Mesh 
		1 -> Chunk Mesh (double buffer)
		2 -> Water Mesh
		3 -> Water Mesh (double buffer)
	 */
	bool rendermesh;
	struct DFA vertex_array[MESH_LEVELS]; 
	struct DFA texcrd_array[MESH_LEVELS];
	struct DFA lightl_array[MESH_LEVELS];
	
	pthread_mutex_t c_mutex;
	bool initialized;
};

struct CLL* get_chunk_list (uint8_t l);
bool chunk_updating (struct sync_chunk_t* c);
void chunk_data_sync  (struct sync_chunk_t* c);

void chunk_data_unsync(struct sync_chunk_t* c);

bool initialize_chunk_thread ();
void start_chunk_thread ();
void terminate_chunk_thread ();

void trigger_chunk_update ();

#endif
