#ifndef CHUNKLIST
#define CHUNKLIST

#include <stdint.h>
#include <chunkbuilder.h>
#include <pthread.h>

struct CLL_element {
	struct sync_chunk_t* data;
	struct CLL_element* nxt;
};

struct CLL {
	uint32_t size;
	struct CLL_element* first;
	pthread_mutex_t mutex;
};

void lock_list (struct CLL* list);
void unlock_list (struct CLL* list);

void CLL_copyList (struct CLL* src, struct CLL* dest);

struct CLL CLL_init ();
void CLL_add  (struct CLL* list ,struct sync_chunk_t* data);
void CLL_freeList (struct CLL* list);
void CLL_freeListAndData (struct CLL* list);
void CLL_destroyList (struct CLL* list);

struct sync_chunk_t* CLL_getDataAt (struct CLL* list, int32_t x, int32_t z);

#endif
