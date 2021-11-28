#include <lightcalc.h>

#include <worlddefs.h>
#include <chunklist.h>
#include <globallists.h>
#include <genericlist.h>
#include <generator.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
	TODO:
	Lots and lots of shared code between skylight_func and blocklight_func
 */

void calculate_light (struct sync_chunk_t* for_chunk, void (*calc_func)(struct sync_chunk_t**), bool override){
	
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
	struct sync_chunk_t* fast_list[9]; // This optimisation means that light will only be able to cross a single chunk border
	fast_list[0] = CLL_getDataAt(&calc_list, chunk_x + 1, chunk_z);
	fast_list[1] = CLL_getDataAt(&calc_list, chunk_x - 1, chunk_z);
	fast_list[2] = CLL_getDataAt(&calc_list, chunk_x, chunk_z + 1);
	fast_list[3] = CLL_getDataAt(&calc_list, chunk_x, chunk_z - 1);
	fast_list[4] = CLL_getDataAt(&calc_list, chunk_x + 1, chunk_z + 1);
	fast_list[5] = CLL_getDataAt(&calc_list, chunk_x + 1, chunk_z - 1);
	fast_list[6] = CLL_getDataAt(&calc_list, chunk_x - 1, chunk_z + 1);
	fast_list[7] = CLL_getDataAt(&calc_list, chunk_x - 1, chunk_z - 1);
	fast_list[8] = CLL_getDataAt(&calc_list, chunk_x, chunk_z);
	
	(*calc_func)(fast_list);
	
	
	for(p = calc_list.first; p != NULL; p = p->nxt){ // Copy over Light Data
		struct sync_chunk_t* cpy = CLL_getDataAt(&chunk_list[0] ,p->data->_x, p->data->_z);
		if(cpy != NULL){
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
	}
	
	for(p = calc_list.first; p != NULL; p = p->nxt){ // delete temporary lightlist copies
		GLL_free_rec(&p->data->lightlist);
		GLL_destroy(&p->data->lightlist);
	}
	
	CLL_freeListAndData(&calc_list); // And Data, to delete the copies
	CLL_destroyList(&calc_list);
	
}

void skylight_func (struct sync_chunk_t** fast_list){
	
	int current_que_length = 0;
	struct ipos3 lighting_priority_que [CHUNK_LAYER * CHUNK_SIZE_Y * 9]; // statically allocated, because dynamically allocating lists with like 100000+ members is big oof for cpu time (so we do big oof for memory stack instead)
	
	void push_back_position (int x, int y, int z){
		
		struct ipos3 t = {x,y,z};
		lighting_priority_que[current_que_length] = t;
		current_que_length++;
	}
	
	/*
		initialize the priority que, by filling every Air Block at the Max Height with MAX_LIGHT
	 */
	
	int32_t base_chunk_x = fast_list[8]->_x;
	int32_t base_chunk_z = fast_list[8]->_z;
	
	for(int i = 0; i < 9; i++){
		struct sync_chunk_t* sct = fast_list[i];
		
		if(sct != NULL){
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
	}
	/*
		Propagate Light on the most efficient path using the priority-que
	 */
	
	struct sync_chunk_t* patient = fast_list[8];
	int current_chunk_x = base_chunk_x;
	int current_chunk_z = base_chunk_z;
	
	struct sync_chunk_t* get_border_chunk (int32_t cur_x, int32_t cur_z){
		if(cur_x == base_chunk_x + 1 && cur_z == base_chunk_z + 1)
			return fast_list[4];
		else if(cur_x == base_chunk_x + 1 && cur_z == base_chunk_z - 1)
			return fast_list[5];
		else if(cur_x == base_chunk_x - 1 && cur_z == base_chunk_z + 1)
			return fast_list[6];
		else if(cur_x == base_chunk_x - 1 && cur_z == base_chunk_z - 1)
			return fast_list[7];
		else if(cur_x == base_chunk_x + 1 && cur_z == base_chunk_z)
			return fast_list[0];
		else if(cur_x == base_chunk_x - 1 && cur_z == base_chunk_z)
			return fast_list[1];
		else if(cur_z == base_chunk_z + 1 && cur_x == base_chunk_x)
			return fast_list[2];
		else if(cur_z == base_chunk_z - 1 && cur_x == base_chunk_x)
			return fast_list[3];
		else if(cur_x == base_chunk_x && cur_z == base_chunk_z)
			return fast_list[8];
		else 
			return NULL;
	}
	
	for(int i = 0; i < current_que_length; i++) { // go over the blocks in decending order of light intensity
		
		struct ipos3* local = &lighting_priority_que[i];
		int cs_x = local->_x % CHUNK_SIZE; cs_x = (cs_x >= 0) ? cs_x : (CHUNK_SIZE + cs_x);
		int cs_y = local->_y;
		int cs_z = local->_z % CHUNK_SIZE; cs_z = (cs_z >= 0) ? cs_z : (CHUNK_SIZE + cs_z);
		int new_chunk_x = (local->_x / 16) + base_chunk_x + ((local->_x >= 0) ? 0 : -1);
		int new_chunk_z = (local->_z / 16) + base_chunk_z + ((local->_z >= 0) ? 0 : -1);
						
		if(new_chunk_x != current_chunk_x || new_chunk_z != current_chunk_z){ // if we are in a different chunk now
			current_chunk_x = new_chunk_x;
			current_chunk_z = new_chunk_z;

			// TODO: Replace this with an If-Statement (like with in load_surrounding_data)
			patient = get_border_chunk(current_chunk_x, current_chunk_z); 
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
				struct sync_chunk_t* n_chunk = get_border_chunk( current_chunk_x + 1, current_chunk_z);
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
				struct sync_chunk_t* n_chunk = get_border_chunk(current_chunk_x - 1, current_chunk_z);
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
				struct sync_chunk_t* n_chunk = get_border_chunk(current_chunk_x, current_chunk_z + 1);
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
				struct sync_chunk_t* n_chunk = get_border_chunk(current_chunk_x, current_chunk_z - 1);
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

void blocklight_func (struct sync_chunk_t** fast_list){ 
	
	int current_que_length = 0;
	struct ipos3 lighting_priority_que [CHUNK_LAYER * CHUNK_SIZE_Y * 9]; // statically allocated, because dynamically allocating lists with like 100000+ members is big oof for cpu time (so we do big oof for memory stack instead)
	
	void push_back_position (int x, int y, int z){
		
		struct ipos3 t = {x,y,z};
		lighting_priority_que[current_que_length] = t;
		current_que_length++;
	}
	
	/*
		initialize the priority que, by filling every Air Block at the Max Height with MAX_LIGHT
	 */
	
	int32_t base_chunk_x = fast_list[8]->_x;
	int32_t base_chunk_z = fast_list[8]->_z;
	
	for (int i = 0; i < 9; i++){
		
		struct sync_chunk_t* sct = fast_list[i];
		
		if( sct != NULL){
			int32_t offset_x = (sct->_x - base_chunk_x) * CHUNK_SIZE;
			int32_t offset_z = (sct->_z - base_chunk_z) * CHUNK_SIZE;
			
			for(struct GLL_element* g = sct->lightlist.first; g != NULL; g = g->next){
				struct ipos3* d = g->data;
				uint16_t llevel = MAX_LIGHT;
				sct->light.block_data[ATBLOCK(d->_x,d->_y,d->_z)] = llevel;
				push_back_position(d->_x + offset_x, d->_y, d->_z + offset_z);
			}
			
		}
	}
	/*
		Propagate Light on the most efficient path using the priority-que
	 */
	
	struct sync_chunk_t* patient = fast_list[8];
	int current_chunk_x = base_chunk_x;
	int current_chunk_z = base_chunk_z;
	
	struct sync_chunk_t* get_border_chunk (int32_t cur_x, int32_t cur_z){
		if(cur_x == base_chunk_x + 1 && cur_z == base_chunk_z + 1)
			return fast_list[4];
		else if(cur_x == base_chunk_x + 1 && cur_z == base_chunk_z - 1)
			return fast_list[5];
		else if(cur_x == base_chunk_x - 1 && cur_z == base_chunk_z + 1)
			return fast_list[6];
		else if(cur_x == base_chunk_x - 1 && cur_z == base_chunk_z - 1)
			return fast_list[7];
		else if(cur_x == base_chunk_x + 1 && cur_z == base_chunk_z)
			return fast_list[0];
		else if(cur_x == base_chunk_x - 1 && cur_z == base_chunk_z)
			return fast_list[1];
		else if(cur_z == base_chunk_z + 1 && cur_x == base_chunk_x)
			return fast_list[2];
		else if(cur_z == base_chunk_z - 1 && cur_x == base_chunk_x)
			return fast_list[3];
		else if(cur_x == base_chunk_x && cur_z == base_chunk_z)
			return fast_list[8];
		else 
			return NULL;
	}
	
	for(int i = 0; i < current_que_length; i++) { // go over the blocks in decending order of light intensity
		
		struct ipos3* local = &lighting_priority_que[i];
		int cs_x = local->_x % CHUNK_SIZE; cs_x = (cs_x >= 0) ? cs_x : (CHUNK_SIZE + cs_x);
		int cs_y = local->_y;
		int cs_z = local->_z % CHUNK_SIZE; cs_z = (cs_z >= 0) ? cs_z : (CHUNK_SIZE + cs_z);
		int new_chunk_x = (local->_x / 16) + base_chunk_x + ((local->_x >= 0) ? 0 : -1);
		int new_chunk_z = (local->_z / 16) + base_chunk_z + ((local->_z >= 0) ? 0 : -1);
						
		if(new_chunk_x != current_chunk_x || new_chunk_z != current_chunk_z){ // if we are in a different chunk now
			current_chunk_x = new_chunk_x;
			current_chunk_z = new_chunk_z;

			// TODO: Replace this with an If-Statement (like with in load_surrounding_data)
			patient = get_border_chunk(current_chunk_x, current_chunk_z); 
		}
		
		if(patient != NULL){
			
			uint8_t llevel = patient->light.block_data[ATBLOCK(cs_x, cs_y, cs_z)];
			
			if( llevel == MIN_LIGHT ){
				continue;
			}
			
			if(cs_y-1 >= 0){

				if(!(patient->light.block_data[ATBLOCK(cs_x, cs_y-1, cs_z)] >= llevel-1)) // If next block isnt already lit
				if( !IS_SOLID(patient->data_unique.block_data[ATBLOCK(cs_x, cs_y-1, cs_z)] )){ // If next block isnt solid
					patient->light.block_data[ATBLOCK(cs_x, cs_y-1, cs_z)] = llevel - 1;
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
				struct sync_chunk_t* n_chunk = get_border_chunk( current_chunk_x + 1, current_chunk_z);
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
				struct sync_chunk_t* n_chunk = get_border_chunk(current_chunk_x - 1, current_chunk_z);
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
				struct sync_chunk_t* n_chunk = get_border_chunk(current_chunk_x, current_chunk_z + 1);
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
				struct sync_chunk_t* n_chunk = get_border_chunk(current_chunk_x, current_chunk_z - 1);
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
