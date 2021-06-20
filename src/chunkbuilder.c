#include <chunkbuilder.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <chunklist.h>

#include <pnoise.h>

#include <pthread.h>
#include <string.h>
#include <math.h>

#define X_AXIS 1
#define Y_AXIS 2
#define Z_AXIS 3

#define TEX_SIZE (1.0f/16.0f)

#include <globallists.h>
/*
 * Contains:
 * chunk_list as defined in globallists.c:
 * [0] -> All Chunks 
 * [1] -> All Chunks whose mesh needs updating 
 * [2] -> All Chunks who need to be regenerated at a new position
*/

pthread_t chunk_thread;
pthread_mutex_t chunk_builder_mutex;
pthread_cond_t chunk_builder_lock;
bool is_running;

void emit_face (struct sync_chunk_t* in, float wx, float wy, float wz, uint8_t axis, bool mirorred, uint8_t block_t, uint8_t offset, bool shortened){

	float lightlevel = (Y_AXIS == axis) ? ( mirorred ? 1.0f : 0.45f) : 0.8f;
	float diroffset =  (Z_AXIS == axis) ? 0.7f : 1.0f;
	lightlevel *= diroffset;
	
	float y_offset;
	switch(offset){
		case 1:  y_offset = 0.2f;break;
		default: y_offset = 0.0f;break;
	}
	
	switch(axis){
		
		case X_AXIS:{
			DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz);
			DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
			DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
			DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz);
		break;}
		
		case Y_AXIS:{
			DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz);
			DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz);
			DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
			DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
		break;}
		
		case Z_AXIS:{
			DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * mirorred);
			DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * mirorred);
			DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * mirorred);
			DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * mirorred);
		break;}
		
	}
	
	switch(block_t){
		case 1:{
			if(axis == Y_AXIS){
				if(mirorred){
					DFA_add(&in[0].texcrd_array[offset], 0.0f);DFA_add(&in[0].texcrd_array[offset], 0.0f);
					DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);DFA_add(&in[0].texcrd_array[offset], 0.0f);
					DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);
					DFA_add(&in[0].texcrd_array[offset], 0.0f);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);
				}else{
					DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * 2);DFA_add(&in[0].texcrd_array[offset], 0.0f);
					DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * 3);DFA_add(&in[0].texcrd_array[offset], 0.0f);
					DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * 3);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);
					DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * 2);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);
				}
			}else{
				DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * 2);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);
				DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);
				DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);DFA_add(&in[0].texcrd_array[offset], 0.0f);
				DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * 2);DFA_add(&in[0].texcrd_array[offset], 0.0f);
			}
		break;}
		default:{
			DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * block_t * 1.0f);DFA_add(&in[0].texcrd_array[offset], 0.0f);
			DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * (block_t+1) * 1.0f);DFA_add(&in[0].texcrd_array[offset], 0.0f);
			DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * (block_t+1) * 1.0f);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);
			DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * block_t * 1.0f);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE);
		break;}
	}
	
	for(int i = 0; i < 12; i++){DFA_add(&in[0].lightl_array[offset], lightlevel);};
}

