#ifndef GENERATOR
#define GENERATOR

#include <stdbool.h>
#include <stdint.h>

#include <worlddefs.h>

bool initialize_generator_thread ();
void terminate_generator_thread ();

void trigger_generator_update (struct chunkspace_position* pos);

#endif
