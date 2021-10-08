#ifndef WORLDSAVE
#define WORLDSAVE

#define MAX_WORLDNAME_LENGTH 50

#include <stdbool.h>
#include <worlddefs.h>

void set_world_name (char* w_name);
void delete_current_world ();
void dump_seed ();
void read_seed ();

void dump_chunk (struct sync_chunk_t* c);
void dump_player_data ();

bool read_chunk (struct sync_chunk_t* c);
void read_player_data ();

#endif
