#include <generator.h>
#include <chunklist.h>
#include <chunkbuilder.h>
#include <pnoise.h>

#include <pthread.h>

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

pthread_t gen_thread;
pthread_mutex_t chunk_gen_mutex;

// Copy of chunk_list in chunkbuilder.c
struct CLL* cl[3];

#define HFUNC(y, m ,l) ((m) * (y) - (m) * (l))

float block_noise (float x, float y, float z){ 
	float first_oct = noise((x) * 0.01f, (y) * 0.0125f, (z) * 0.01f);
	float second_oct = noise((x) * 0.04f, (y) * 0.05f, (z) * 0.04f);
	float third_oct = noise((x) * 0.08f, (y) * 0.1f, (z) * 0.08f);
	float fourth_oct = noise((x) * 0.16f, (y) * 0.2f, (z) * 0.16f);
	
	float blend = first_oct * 0.66f + second_oct * 0.166f + third_oct *0.0833f + fourth_oct * 0.04155f;
	
	return blend - HFUNC(y, 0.015f, 64);
}


void* chunk_gen_thread (void* arg){
	
	struct chunkspace_position prepos = *((struct chunkspace_position*)arg);
	struct chunkspace_position* pos = &prepos;
	
	pthread_mutex_lock(&chunk_gen_mutex);
	
	lock_list(cl[2]);
	
	bool has_chunk_assigned [NUMBER_CHUNKS];
	memset(has_chunk_assigned, 0, sizeof(has_chunk_assigned));
	
	struct CLL or_chunks = CLL_init(); // List of Out-Of-Range Chunks
	
	// Find All Chunks that are out of range 
	for(struct CLL_element* e = cl[0]->first; e != NULL; e = e->nxt){ 
		
		chunk_data_sync(e->data);
		int32_t x_local = e->data->_x - pos->_x + WORLD_RANGE;
		int32_t z_local = e->data->_z - pos->_z + WORLD_RANGE;
		
		if(x_local < 0 || z_local < 0 || x_local > WORLD_RANGE * 2 || z_local > WORLD_RANGE * 2){ // Chunks is out of Range
			e->data->render = false; // reenabled by the chunkbuilder-thread
			CLL_add(&or_chunks, e->data);
		}else{
			has_chunk_assigned[ATCHUNK(x_local, z_local)] = true;
			
			if(!e->data->initialized){
				CLL_add(cl[2], e->data);
				e->data->initialized = true;
			}
			
		}

		chunk_data_unsync(e->data);
	}
	// Reassign out of range chunks to positions that dont have one yet
	struct CLL_element* j;
	j = or_chunks.first;
	for(int cx = 0; cx < WORLD_RANGE*2+1;++cx){
		for(int cz = 0; cz < WORLD_RANGE*2+1;++cz){
			if(!has_chunk_assigned[ATCHUNK(cx,cz)]){
				if( j != NULL ){
					chunk_data_sync(j->data);
					int32_t x_global = cx + pos->_x - WORLD_RANGE;
					int32_t z_global = cz + pos->_z - WORLD_RANGE;
					j->data->_x = x_global;
					j->data->_z = z_global;
					CLL_add(cl[2], j->data);
					chunk_data_unsync(j->data);
					j = j->nxt;
				}
			}
			
		}
	}
	
	CLL_freeList(&or_chunks);
	CLL_destroyList(&or_chunks);
	
	//TODO: figure out which chunks to update based on where the player is located
	struct CLL_element* p;
	for(p = cl[2]->first; p != NULL; p = p->nxt){
		chunk_data_sync(p->data);
		
		memset(p->data->data.block_data, 0, CHUNK_MEM);
		
		int x = p->data->_x;
		int z = p->data->_z;
		
		for(int cx = 0; cx < CHUNK_SIZE_X;++cx){
			for(int cz = 0; cz < CHUNK_SIZE_Z;++cz){
				bool under_sky = true;
				int depth_below = 0;
				for(int cy = CHUNK_SIZE_Y - 1; cy >= 0;--cy){
					if( block_noise((cx + x * CHUNK_SIZE_X), (cy), (cz + z * CHUNK_SIZE_Z)) > 0){
						if(!under_sky){
							if(depth_below < 4){
								p->data->data.block_data[ATBLOCK(cx,cy,cz)] = 2;
								depth_below++;
							}
							else{
								p->data->data.block_data[ATBLOCK(cx,cy,cz)] = 3;
							}
						}else{
							p->data->data.block_data[ATBLOCK(cx,cy,cz)] = 1;
							under_sky = false;
						}
					}else{
						under_sky = true;
						depth_below = 0;
					}
				}
			}
		}
		chunk_data_unsync(p->data);
		lock_list(cl[1]);
		CLL_add(cl[1], p->data);
		unlock_list(cl[1]);
		trigger_chunk_update();
	}
	
	lock_list(cl[1]);
	CLL_copyList(cl[0], cl[1]);
	unlock_list(cl[1]);

	CLL_freeList(cl[2]);
	
	trigger_chunk_update(); //Clear up the borders
	
	unlock_list(cl[2]);
	
	pthread_mutex_unlock(&chunk_gen_mutex);
	return 0;
}

bool initialize_generator_thread (){
	cl[0] = get_chunk_list(0);
	cl[1] = get_chunk_list(1);
	cl[2] = get_chunk_list(2);
	
	pthread_mutex_init(&chunk_gen_mutex, NULL);
	
	return true;
}


void trigger_generator_update (struct chunkspace_position* pos ){
	pthread_create(&gen_thread, NULL, chunk_gen_thread, pos);
}

void terminate_generator_thread (){
	pthread_cancel(gen_thread);
	pthread_join(gen_thread, NULL);
	pthread_mutex_destroy(&chunk_gen_mutex);
}
