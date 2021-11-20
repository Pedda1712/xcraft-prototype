#ifndef WORLDSAVE
#define WORLDSAVE

#define MAX_WORLDNAME_LENGTH 50

#define STRUCTURE_CACHE_SIZE 1

#include <stdbool.h>

#include <worlddefs.h>
#include <genericlist.h>

struct structure_t {
	struct ipos3 base;
	uint32_t index;
};

struct structure_log_t {
	int32_t _x;
	int32_t _z;
	struct GLL blocks;
};

extern struct GLL structure_cache [STRUCTURE_CACHE_SIZE];

void set_world_name (char* w_name);
void delete_current_world ();
void dump_seed ();
void read_seed ();

void dump_chunk (struct sync_chunk_t* c);
void dump_player_data ();

bool read_chunk (struct sync_chunk_t* c);
void read_player_data ();

void init_structure_cache ();
void clean_structure_cache();

void read_structure_file_into_gll (char* fname, struct GLL* gll);

void save_structure_log_into_file (struct structure_log_t* log);
bool read_structure_log_into_gll (int x, int z, struct GLL* gll);

#endif
