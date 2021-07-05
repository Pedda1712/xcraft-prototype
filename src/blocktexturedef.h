#ifndef BLOCKTEXTUREDEF
#define BLOCKTEXTUREDEF

#include <worlddefs.h>

struct blocktexdef_t {
	/*
		0 -> Y-
		1 -> Y+
		2 -> X-
		3 -> X+
		4 -> Z-
		5 -> Z+
		
	 */
	uint8_t index [6];
};

extern struct blocktexdef_t btd_map [BLOCK_TYPE_COUNT];

#define MAX_BLOCKNAME_LENGTH 50
extern char blockname_map [BLOCK_TYPE_COUNT] [MAX_BLOCKNAME_LENGTH];

void loadblockdef (char* filename);

#endif
