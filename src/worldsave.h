#ifndef WORLDSAVE
#define WORLDSAVE

#define MAX_WORLDNAME_LENGTH 50

#include <stdbool.h>
#include <worlddefs.h>

void set_world_name (char* w_name);

void dump_chunk (struct sync_chunk_t* c);

bool read_chunk (struct sync_chunk_t* c);

#endif
