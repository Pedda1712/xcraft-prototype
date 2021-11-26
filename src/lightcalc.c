#include <lightcalc.h>

#include <worlddefs.h>
#include <chunklist.h>
#include <globallists.h>
#include <genericlist.h>
#include <generator.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void calculate_light (struct sync_chunk_t* for_chunk, void (*calc_func)(struct CLL* list), bool override){
	
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
				
				memcpy(temp->data_unique.block_data, current->data_unique.block_data, CHUNK_MEM);
				memcpy(temp->light.block_data, current->light.block_data, CHUNK_MEM);
				temp->_x = current->_x;
				temp->_z = current->_z;
				temp->lightlist = GLL_init();
				
				GLL_lock(&current->lightlist);
				for(struct GLL_element* e = current->lightlist.first; e != NULL; e = e->next){
					struct ipos3* n = malloc(sizeof(struct ipos3));
					*n = *(struct ipos3*)e->data;
					GLL_add(&temp->lightlist, n);
				}
				GLL_unlock (&current->lightlist);
				
				for(int i = 0; i < MESH_LEVELS; ++i){ // We need to initialize these, even though they are not used because CLL_freeListAndData will attempt to free them
					temp->mesh_buffer[i] = DFA_init();
				}
				
				CLL_add (&calc_list, temp);
			}
			
		}
	}
	
	struct CLL_element* p;
	for(p = calc_list.first; p != NULL; p = p->nxt){ // Delete Existing Light Data
		//memset (p->data->light.block_data, 0, CHUNK_MEM);
		for(int i = 0; i < CHUNK_MEM / 2; i++){
			p->data->light.block_data[i] = MIN_LIGHT;
		}
	}
	(*calc_func)(&calc_list);
	
	
	for(p = calc_list.first; p != NULL; p = p->nxt){ // Copy over Light Data
		struct sync_chunk_t* cpy = CLL_getDataAt(&chunk_list[0] ,p->data->_x, p->data->_z);
		if(cpy != NULL)
			if(override){
				memcpy(cpy->light.block_data, p->data->light.block_data, CHUNK_MEM); // Copy from temp to actual light buffer
			}else{
				for(int i = 0; i < CHUNK_MEM/2; i++){
					if(cpy->light.block_data[i] < p->data->light.block_data[i]){
						cpy->light.block_data[i] = p->data->light.block_data[i];
					}
				}
			}
	}
	
	for(p = calc_list.first; p != NULL; p = p->nxt){ // delete temporary lightlist copies
		GLL_free_rec(&p->data->lightlist);
		GLL_destroy(&p->data->lightlist);
	}
	
	CLL_freeListAndData(&calc_list); // And Data, to delete the copies
	CLL_destroyList(&calc_list);
	
}

