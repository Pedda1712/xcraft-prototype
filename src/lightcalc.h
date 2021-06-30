#ifndef LIGHTCALC
#define LIGHTCALC

#include <chunkbuilder.h>

void skylight_func (void);
void calculate_light (struct sync_chunk_t* for_chunk, void (*calc_func)(void));
void init_light_calc ();

#endif
