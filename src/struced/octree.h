#ifndef OCTREE
#define OCTREE

#include <worlddefs.h>

struct block_t {
	int _x;
	int _y;
	int _z;
	uint16_t _type;
};

struct OCT_node_t {
	struct block_t* data;
	struct OCT_node_t* children [8];
};

struct OCT_node_t* OCT_alloc  (struct block_t);
bool OCT_check (struct OCT_node_t*, struct block_t);
void OCT_insert (struct OCT_node_t*, struct block_t);
struct OCT_node_t* OCT_remove (struct OCT_node_t*, struct block_t);

void OCT_operation (struct OCT_node_t*, void (*operator)(struct OCT_node_t*));
void OCT_operator_free (struct OCT_node_t*);
void OCT_operator_gl_draw (struct OCT_node_t*);

int OCT_count_levels (struct OCT_node_t*);

#endif