void build_chunk_mesh (struct sync_chunk_t* in, uint8_t m_level){
	
	
	int chunk_x = in->_x;
	int chunk_z = in->_z;

	/*
	0->the chunk in +X direction (or NULL)
	1->the chunk in -X direction (or NULL)
	2->the chunk in +Z direction (or NULL)
	3->the chunk in -Z direction (or NULL)
	*/
	
	struct sync_chunk_t* neighbours [4];
	neighbours[0] = CLL_getDataAt(&chunk_list[0], chunk_x + 1, chunk_z);
	neighbours[1] = CLL_getDataAt(&chunk_list[0], chunk_x - 1, chunk_z);
	neighbours[2] = CLL_getDataAt(&chunk_list[0], chunk_x, chunk_z + 1);
	neighbours[3] = CLL_getDataAt(&chunk_list[0], chunk_x, chunk_z - 1);
	
	struct chunk_t* data;
	struct chunk_t* neighbour_data [4];
	uint8_t emit_offset;
	bool shortened_block = (m_level == 1);
	
	switch (m_level){
		
		case 1: {
			data = &in->water;
			emit_offset = 1;
			for(int i = 0; i < 4; ++i){
				if(neighbours[i] != NULL){
					neighbour_data[i] = &neighbours[i]->water;
				}else{
					neighbour_data[i] = NULL;
				}
			}
			break;
		}
		
		default: {
			data = &in->data;
			emit_offset = 0;
			for(int i = 0; i < 4; ++i){
				if(neighbours[i] != NULL){
					neighbour_data[i] = &neighbours[i]->data;
				} else{
					neighbour_data[i] = NULL;
				}
			}
			break;
		}
	}
	
	float c_x_off = (float)chunk_x - 0.5f;
	float c_z_off = (float)chunk_z - 0.5f;
	
	DFA_clear(&in->vertex_array[emit_offset]);
	DFA_clear(&in->texcrd_array[emit_offset]);
	DFA_clear(&in->lightl_array[emit_offset]);
	
	bool has_block_in_surrounding (int x, int y, int z, uint8_t type){
		
		uint8_t block_type = 0;
		
		//X+
		if(x+1 >= CHUNK_SIZE_X){
			if(neighbour_data[0] != NULL)
				block_type = neighbour_data[0]->block_data[ATBLOCK(0, y, z)];
		}else{
			block_type = data->block_data[ATBLOCK(x+1, y, z)];
		}
		if(block_type == type) return true;
		
		//X-
		if(x-1 < 0){
			if(neighbour_data[1] != NULL)
				block_type = neighbour_data[1]->block_data[ATBLOCK(CHUNK_SIZE_X-1, y, z)];
		}else{
			block_type = data->block_data[ATBLOCK(x-1, y, z)];
		}
		if(block_type == type) return true;
		
		//Z+
		if(z+1 >= CHUNK_SIZE_Z){
			if(neighbour_data[2] != NULL)
				block_type = neighbour_data[2]->block_data[ATBLOCK(x, y, 0)];
		}else{
			block_type = data->block_data[ATBLOCK(x, y, z+1)];
		}
		if(block_type == type) return true;
		
		//Z-
		if(z-1 < 0){
			if(neighbour_data[3] != NULL)
				block_type = neighbour_data[3]->block_data[ATBLOCK(x, y, CHUNK_SIZE_Z-1)];
		}else{
			block_type = data->block_data[ATBLOCK(x, y, z-1)];
		}
		if(block_type == type) return true;
		
		return false;
	}
	
	bool is_shortened (int x, int y, int z){
		if(m_level == 1){
			if(y + 1 > CHUNK_SIZE_Y){
				return true;
			}else{
				if(data->block_data[ATBLOCK(x, y+1, z)] == WATER_B){
					return false;
				}else{
					return true;
				}
			}
		}
		return false;
	}
	

	for(int x = 0; x < CHUNK_SIZE_X;++x){
		for(int y = 0; y < CHUNK_SIZE_Y;++y){
			for(int z = 0; z < CHUNK_SIZE_Z;++z){
				
				bool emitter_cond;
				switch(m_level){
					case 1: {
						emitter_cond = data->block_data[ATBLOCK(x,y,z)] == 5;
						break;
					}
					default: {
						emitter_cond = data->block_data[ATBLOCK(x,y,z)] != 0;
						break;
					}
				}
				
				if (emitter_cond){
					float wx,wy,wz;
					wx = (x + c_x_off * CHUNK_SIZE_X) * BLOCK_SIZE;
					wz = (z + c_z_off * CHUNK_SIZE_Z) * BLOCK_SIZE;
					wy = y * BLOCK_SIZE;
					uint8_t block_t = data->block_data[ATBLOCK(x,y,z)];
					
					shortened_block = is_shortened (x, y, z);
					
					if(!((y + 1) == CHUNK_SIZE_Y)){
						if(data->block_data[ATBLOCK(x,y+1,z)] == 0){
							emit_face(in, wx, wy, wz, Y_AXIS, true, block_t, emit_offset, shortened_block);
						}else{ // Special Case for Water only
							if(m_level == 1){
								if(data->block_data[ATBLOCK(x,y+1,z)] != WATER_B && !has_block_in_surrounding(x, y+1, z, WATER_B) && has_block_in_surrounding(x, y+1, z, 0)){
									emit_face(in, wx, wy, wz, Y_AXIS, true, block_t, emit_offset, shortened_block);
								}
							}
						}
					}else{
						emit_face(in, wx, wy, wz, Y_AXIS, true, block_t, emit_offset, shortened_block);
					}
					
					if(!((y - 1) < 0)){
						if(data->block_data[ATBLOCK(x,y-1,z)] == 0){
							emit_face(in, wx, wy, wz, Y_AXIS, false, block_t, emit_offset, shortened_block);
						}
					}else{
						//emit_face(in, wx, wy, wz, Y_AXIS, false, block_t);
					}
					
					if(!((x + 1) == CHUNK_SIZE_X)){
						if(data->block_data[ATBLOCK(x+1,y,z)] == 0){
							emit_face(in, wx, wy, wz, X_AXIS, true, block_t, emit_offset, shortened_block);
						}
					}else{
						if(neighbour_data[0] != NULL)
							if(neighbour_data[0]->block_data[ATBLOCK(0,y,z)] == 0){
								emit_face(in, wx, wy, wz, X_AXIS, true, block_t, emit_offset, shortened_block);
							}
					}
					
					if(!((x - 1) < 0)){
						if(data->block_data[ATBLOCK(x-1,y,z)] == 0){
							emit_face(in, wx, wy, wz, X_AXIS, false, block_t, emit_offset, shortened_block);
						}
					}else{
						if(neighbour_data[1] != NULL){
							if(neighbour_data[1]->block_data[ATBLOCK(CHUNK_SIZE_X-1,y,z)] == 0){
								emit_face(in, wx, wy, wz, X_AXIS, false, block_t, emit_offset, shortened_block);
							}
						}
					}
					
					if(!((z + 1) == CHUNK_SIZE_Z)){
						if(data->block_data[ATBLOCK(x,y,z+1)] == 0){
							emit_face(in, wx, wy, wz, Z_AXIS, true, block_t, emit_offset, shortened_block);
						}
					}else{
						if(neighbour_data[2] != NULL){
							if(neighbour_data[2]->block_data[ATBLOCK(x,y,0)] == 0){
								emit_face(in, wx, wy, wz, Z_AXIS, true, block_t, emit_offset, shortened_block);
							}
						}
					}
					
					if(!((z - 1) < 0)){
						if(data->block_data[ATBLOCK(x,y,z-1)] == 0){
							emit_face(in, wx, wy, wz, Z_AXIS, false, block_t, emit_offset, shortened_block);
						}
					}else{
						if(neighbour_data[3] != NULL){
							if(neighbour_data[3]->block_data[ATBLOCK(x,y,CHUNK_SIZE_Z-1)] == 0){
								emit_face(in, wx, wy, wz, Z_AXIS, false, block_t, emit_offset, shortened_block);
							}
						}
					}
				}
				
			}
		}
	}
}

