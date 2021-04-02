#ifndef GENERATOR
#define GENERATOR

#include <stdbool.h>
#include <stdint.h>

struct chunkspace_position {
	int32_t _x;
	int32_t _z;
};

bool initialize_generator_thread ();
void terminate_generator_thread ();

void trigger_generator_update (struct chunkspace_position* pos);

#endif