void skylight_func (struct CLL* calc_list){
	
	int llength = 0;
	for(struct CLL_element* e = calc_list->first; e != NULL; e = e->nxt) llength++;
	
	int current_que_length = 0;
	
	struct ipos3 lighting_priority_que [CHUNK_LAYER * CHUNK_SIZE_Y * llength]; // statically allocated, because dynamically allocating lists with like 100000+ members is big oof for cpu time (so we do big oof for memory stack instead)
	
	void push_back_position (int x, int y, int z){
		
		struct ipos3 t = {x,y,z};
		lighting_priority_que[current_que_length] = t;
		current_que_length++;
	}
	
	/*
		initialize the priority que, by filling every Air Block at the Max Height with MAX_LIGHT
	 */
	
	int32_t base_chunk_x = calc_list->first->data->_x;
	int32_t base_chunk_z = calc_list->first->data->_z;
	
	struct CLL_element* p;
	for(p = calc_list->first; p != NULL; p = p->nxt){
		struct sync_chunk_t* sct = p->data;
		
		int32_t offset_x = (sct->_x - base_chunk_x) * CHUNK_SIZE;
		int32_t offset_z = (sct->_z - base_chunk_z) * CHUNK_SIZE;
		
		for (int x = 0; x < CHUNK_SIZE; x++){
			for(int z = 0; z < CHUNK_SIZE; z++){
				uint16_t llevel = MAX_LIGHT;
				int y = CHUNK_SIZE_Y-1;
				if(!IS_SOLID( sct->data_unique.block_data[ATBLOCK(x, y, z)])){
					sct->light.block_data[ATBLOCK(x,y,z)] = llevel;
					push_back_position(x + offset_x, y, z + offset_z);
				}
			}
		}
	}
	/*
		Propagate Light on the most efficient path using the priority-que
	 */
	
	struct sync_chunk_t* patient = calc_list->first->data;
	int current_chunk_x = base_chunk_x;
	int current_chunk_z = base_chunk_z;
	for(int i = 0; i < current_que_length; i++) { // go over the blocks in decending order of light intensity
		
		struct ipos3* local = &lighting_priority_que[i];
		int cs_x = local->_x % 16;
		int cs_y = local->_y;
		int cs_z = local->_z % 16;
		int new_chunk_x = (local->_x / 16) + base_chunk_x;
		int new_chunk_z = (local->_z / 16) + base_chunk_z;
						
		if(new_chunk_x != current_chunk_x || new_chunk_z != current_chunk_z){ // if we are in a different chunk now
			current_chunk_x = new_chunk_x;
			current_chunk_z = new_chunk_z;

			patient = CLL_getDataAt(calc_list, current_chunk_x, current_chunk_z);
			//printf("ok\n");
		}
		
		if(patient != NULL){
			
			uint8_t llevel = patient->light.block_data[ATBLOCK(cs_x, cs_y, cs_z)];
			
			if( llevel == MIN_LIGHT ){
				continue;
			}
			
			if(cs_y-1 >= 0){
				uint8_t down_fac = (llevel == MAX_LIGHT) ? 0 : 1;
				if (BLOCK_ID(patient->data_unique.block_data [ATBLOCK(cs_x, cs_y-1, cs_z)]) == WATER_B || IS_P(patient->data_unique.block_data [ATBLOCK(cs_x, cs_y-1, cs_z)])) down_fac = 1; // Light gets dimmer through water
				
				if(!(patient->light.block_data[ATBLOCK(cs_x, cs_y-1, cs_z)] >= llevel-down_fac)) // If next block isnt already lit
				if( !IS_SOLID(patient->data_unique.block_data[ATBLOCK(cs_x, cs_y-1, cs_z)] )){ // If next block isnt solid
					patient->light.block_data[ATBLOCK(cs_x, cs_y-1, cs_z)] = llevel - down_fac;
					push_back_position(local->_x, cs_y-1, local->_z); // Add new block to the que
				}
			}
			
			if ( cs_y+1 < CHUNK_SIZE_Y ){
				if(!(patient->light.block_data[ATBLOCK(cs_x, cs_y+1, cs_z)] >= llevel-1))
				if( !IS_SOLID(patient->data_unique.block_data[ATBLOCK(cs_x, cs_y+1, cs_z)] )){
					patient->light.block_data[ATBLOCK(cs_x, cs_y+1, cs_z)] = llevel - 1;
					push_back_position(local->_x, cs_y+1, local->_z);
				}
			}
			

			if ( cs_x+1 < CHUNK_SIZE ){
				if(!(patient->light.block_data[ATBLOCK(cs_x+1, cs_y, cs_z)] >= llevel-1))
				if( !IS_SOLID(patient->data_unique.block_data[ATBLOCK(cs_x+1, cs_y, cs_z)] )){
					patient->light.block_data[ATBLOCK(cs_x+1, cs_y, cs_z)] = llevel - 1;
					push_back_position(local->_x + 1, cs_y, local->_z);
				}
			}else{
				struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, current_chunk_x + 1, current_chunk_z);
				if(n_chunk != NULL){
					if(!(n_chunk->light.block_data[ATBLOCK(0, cs_y, cs_z)] >= llevel-1))
					if( !IS_SOLID(n_chunk->data_unique.block_data[ATBLOCK(0, cs_y, cs_z)] )){
						n_chunk->light.block_data[ATBLOCK(0, cs_y, cs_z)] = llevel - 1;
						push_back_position(local->_x + 1, cs_y, local->_z);
					}
				}
			}
			
			if ( cs_x-1 >= 0 ){
				if(!(patient->light.block_data[ATBLOCK(cs_x-1, cs_y, cs_z)] >= llevel-1))
				if( !IS_SOLID(patient->data_unique.block_data[ATBLOCK(cs_x-1, cs_y, cs_z)] )){
					patient->light.block_data[ATBLOCK(cs_x-1, cs_y, cs_z)] = llevel - 1;
					push_back_position(local->_x - 1, cs_y, local->_z);
				}
			}else{
				struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, current_chunk_x - 1, current_chunk_z);
				if(n_chunk != NULL){
					
					if(!(n_chunk->light.block_data[ATBLOCK(CHUNK_SIZE - 1, cs_y, cs_z)] >= llevel-1))
					if( !IS_SOLID(n_chunk->data_unique.block_data[ATBLOCK(CHUNK_SIZE - 1, cs_y, cs_z)] )){
						n_chunk->light.block_data[ATBLOCK(CHUNK_SIZE - 1, cs_y, cs_z)] = llevel - 1;
						push_back_position(local->_x - 1, cs_y, local->_z);
					}
				}
			}
			
			if ( cs_z+1 < CHUNK_SIZE ){
				if(!(patient->light.block_data[ATBLOCK(cs_x, cs_y, cs_z+1)] >= llevel-1))
				if( !IS_SOLID(patient->data_unique.block_data[ATBLOCK(cs_x, cs_y, cs_z+1)] )){
					patient->light.block_data[ATBLOCK(cs_x, cs_y, cs_z+1)] = llevel - 1;
					push_back_position(local->_x, cs_y, local->_z + 1);
				}
			}else{
				struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, current_chunk_x, current_chunk_z + 1);
				if(n_chunk != NULL){
					if(!(n_chunk->light.block_data[ATBLOCK(cs_x, cs_y, 0)] >= llevel-1))
					if( !IS_SOLID(n_chunk->data_unique.block_data[ATBLOCK(cs_x, cs_y, 0)] )){
						n_chunk->light.block_data[ATBLOCK(cs_x, cs_y, 0)] = llevel - 1;
						push_back_position(local->_x, cs_y, local->_z + 1);
					}
				}
			}
			
			if ( cs_z-1 >= 0 ){
				if(!(patient->light.block_data[ATBLOCK(cs_x, cs_y, cs_z-1)] >= llevel-1))
				if( !IS_SOLID(patient->data_unique.block_data[ATBLOCK(cs_x, cs_y, cs_z-1)] )){
					patient->light.block_data[ATBLOCK(cs_x, cs_y, cs_z-1)] = llevel - 1;
					push_back_position(local->_x, cs_y, local->_z - 1);
				}
			}else{
				struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, current_chunk_x, current_chunk_z - 1);
				if(n_chunk != NULL){
					
					if(!(n_chunk->light.block_data[ATBLOCK(cs_x, cs_y, CHUNK_SIZE - 1)] >= llevel-1))
					if( !IS_SOLID(n_chunk->data_unique.block_data[ATBLOCK(cs_x, cs_y, CHUNK_SIZE - 1)] )){
						n_chunk->light.block_data[ATBLOCK(cs_x, cs_y, CHUNK_SIZE - 1)] = llevel - 1;
						push_back_position(local->_x, cs_y, local->_z - 1);
					}
				}
			}
			
			
		}
		
	}

}