void* chunk_thread_func (void* arg){
	pthread_mutex_lock(&chunk_builder_mutex);
	while(is_running){
		pthread_cond_wait(&chunk_builder_lock, &chunk_builder_mutex);
		
		lock_list(&chunk_list[1]);
			
		struct CLL_element* p;
		for(p = chunk_list[1].first; p != NULL; p = p->nxt){
			chunk_data_sync(p->data);

			build_chunk_mesh(p->data, 0); // Build Block Mesh
			build_chunk_mesh(p->data, 1); // Build Water Mesh
			
			chunk_data_unsync(p->data);
		}
		CLL_freeList(&chunk_list[1]);
		unlock_list(&chunk_list[1]);
		
	}
	pthread_mutex_unlock(&chunk_builder_mutex);
	return 0;
}

bool initialize_chunk_thread (){
	chunk_list[0] = CLL_init();
	chunk_list[1] = CLL_init();
	chunk_list[2] = CLL_init();
	
	pthread_mutex_init(&chunk_builder_mutex,NULL);
	pthread_cond_init(&chunk_builder_lock, NULL);
	
	for(int x = -WORLD_RANGE; x <= WORLD_RANGE; ++x){
		for(int z = -WORLD_RANGE; z <= WORLD_RANGE;++z){
			struct sync_chunk_t* temp = malloc (sizeof(struct sync_chunk_t));
			if(pthread_mutex_init(&temp->c_mutex, NULL) != 0){return false;}

			for(int i = 0; i < MESH_LEVELS; ++i){
				temp->vertex_array[i] = DFA_init();
				temp->texcrd_array[i] = DFA_init();
				temp->lightl_array[i] = DFA_init();
			}
			
			temp->_x = x;
 			temp->_z = z;
			temp->initialized = false;
			CLL_add(&chunk_list[0], temp);
		}
	}
	
	is_running = true;
	pthread_create(&chunk_thread, NULL, chunk_thread_func, NULL);
	
	return true;
}

void terminate_chunk_thread (){
	
	is_running = false;
	trigger_chunk_update();
	
	pthread_join(chunk_thread, NULL);
	
	printf("Freeing Chunk Memory ...\n");
	lock_list(&chunk_list[0]);
	lock_list(&chunk_list[1]);
	lock_list(&chunk_list[2]);
	CLL_freeListAndData(&chunk_list[0]);
	CLL_freeList(&chunk_list[1]);
	CLL_freeList(&chunk_list[2]);
	CLL_destroyList(&chunk_list[0]);
	CLL_destroyList(&chunk_list[1]);
	CLL_destroyList(&chunk_list[2]);
	
	pthread_mutex_destroy(&chunk_builder_mutex);
	pthread_cond_destroy(&chunk_builder_lock);
}

struct CLL* get_chunk_list (uint8_t l){
	return &chunk_list[l];
}

bool chunk_updating (struct sync_chunk_t* c){
	if(pthread_mutex_trylock(&c->c_mutex) == 0){
		return false;
	}else{
		return true;
	}
}

void chunk_data_sync  (struct sync_chunk_t* c){
	pthread_mutex_lock(&c->c_mutex);
}

void chunk_data_unsync(struct sync_chunk_t* c){
	pthread_mutex_unlock(&c->c_mutex);
}

void trigger_chunk_update (){
	pthread_cond_broadcast(&chunk_builder_lock);
}
