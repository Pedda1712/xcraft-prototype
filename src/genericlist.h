#ifndef GENERICLSIT
#define GENERICLIST

#include <stdint.h>

struct GLL_element {
	void* data;
	struct GLL_element* next;
};

struct GLL {
	uint32_t size;
	struct GLL_element* first;
};

struct GLL GLL_init ();

void GLL_add (struct GLL* gll, void* _new);
void GLL_rem (struct GLL* gll, void* _rem);

void GLL_free (struct GLL* gll);
void GLL_free_rec (struct GLL* gll);

#endif