static void rec_blocklight_func (struct sync_chunk_t* sct, int x, int y, int z, uint16_t llevel, struct CLL* calc_list){

	if (llevel == MIN_LIGHT) return;
	if (sct->light.block_data[ATBLOCK(x, y, z)] >= llevel) return;
	if (IS_SOLID(sct->data_unique.block_data[ATBLOCK(x, y, z)]) && BLOCK_ID(sct->data_unique.block_data [ATBLOCK(x, y, z)]) != LIGHT_B) return;

	
	sct->light.block_data[ATBLOCK(x, y, z)] = llevel;
	
	
	if ( y+1 < CHUNK_SIZE_Y ){
		rec_blocklight_func (sct, x, y+1, z, llevel - 1, calc_list);
	}
	
	int chunk_x = sct->_x;
	int chunk_z = sct->_z;
	if ( x+1 < CHUNK_SIZE ){
		rec_blocklight_func (sct, x+1, y, z, llevel - 1, calc_list);
	}else{
		struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, chunk_x + 1, chunk_z);
		if(n_chunk != NULL){
			rec_blocklight_func (n_chunk, 0, y, z, llevel - 1, calc_list);
		}
	}
	if ( x-1 >= 0 ){
		rec_blocklight_func (sct, x-1, y, z, llevel - 1, calc_list);
	}else{
		struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, chunk_x - 1, chunk_z);
		if(n_chunk != NULL){
			rec_blocklight_func (n_chunk, CHUNK_SIZE-1, y, z, llevel - 1, calc_list);
		}
	}
	if ( z+1 < CHUNK_SIZE ){
		rec_blocklight_func (sct, x, y, z+1, llevel - 1, calc_list);
	}else{
		struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, chunk_x, chunk_z + 1);
		if(n_chunk != NULL){
			rec_blocklight_func (n_chunk, x, y, 0, llevel - 1, calc_list);
		}
	}
	if ( z-1 >= 0 ){
		rec_blocklight_func (sct, x, y, z-1, llevel - 1, calc_list);
	}else{
		struct sync_chunk_t* n_chunk = CLL_getDataAt(calc_list, chunk_x, chunk_z - 1);
		if(n_chunk != NULL){
			rec_blocklight_func (n_chunk, x, y, CHUNK_SIZE-1, llevel - 1, calc_list);
		}
	}
	
	if(y-1 >= 0){
		rec_blocklight_func (sct, x, y-1, z, llevel - 1, calc_list);
	}
}

void blocklight_func (struct CLL* calc_list){
	for (struct CLL_element* e = calc_list->first; e != NULL; e = e->nxt){
		for(struct GLL_element* g = e->data->lightlist.first; g != NULL; g = g->next){
			struct ipos3* d = g->data;
			rec_blocklight_func (e->data, d->_x, d->_y, d->_z, MAX_LIGHT, calc_list);
		}
	}
}
