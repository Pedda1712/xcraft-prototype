#include <blocktexturedef.h>
#include <worlddefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct blocktexdef_t btd_map [BLOCK_TYPE_COUNT] = { // Default BTD values
	{{255, 255, 255, 255, 255, 255}}, // Air
	{{2, 0, 1, 1, 1, 1}}, // Grass
	{{2, 2, 2, 2, 2, 2}}, // Dirt
	{{3, 3, 3, 3, 3, 3}}, // Stone
	{{4, 4, 4, 4, 4, 4}}, // Gravel
	{{5, 5, 5, 5, 5, 5}}, // Water
	{{6, 6, 6, 6, 6, 6}}  // Light
};

char blockname_map [BLOCK_TYPE_COUNT] [MAX_BLOCKNAME_LENGTH] = {
	"Air",
	"Grass",
	"Dirt",
	"Stone",
	"Gravel",
	"Water",
	"Light Source"
};

void loadblockdef (char* filename){
	FILE* btd_file = fopen (filename, "r");
	
	if (btd_file == NULL) {
		printf("Error opening BTD file, using defaults ... \n");
		return;
	}
	
	for(int i = 0; i < BLOCK_TYPE_COUNT; i++){
		char name[MAX_BLOCKNAME_LENGTH];
		int nums [6];
		fscanf ( btd_file, "%s %i %i %i %i %i %i", name, &nums[0], &nums[1], &nums[2], &nums[3], &nums[4], &nums[5]);
		memcpy(blockname_map[i], name, MAX_BLOCKNAME_LENGTH);
		for(int k = 0; k < 6; k++){
			btd_map[i].index[k] =  (uint8_t)nums[k];
		}
		
	}
	
	fclose (btd_file);
}
