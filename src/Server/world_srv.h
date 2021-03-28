#ifndef WORLD_SRV
#define WORLD_SRV

#include <stdint.h>
#include <stdbool.h>

#include <worlddefs.h>


bool initialize_world ();
struct chunk_t* chunk_data(int32_t x, int32_t z);

#endif
