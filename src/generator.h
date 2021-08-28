#ifndef GENERATOR
#define GENERATOR

#include <stdbool.h>
#include <stdint.h>

#include <worlddefs.h>

bool initialize_generator_thread ();
void terminate_generator_thread ();

void run_chunk_generation (struct chunkspace_position* pos);
void run_chunk_generation_atprev ();
void trigger_generator_update (struct chunkspace_position* pos);

#endif
