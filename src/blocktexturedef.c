#include <blocktexturedef.h>
#include <worlddefs.h>

struct blocktexdef_t btd_map [BLOCK_TYPE_COUNT] = {
	{{0, 0, 0, 0, 0, 0}}, // Air
	{{2, 0, 1, 1, 1, 1}}, // Grass
	{{2, 2, 2, 2, 2, 2}}, // Dirt
	{{3, 3, 3, 3, 3, 3}}, // Stone
	{{4, 4, 4, 4, 4, 4}}, // Gravel
	{{5, 5, 5, 5, 5, 5}}  // Water
};
