#ifndef WORLDTHREAD
#define WORLDTHREAD

#include <stdbool.h>
#include <worlddefs.h>
#include <pthread.h>
#include <dynamicarray.h>

struct sync_chunk_t {
	int32_t _x;
	int32_t _z;
	struct chunk_t data;
	
	// Chunk Mesh
	bool onmesh; // Which mesh the chunkbuilder writes into / which mesh ISNT rendered
	struct DFA vertex_array[2];
	struct DFA texcrd_array[2];
	struct DFA lightl_array[2];
	
	pthread_mutex_t c_mutex;
	bool initialized;
	bool render;
};

struct CLL* get_chunk_list (uint8_t l);
bool chunk_updating (struct sync_chunk_t* c);
void chunk_data_sync  (struct sync_chunk_t* c);

void chunk_data_unsync(struct sync_chunk_t* c);

void generate_world ();

bool initialize_chunk_thread ();
void start_chunk_thread ();
void terminate_chunk_thread ();

void trigger_chunk_update ();

#endif
