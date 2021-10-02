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
	{{6, 6, 6, 6, 6, 6}}, // Light
	{{2, 7, 8, 8, 8, 8}}, // Snowy Grass
	{{7, 7, 7, 7, 7, 7}}, // Snow
	{{240, 240, 240, 240, 240, 240}}, // Grass Blades
	{{241, 241, 241, 241, 241, 241}}, // Tall Grass Blades
	{{242, 242, 242, 242, 242, 242}}, // Grass Blades with Flowers
	{{243, 243, 243, 243, 243, 243}}, // Grass Blades with Flowers
	{{244, 244, 244, 244, 244, 244}}, // Grass Blades with Flowers
	{{245, 245, 245, 245, 245, 245}}, // Grass Blades with Flowers
	{{246, 246, 246, 246, 246, 246}}, // Grass Blades with Flowers
	{{247, 247, 247, 247, 247, 247}}, // Grass Blades with Flowers
};

char blockname_map [BLOCK_TYPE_COUNT] [MAX_BLOCKNAME_LENGTH] = {
	"Air",
	"Grass",
	"Dirt",
	"Stone",
	"Gravel",
	"Water",
	"Light Source",
	"Snowy Grass",
	"Snow",
	"Grass Blades",
	"Tall Grass Blades",
	"Flower Blades",
	"Tall Flower Blades",
	"Flower Blades",
	"Tall Flower Blades",
	"Flower Blades",
	"Tall Flower Blades"
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
