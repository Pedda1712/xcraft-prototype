#ifndef LIGHTCALC
#define LIGHTCALC

#include <chunkbuilder.h>

void skylight_func (struct CLL* list);
void calculate_light (struct sync_chunk_t* for_chunk, void (*calc_func)(struct CLL* list));

#endif
