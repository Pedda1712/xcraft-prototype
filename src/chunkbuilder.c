#include <chunkbuilder.h>
#include <stdio.h>
#include <stdlib.h>

#include <chunklist.h>

#include <pnoise.h>

#include <pthread.h>

#include <string.h>
#include <math.h>

#define X_AXIS 1
#define Y_AXIS 2
#define Z_AXIS 3

#define TEX_SIZE (1.0f/16.0f)

/*
 * [0] -> All Chunks 
 * [1] -> All Chunks whos mesh needs updating 
 * [2] -> All Chunks who need to be regenerated at a new position
*/
struct CLL chunk_list [3];
pthread_t chunk_thread;
pthread_mutex_t chunk_builder_mutex;

void emit_face (struct sync_chunk_t* in, float wx, float wy, float wz, uint8_t axis, bool mirorred, uint8_t block_t){
	bool onmesh = in->onmesh;
	float lightlevel = (Y_AXIS == axis) ? ( mirorred ? 1.0f : 0.4f) : 0.8f;
	float diroffset =  (Z_AXIS == axis) ? 0.7f : 1.0f;
	lightlevel *= diroffset;
	
	switch(axis){
		
		case X_AXIS:{
			DFA_add(&in->vertex_array[onmesh], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[onmesh], wy             );DFA_add(&in->vertex_array[onmesh], wz);
			DFA_add(&in->vertex_array[onmesh], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[onmesh], wy             );DFA_add(&in->vertex_array[onmesh], wz + BLOCK_SIZE);
			DFA_add(&in->vertex_array[onmesh], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[onmesh], wy + BLOCK_SIZE);DFA_add(&in->vertex_array[onmesh], wz + BLOCK_SIZE);
			DFA_add(&in->vertex_array[onmesh], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[onmesh], wy + BLOCK_SIZE);DFA_add(&in->vertex_array[onmesh], wz);
		break;}
		
		case Y_AXIS:{
			DFA_add(&in->vertex_array[onmesh], wx             );DFA_add(&in->vertex_array[onmesh], wy + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[onmesh], wz);
			DFA_add(&in->vertex_array[onmesh], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[onmesh], wy + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[onmesh], wz);
			DFA_add(&in->vertex_array[onmesh], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[onmesh], wy + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[onmesh], wz + BLOCK_SIZE);
			DFA_add(&in->vertex_array[onmesh], wx             );DFA_add(&in->vertex_array[onmesh], wy + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[onmesh], wz + BLOCK_SIZE);
		break;}
		
		case Z_AXIS:{
			DFA_add(&in->vertex_array[onmesh], wx             );DFA_add(&in->vertex_array[onmesh], wy             );DFA_add(&in->vertex_array[onmesh], wz + BLOCK_SIZE * mirorred);
			DFA_add(&in->vertex_array[onmesh], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[onmesh], wy             );DFA_add(&in->vertex_array[onmesh], wz + BLOCK_SIZE * mirorred);
			DFA_add(&in->vertex_array[onmesh], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[onmesh], wy + BLOCK_SIZE);DFA_add(&in->vertex_array[onmesh], wz + BLOCK_SIZE * mirorred);
			DFA_add(&in->vertex_array[onmesh], wx             );DFA_add(&in->vertex_array[onmesh], wy + BLOCK_SIZE);DFA_add(&in->vertex_array[onmesh], wz + BLOCK_SIZE * mirorred);
		break;}
		
	}
	
	switch(block_t){
		case 1:{
			if(axis == Y_AXIS){
				if(mirorred){
					DFA_add(&in[0].texcrd_array[onmesh], 0.0f);DFA_add(&in[0].texcrd_array[onmesh], 0.0f);
					DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);DFA_add(&in[0].texcrd_array[onmesh], 0.0f);
					DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);
					DFA_add(&in[0].texcrd_array[onmesh], 0.0f);DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);
				}else{
					DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE * 2);DFA_add(&in[0].texcrd_array[onmesh], 0.0f);
					DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE * 3);DFA_add(&in[0].texcrd_array[onmesh], 0.0f);
					DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE * 3);DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);
					DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE * 2);DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);
				}
			}else{
				DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE * 2);DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);
				DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);
				DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);DFA_add(&in[0].texcrd_array[onmesh], 0.0f);
				DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE * 2);DFA_add(&in[0].texcrd_array[onmesh], 0.0f);
			}
		break;}
		default:{
			DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE * block_t * 1.0f);DFA_add(&in[0].texcrd_array[onmesh], 0.0f);
			DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE * (block_t+1) * 1.0f);DFA_add(&in[0].texcrd_array[onmesh], 0.0f);
			DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE * (block_t+1) * 1.0f);DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);
			DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE * block_t * 1.0f);DFA_add(&in[0].texcrd_array[onmesh], TEX_SIZE);
		break;}
	}
	
	for(int i = 0; i < 12; i++){DFA_add(&in[0].lightl_array[onmesh], lightlevel);};
}

