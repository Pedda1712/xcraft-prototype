#include <Server/world_srv.h>
#include <worlddefs.h>

#include <string.h>

#include <stdio.h>

struct chunk_t chunk_world [WORLD_SIZE_X * WORLD_SIZE_Z];

struct chunk_t* chunk_data(int32_t x, int32_t z){
	return &chunk_world[x + z * WORLD_SIZE_X];
}

bool initialize_world () {
	if(CHUNK_SIZE_X * CHUNK_SIZE_Z > 256){
		printf("Error: Chunk Layer too big, max 256 blocks!\n");
		return false;
	}
	memset(chunk_world, 0, WORLD_SIZE_X * WORLD_SIZE_Z * sizeof(struct chunk_t));
	
	for(int x = 0; x < WORLD_SIZE_X;++x){
		for(int z = 0; z < WORLD_SIZE_Z;++z){
			
			for(int _x = 0; _x < CHUNK_SIZE_X;++_x){
				for(int _z = 0; _z < CHUNK_SIZE_Z;++_z){
					for(int _y = 0; _y < CHUNK_SIZE_Y;++_y){
						if(_y < 16)
							chunk_world[x + z * WORLD_SIZE_X].block_data[_x + _z * CHUNK_SIZE_X + _y * CHUNK_LAYER] = 1;
					}
				}
			}
			
		}
	}
	
}
