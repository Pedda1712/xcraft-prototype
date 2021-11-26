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

void  GLL_add (struct GLL* gll, void* _new); // Appends New GLL_element with data _new at end of list
void  GLL_rem (struct GLL* gll, void* _rem); // Removes GLL_element when its data pointer is the same as _rem
void  GLL_push(struct GLL* gll, void* _new); // Appends New GLL_element with data _new at beginning of list
void* GLL_pop (struct GLL* gll); // Returns Data Pointer of first element, and removes it from the list

void GLL_free (struct GLL* gll); // First order Free -> only frees the data structure
void GLL_free_rec (struct GLL* gll); // Second order Free -> also calls free on the data pointers
void GLL_destroy (struct GLL* gll); // Destroys the List Mutex

void GLL_lock ( struct GLL* gll ); // Acquire List Mutex
void GLL_unlock ( struct GLL* gll ); // Give Up List Mutex
void GLL_operation (struct GLL* gll, void (*operator)(struct GLL_element*)); // Applies operator on every element of the list

#endif
