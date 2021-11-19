#include <blocktexturedef.h>
#include <worlddefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct blocktexdef_t btd_map [BLOCK_TYPE_COUNT] = { // Default BTD values
	{{255, 255, 255, 255, 255, 255}, 0, 0}, // Air
	{{2, 0, 1, 1, 1, 1}, 128, 1 + SOLID_FLAG}, // Grass
	{{2, 2, 2, 2, 2, 2}, 128, 2 + SOLID_FLAG}, // Dirt
	{{3, 3, 3, 3, 3, 3}, 128, 3 + SOLID_FLAG}, // Stone
	{{4, 4, 4, 4, 4, 4}, 128, 4 + SOLID_FLAG}, // Gravel
	{{5, 5, 5, 5, 5, 5}, 32 , 5 + TRANS_FLAG}, // Water
	{{6, 6, 6, 6, 6, 6}, 128, 6 + SOLID_FLAG}, // Light
	{{2, 7, 8, 8, 8, 8}, 128, 7 + SOLID_FLAG}, // Snowy Grass
	{{7, 7, 7, 7, 7, 7}, 128, 8 + SOLID_FLAG}, // Snow
	{{240, 240, 240, 240, 240, 240}, 64, 9 + X_FLAG}, // Grass Blades
	{{241, 241, 241, 241, 241, 241}, 64, 10 + X_FLAG}, // Tall Grass Blades
	{{242, 242, 242, 242, 242, 242}, 64, 11 + X_FLAG}, // Grass Blades with Flowers
	{{243, 243, 243, 243, 243, 243}, 64, 12 + X_FLAG}, // Grass Blades with Flowers
	{{244, 244, 244, 244, 244, 244}, 64, 13 + X_FLAG}, // Grass Blades with Flowers
	{{245, 245, 245, 245, 245, 245}, 64, 14 + X_FLAG}, // Grass Blades with Flowers
	{{246, 246, 246, 246, 246, 246}, 64, 15 + X_FLAG}, // Grass Blades with Flowers
	{{247, 247, 247, 247, 247, 247}, 64, 16 + X_FLAG}, // Grass Blades with Flowers
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
		int flags;
		fscanf ( btd_file, "%s %i %i %i %i %i %i %i", name, &nums[0], &nums[1], &nums[2], &nums[3], &nums[4], &nums[5], &flags);
		memcpy(blockname_map[i], name, MAX_BLOCKNAME_LENGTH);
		
		btd_map[i].flag_register = (uint8_t) flags;
		btd_map[i].complete_id = i + (flags << 8);
		
		for(int k = 0; k < 6; k++){
			btd_map[i].index[k] =  (uint8_t)nums[k];
		}
		
	}
	
	fclose (btd_file);
}
