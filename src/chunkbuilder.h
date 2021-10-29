#ifndef CHUNKBUILDER
#define CHUNKBUILDER

#include <stdbool.h>
#include <worlddefs.h>

struct CLL* get_chunk_list (uint8_t l);

void chunk_data_sync  (struct sync_chunk_t* c);
void chunk_data_unsync(struct sync_chunk_t* c);

bool initialize_builder_thread ();
void terminate_builder_thread  ();

void do_updates_for_list (struct CLL* list);
void trigger_builder_update ();

#endif