void build_chunk_mesh (struct sync_chunk_t* in){
	
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
	
	float c_x_off = (float)chunk_x - 0.5f;
	float c_z_off = (float)chunk_z - 0.5f;
	
	bool onmesh = in->onmesh;
	in->vertex_array[onmesh].size = 0;
	in->texcrd_array[onmesh].size = 0;
	DFA_clear(&in->vertex_array[onmesh]);
	DFA_clear(&in->texcrd_array[onmesh]);
	DFA_clear(&in->lightl_array[onmesh]);

	for(int x = 0; x < CHUNK_SIZE_X;++x){
		for(int y = 0; y < CHUNK_SIZE_Y;++y){
			for(int z = 0; z < CHUNK_SIZE_Z;++z){
				
				if (in->data.block_data[ATBLOCK(x,y,z)] != 0){
					float wx,wy,wz;
					wx = (x + c_x_off * CHUNK_SIZE_X) * BLOCK_SIZE;
					wz = (z + c_z_off * CHUNK_SIZE_Z) * BLOCK_SIZE;
					wy = y * BLOCK_SIZE;
					uint8_t block_t = in->data.block_data[ATBLOCK(x,y,z)];
					if(!((y + 1) == CHUNK_SIZE_Y)){
						if(in->data.block_data[ATBLOCK(x,y+1,z)] == 0){
							emit_face(in, wx, wy, wz, Y_AXIS, true, block_t);
						}
					}else{
						emit_face(in, wx, wy, wz, Y_AXIS, true, block_t);
					}
					if(!((y - 1) < 0)){
						if(in->data.block_data[ATBLOCK(x,y-1,z)] == 0){
							emit_face(in, wx, wy, wz, Y_AXIS, false, block_t);
						}
					}else{
						//emit_face(in, wx, wy, wz, Y_AXIS, false, block_t);
					}
					
					if(!((x + 1) == CHUNK_SIZE_X)){
						if(in->data.block_data[ATBLOCK(x+1,y,z)] == 0){
							emit_face(in, wx, wy, wz, X_AXIS, true, block_t);
						}
					}else{
						if(neighbours[0] != NULL)
							if(neighbours[0]->data.block_data[ATBLOCK(0,y,z)] == 0){
								emit_face(in, wx, wy, wz, X_AXIS, true, block_t);
							}
					}
					if(!((x - 1) < 0)){
						if(in->data.block_data[ATBLOCK(x-1,y,z)] == 0){
							emit_face(in, wx, wy, wz, X_AXIS, false, block_t);
						}
					}else{
						if(neighbours[1] != NULL){
							if(neighbours[1]->data.block_data[ATBLOCK(CHUNK_SIZE_X-1,y,z)] == 0){
								emit_face(in, wx, wy, wz, X_AXIS, false, block_t);
							}
						}
					}
					
					if(!((z + 1) == CHUNK_SIZE_Z)){
						if(in->data.block_data[ATBLOCK(x,y,z+1)] == 0){
							emit_face(in, wx, wy, wz, Z_AXIS, true, block_t);
						}
					}else{
						if(neighbours[2] != NULL){
							if(neighbours[2]->data.block_data[ATBLOCK(x,y,0)] == 0){
								emit_face(in, wx, wy, wz, Z_AXIS, true, block_t);
							}
						}
					}
					if(!((z - 1) < 0)){
						if(in->data.block_data[ATBLOCK(x,y,z-1)] == 0){
							emit_face(in, wx, wy, wz, Z_AXIS, false, block_t);
						}
					}else{
						if(neighbours[3] != NULL){
							if(neighbours[3]->data.block_data[ATBLOCK(x,y,CHUNK_SIZE_Z-1)] == 0){
								emit_face(in, wx, wy, wz, Z_AXIS, false, block_t);
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
	lock_list(&chunk_list[1]);
		
	struct CLL_element* p;
	for(p = chunk_list[1].first; p != NULL; p = p->nxt){
		chunk_data_sync(p->data);

		build_chunk_mesh(p->data);
		p->data->render = true;
		
		p->data->onmesh = !p->data->onmesh; // "Swap" the meshes / make it available to render
		
		chunk_data_unsync(p->data);
	}
	CLL_freeList(&chunk_list[1]);
	unlock_list(&chunk_list[1]);
	
	pthread_mutex_unlock(&chunk_builder_mutex);
	return 0;
}

bool initialize_chunk_thread (){
	chunk_list[0] = CLL_init();
	chunk_list[1] = CLL_init();
	chunk_list[2] = CLL_init();
	
	pthread_mutex_init(&chunk_builder_mutex,NULL);
	
	for(int x = -WORLD_RANGE; x <= WORLD_RANGE; ++x){
		for(int z = -WORLD_RANGE; z <= WORLD_RANGE;++z){
			struct sync_chunk_t* temp = malloc (sizeof(struct sync_chunk_t));
			if(pthread_mutex_init(&temp->c_mutex, NULL) != 0){return false;}
			temp->vertex_array[0] = DFA_init();
			temp->texcrd_array[0] = DFA_init();
			temp->lightl_array[0] = DFA_init();
			temp->vertex_array[1] = DFA_init();
			temp->texcrd_array[1] = DFA_init();
			temp->lightl_array[1] = DFA_init();
			temp->onmesh = 0;
			temp->_x = x;
 			temp->_z = z;
			temp->initialized = false;
			temp->render = false;
			CLL_add(&chunk_list[0], temp);
		}
	}
	
	return true;
}

void terminate_chunk_thread (){
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
	pthread_create(&chunk_thread, NULL, chunk_thread_func, NULL);
}
