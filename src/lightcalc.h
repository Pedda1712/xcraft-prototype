#ifndef LIGHTCALC
#define LIGHTCALC

#include <stdbool.h>
#include <chunkbuilder.h>

void skylight_func (struct sync_chunk_t**);
void blocklight_func (struct sync_chunk_t** );
void calculate_light (struct sync_chunk_t* for_chunk, void (*calc_func)(struct sync_chunk_t**), bool override);

#endif
