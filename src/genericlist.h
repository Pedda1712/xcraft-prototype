#ifndef GENERICLIST
#define GENERICLIST

#include <stdint.h>
#include <pthread.h>

struct GLL_element {
	void* data;
	struct GLL_element* next;
};

struct GLL {
	uint32_t size;
	struct GLL_element* first;
	pthread_mutex_t mutex;
};

struct GLL GLL_init ();

void GLL_add (struct GLL* gll, void* _new);
void GLL_rem (struct GLL* gll, void* _rem);

void GLL_free (struct GLL* gll);
void GLL_free_rec (struct GLL* gll);
void GLL_destroy (struct GLL* gll);

void GLL_lock ( struct GLL* gll );
void GLL_unlock ( struct GLL* gll );
void GLL_operation (struct GLL* gll, void (*operator)(struct GLL_element*));

#endif
