#include <lightcalc.h>

#include <worlddefs.h>
#include <chunklist.h>
#include <globallists.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void calculate_light (struct sync_chunk_t* for_chunk, void (*calc_func)(struct CLL* list)){
	
	int chunk_x = for_chunk->_x;
	int chunk_z = for_chunk->_z;
	
	int chunk_distance = (int)(MAX_LIGHT / CHUNK_SIZE) + 1;
	
	struct CLL calc_list = CLL_init();

	for (int cx = chunk_x - chunk_distance; cx <= chunk_x + chunk_distance; cx++){
		for(int cz = chunk_z - chunk_distance; cz <= chunk_z + chunk_distance; cz++){
			struct sync_chunk_t* current = CLL_getDataAt (&chunk_list[0], cx, cz);
			
			if (current != NULL) {
				
				/*
				 * Creating copies of the chunks to make this thread-safe
				 */
				
				struct sync_chunk_t* temp = malloc (sizeof(struct sync_chunk_t));
				pthread_mutex_init(&temp->c_mutex, NULL); // This mutex is never used, but is destroyed by CLL_freeListAndData
				
				memcpy(temp->data.block_data, current->data.block_data, CHUNK_MEM);
				memcpy(temp->water.block_data, current->water.block_data, CHUNK_MEM);
				temp->_x = current->_x;
				temp->_z = current->_z;
				
				for(int i = 0; i < MESH_LEVELS; ++i){ // We need to initialize these, even though they are not used because CLL_freeListAndData will attempt to free them
					temp->vertex_array[i] = DFA_init();
					temp->texcrd_array[i] = DFA_init();
					temp->lightl_array[i] = DFA_init();
				}
				
				CLL_add (&calc_list, temp);
			}
			
		}
	}
	
	struct CLL_element* p;
	for(p = calc_list.first; p != NULL; p = p->nxt){ // Delete Existing Light Data
		memset (p->data->light.block_data, MIN_LIGHT, CHUNK_MEM);
	}
	(*calc_func)(&calc_list);
	
	struct sync_chunk_t* cpy = CLL_getDataAt(&calc_list ,for_chunk->_x, for_chunk->_z);
	if(cpy != NULL){ // Should never fail
		memcpy(for_chunk->light.block_data, cpy->light.block_data, CHUNK_MEM); // Copy from temp to actual light buffer
	}
	
	CLL_freeListAndData(&calc_list); // AndData, to delete the copies
	CLL_destroyList(&calc_list);
	
}

void rec_skylight_func_down (struct sync_chunk_t* sct, int x, int y, int z, uint8_t llevel){

	
	if (llevel == MIN_LIGHT) return;
	if (sct->light.block_data[ATBLOCK(x, y, z)] >= llevel) return;
	if (sct->data.block_data [ATBLOCK(x, y, z)] != AIR_B) return;
	
	uint8_t down_fac = 0;

	sct->light.block_data[ATBLOCK(x, y, z)] = llevel;
	
	if (y-1 >= 0) {
		if (sct->water.block_data [ATBLOCK(x, y-1, z)] == WATER_B) down_fac = 1; // Light doesnt "flow" freely through water
		rec_skylight_func_down (sct, x, y-1, z, llevel - down_fac);
	}

}

void rec_skylight_func_sides (struct sync_chunk_t* sct, int x, int y, int z, uint8_t llevel, bool fromup, struct CLL* calc_list){

	
	if (llevel == MIN_LIGHT) return;
	if (sct->light.block_data[ATBLOCK(x, y, z)] > llevel) return;
	if (sct->light.block_data[ATBLOCK(x, y, z)] >= llevel && !fromup) return;
	if (sct->data.block_data [ATBLOCK(x, y, z)] != AIR_B) return;

	
	sct->light.block_data[ATBLOCK(x, y, z)] = llevel;
	
	uint8_t down_fac = 0;
	
	if ( y+1 < CHUNK_SIZE_Y ){
		rec_skylight_func_sides (sct, x, y+1, z, llevel - 1, false, calc_list);
	}
	
	int chunk_x = sct->_x;
	int chunk_z = sct->_z;
	if ( x+1 < CHUNK_SIZE ){
		rec_skylight_func_sides (sct, x+1, y, z, llevel - 1, false, calc_list);
	}else{
		struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, chunk_x + 1, chunk_z);
		if(n_chunk != NULL){
			rec_skylight_func_sides (n_chunk, 0, y, z, llevel - 1, false, calc_list);
		}
	}
	if ( x-1 >= 0 ){
		rec_skylight_func_sides (sct, x-1, y, z, llevel - 1, false, calc_list);
	}else{
		struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, chunk_x - 1, chunk_z);
		if(n_chunk != NULL){
			rec_skylight_func_sides (n_chunk, CHUNK_SIZE-1, y, z, llevel - 1, false, calc_list);
		}
	}
	if ( z+1 < CHUNK_SIZE ){
		rec_skylight_func_sides (sct, x, y, z+1, llevel - 1, false, calc_list);
	}else{
		struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, chunk_x, chunk_z + 1);
		if(n_chunk != NULL){
			rec_skylight_func_sides (n_chunk, x, y, 0, llevel - 1, false, calc_list);
		}
	}
	if ( z-1 >= 0 ){
		rec_skylight_func_sides (sct, x, y, z-1, llevel - 1, false, calc_list);
	}else{
		struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, chunk_x, chunk_z - 1);
		if(n_chunk != NULL){
			rec_skylight_func_sides (n_chunk, x, y, CHUNK_SIZE-1, llevel - 1, false, calc_list);
		}
	}
	
	if(y-1 >= 0){
		if (sct->water.block_data [ATBLOCK(x, y-1, z)] == WATER_B) down_fac = 1; // Light doesnt "flow" freely through water
		rec_skylight_func_sides (sct, x, y-1, z, llevel - down_fac, true, calc_list);
	}
}

void skylight_func (struct CLL* calc_list){

	struct CLL_element* p;
	for(p = calc_list->first; p != NULL; p = p->nxt){
		int y = CHUNK_SIZE_Y - 1;
		for (int x = 0; x < CHUNK_SIZE; x++){
			for(int z = 0; z < CHUNK_SIZE; z++){
				rec_skylight_func_down(p->data, x, y, z, MAX_LIGHT);
			}
		}
	}
	for(p = calc_list->first; p != NULL; p = p->nxt){
		int y = CHUNK_SIZE_Y - 1;
		for (int x = 0; x < CHUNK_SIZE; x++){
			for(int z = 0; z < CHUNK_SIZE; z++){
				rec_skylight_func_sides(p->data, x, y, z, MAX_LIGHT, true, calc_list);
			}
		}
	}
}
