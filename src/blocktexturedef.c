#include <blocktexturedef.h>
#include <worlddefs.h>
#include <stdio.h>
#include <stdlib.h>

struct blocktexdef_t btd_map [BLOCK_TYPE_COUNT] = { // Default BTD values
	{{255, 255, 255, 255, 255, 255}}, // Air
	{{2, 0, 1, 1, 1, 1}}, // Grass
	{{2, 2, 2, 2, 2, 2}}, // Dirt
	{{3, 3, 3, 3, 3, 3}}, // Stone
	{{4, 4, 4, 4, 4, 4}}, // Gravel
	{{5, 5, 5, 5, 5, 5}}  // Water
};

void loadblockdef (char* filename){
	FILE* btd_file = fopen (filename, "r");
	
	if (btd_file == NULL) {
		printf("Error opening BTD file, using defaults ... \n");
		return;
	}
	
	for(int i = 0; i < BLOCK_TYPE_COUNT; i++){
		char nums [6] [3];
		fscanf ( btd_file, "%s %s %s %s %s %s", nums[0], nums[1], nums[2], nums[3], nums[4], nums[5]);
		for(int k = 0; k < 6; k++){
			char *p;
			btd_map[i].index[k] =  (uint8_t) strtol( nums[k], &p, 10);
		}
		
	}
	
	fclose (btd_file);
}
