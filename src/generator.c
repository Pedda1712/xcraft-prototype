#include <generator.h>
#include <chunklist.h>
#include <chunkbuilder.h>
#include <pnoise.h>
#include <lightcalc.h>
#include <worldsave.h>
#include <genericlist.h>

#include <pthread.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <stdio.h>

pthread_t gen_thread;
pthread_mutex_t chunk_gen_mutex;
pthread_cond_t chunk_gen_lock;

bool is_chunkgen_running;

struct chunkspace_position chunk_gen_arg;

#include <globallists.h>
/*
 * globallists.h Contains:
 * chunk_list as defined in globallists.c:
 * [0] -> All Chunks 
 * [1] -> All Chunks whose mesh needs updating 
 * [2] -> All Chunks who need to be regenerated at a new position
*/

#define HFUNC(y, m ,l) ((m) * (y) - (m) * (l))
// y -> height, m -> slope, l -> "sea level"

float block_noise (float x, float y, float z, float slope, float median_height){ 
	float first_oct  = noise((x) * 0.02f, (y) * 0.0125f, (z) * 0.02f);
	float second_oct = noise((x) * 0.04f, (y) * 0.05f, (z) * 0.04f);
	float third_oct  = noise((x) * 0.08f, (y) * 0.1f, (z) * 0.08f);
	float fourth_oct = noise((x) * 0.16f, (y) * 0.2f, (z) * 0.16f);
	
	float blend = first_oct * 0.75f + second_oct * 0.125f + third_oct * 0.0625f + fourth_oct * 0.03125f;
	
	return blend - HFUNC(y, slope, median_height);
}

void generate_chunk_data () {
	
	lock_list(&chunk_list[2]);
	CLL_freeList(&chunk_list[2]);
	
	struct chunkspace_position prepos = chunk_gen_arg;
	struct chunkspace_position* pos = &prepos;
	
	bool has_chunk_assigned [NUMBER_CHUNKS];
	memset(has_chunk_assigned, 0, sizeof(has_chunk_assigned));
	
	struct CLL or_chunks = CLL_init(); // List of Out-Of-Range Chunks
	
	// Find All Chunks that are out of range 
	for(struct CLL_element* e = chunk_list[0].first; e != NULL; e = e->nxt){ 
		
		chunk_data_sync(e->data);
		int32_t x_local = e->data->_x - pos->_x + WORLD_RANGE;
		int32_t z_local = e->data->_z - pos->_z + WORLD_RANGE;
		
		if(x_local < 0 || z_local < 0 || x_local > WORLD_RANGE * 2 || z_local > WORLD_RANGE * 2){ // Chunks is out of Range
			CLL_add(&or_chunks, e->data);
		}else{
			has_chunk_assigned[ATCHUNK(x_local, z_local)] = true;
			
			if(!e->data->initialized){
				CLL_add(&or_chunks, e->data);
				has_chunk_assigned[ATCHUNK(x_local, z_local)] = false;
			}
			
		}

		chunk_data_unsync(e->data);
	}
	
	for (struct CLL_element* e = or_chunks.first; e != NULL; e = e->nxt){
		if(e->data->initialized){
			dump_chunk(e->data);
		}else{
			e->data->initialized = true;
		}
	}
	
	// Reassign out of range chunks to positions that dont have one yet and add them to the generation queue (chunk_list[2])
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

					CLL_add(&chunk_list[2], j->data);
					
					struct CLL_element* temp = j;
					j = j->nxt;
					chunk_data_unsync(temp->data);
				}
			}
			
		}
	}
	
	CLL_freeList(&or_chunks);
	CLL_destroyList(&or_chunks);

	struct CLL_element* p;
	for(p = chunk_list[2].first; p != NULL; p = p->nxt){
		chunk_data_sync(p->data);
		
		memset(p->data->data.block_data, 0, CHUNK_MEM);
		memset(p->data->data_unique.block_data, 0, CHUNK_MEM);
		GLL_lock(&p->data->lightlist);
		GLL_free_rec (&p->data->lightlist);
		GLL_unlock(&p->data->lightlist);
		
		int x = p->data->_x;
		int z = p->data->_z;
		
		if( !read_chunk (p->data) ){
			for(int cx = 0; cx < CHUNK_SIZE;++cx){
				for(int cz = 0; cz < CHUNK_SIZE;++cz){
					bool under_sky = true;
					int depth_below = 0;
					
					uint8_t top_layer_block = SNOW_B; //Grass
					uint8_t sec_layer_block = SNOW_B;  //Dirt 
					uint8_t thr_layer_block = STONE_B; //Stone
					uint8_t liquid_layer    = WATER_B; //data_unique
					
					float ocean_modif = (noise((cx + x * CHUNK_SIZE) * 0.005f, 0, (cz + z * CHUNK_SIZE) * 0.005f)) * 0.5f + (noise((cx + x * CHUNK_SIZE) * 0.00125f, 0, (cz + z * CHUNK_SIZE) * 0.00125f)) * 0.5f;
					
					float ocean_slope = (ocean_modif < 0.0f) ? 0.0f : ocean_modif;
					float slope_modifier = (noise((cx + x * CHUNK_SIZE) * 0.005f, 300.0f, (cz + z * CHUNK_SIZE) * 0.005f) + 1.002f) + ocean_slope * (5 * ocean_slope + 6);
					slope_modifier = pow(slope_modifier, 8);
					
					float median_f = pow(2, -(slope_modifier-6.01f)) + 32;
					median_f -= 16 * ocean_slope * (5 * ocean_slope + 6);
					
					slope_modifier = slope_modifier * 0.014f;
					if(slope_modifier < 0.014f) slope_modifier = 0.014f;
					
					float lake_modif = (noise((cx + x * CHUNK_SIZE) * 0.02f, 0, (cz + z * CHUNK_SIZE) * 0.02f));
					if(lake_modif < 0.0f){
						median_f += 6 * lake_modif;
					}
					float grass_modif = (noise((cx + x * CHUNK_SIZE) * 0.05f, 0, (cz + z * CHUNK_SIZE) * 0.05f));
					float ngrass_modif = (noise((cx + x * CHUNK_SIZE) * 0.35f, 0, (cz + z * CHUNK_SIZE) * 0.35f));
					
					float flow_modif = (noise((cx + x * CHUNK_SIZE) * 0.02f, 16.0f, (cz + z * CHUNK_SIZE) * 0.02f));
					float nflow_modif = (noise((cx + x * CHUNK_SIZE) * 0.03f, 32.0f, (cz + z * CHUNK_SIZE) * 0.03f)) * 0.5f + (noise((cx + x * CHUNK_SIZE) * 0.9f, 32.0f, (cz + z * CHUNK_SIZE) * 0.9f)) * 0.5f;
					
					for(int cy = CHUNK_SIZE_Y - 1; cy >= 0;--cy){
						
						if( cy < SNOW_LEVEL + lake_modif * 16){
							top_layer_block = SGRASS_B;
							sec_layer_block = DIRT_B;
						}
						if( cy < SGRASS_LEVEL + lake_modif * 16){
							top_layer_block = GRASS_B;
							sec_layer_block = DIRT_B;
						}
						
						if( cy < WATER_LEVEL + 1){
							top_layer_block = GRAVEL_B; //Gravel
							sec_layer_block = GRAVEL_B; //Gravel
						}
						
						if( block_noise((cx + x * CHUNK_SIZE), (cy), (cz + z * CHUNK_SIZE), slope_modifier, median_f) > 0){
							if(!under_sky){
								if(depth_below < 3){
									p->data->data.block_data[ATBLOCK(cx,cy,cz)] = sec_layer_block;
									p->data->data_unique.block_data[ATBLOCK(cx,cy,cz)] = sec_layer_block;
									depth_below++;
								}
								else{
									p->data->data.block_data[ATBLOCK(cx,cy,cz)] = thr_layer_block;
									p->data->data_unique.block_data[ATBLOCK(cx,cy,cz)] = thr_layer_block;
								}
							}else{
								p->data->data.block_data[ATBLOCK(cx,cy,cz)] = top_layer_block;
								p->data->data_unique.block_data[ATBLOCK(cx,cy,cz)] = top_layer_block;
								
								if((top_layer_block == GRASS_B || top_layer_block == SGRASS_B) && cy < CHUNK_SIZE_Y - 1 && ngrass_modif > 0.0f){
									
									uint8_t grass_type = (grass_modif > 0.0f) ? XGRASS_B : XGRAST_B;
									
									if(nflow_modif > 0.28f){
										grass_type += (flow_modif > -0.75f) ? 2 : 0;
										grass_type += (flow_modif > -0.25f) ? 2 : 0;
										grass_type += (flow_modif > 0.25f && flow_modif < 0.75f) ? 2 : 0;
									}
									
									p->data->data_unique.block_data[ATBLOCK(cx, cy + 1, cz)] = grass_type;
								}
								
								under_sky = false;
							}
						}else{
							if(cy == 0){
								p->data->data.block_data[ATBLOCK(cx,cy,cz)] = GRAVEL_B;
								p->data->data_unique.block_data[ATBLOCK(cx,cy,cz)] = GRAVEL_B;
							}
							else if(cy < WATER_LEVEL){
								p->data->data_unique.block_data[ATBLOCK(cx, cy, cz)] = liquid_layer;
							}
							depth_below = 0;
						}
					}
				}
			}
		}else{
			for(int x = 0; x < CHUNK_SIZE; x++){
				for(int z = 0; z < CHUNK_SIZE; z++){
					for(int y = 0; y < CHUNK_SIZE_Y; y++){
						int i = x  + z * CHUNK_SIZE + y * CHUNK_LAYER;
						if(p->data->data_unique.block_data[i] != WATER_B && (p->data->data_unique.block_data[i] < XGRASS_B || p->data->data_unique.block_data[i] > XFLOW6_B)){
							p->data->data.block_data[i] = p->data->data_unique.block_data[i];
						}
						if(p->data->data.block_data[i] == LIGHT_B){
							struct ipos3* npos = malloc (sizeof(struct ipos3));
							npos->_x = x;npos->_y = y;npos->_z = z;
							GLL_lock(&p->data->lightlist);
							GLL_add(&p->data->lightlist, npos);
							GLL_unlock(&p->data->lightlist);
						}
					}
				}
			}
		}
		chunk_data_unsync(p->data);
	}

	// When Generation is done, rebuild Meshes for all Chunks who neighbour the new ones
	lock_list(&chunk_list[1]);
	for(p = chunk_list[2].first; p != NULL; p = p->nxt){
		chunk_data_sync(p->data);
		
		int chunk_x = p->data->_x;
		int chunk_z = p->data->_z;
		
		struct sync_chunk_t* neighbours[4];
		neighbours[0] = CLL_getDataAt(&chunk_list[0], chunk_x + 1, chunk_z);
		neighbours[1] = CLL_getDataAt(&chunk_list[0], chunk_x - 1, chunk_z);
		neighbours[2] = CLL_getDataAt(&chunk_list[0], chunk_x, chunk_z + 1);
		neighbours[3] = CLL_getDataAt(&chunk_list[0], chunk_x, chunk_z - 1);
		
		for(int i = 0; i < 4; ++i){
			if(neighbours[i] != NULL){
				CLL_add(&chunk_list[1], neighbours[i]); // CLL_add automatically checks for duplicates
			}
		}
		
		chunk_data_unsync(p->data);
	}
	unlock_list(&chunk_list[1]);
	
	trigger_builder_update(); //Clear up the borders
	
	unlock_list(&chunk_list[2]);
}

void* chunk_gen_thread (){
	pthread_mutex_lock(&chunk_gen_mutex);
	while(is_chunkgen_running){
		pthread_cond_wait(&chunk_gen_lock, &chunk_gen_mutex);
		
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		generate_chunk_data();
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	}
	pthread_mutex_unlock(&chunk_gen_mutex);
	return 0;
}

bool initialize_generator_thread (){
	pthread_mutex_init(&chunk_gen_mutex, NULL);
	pthread_cond_init(&chunk_gen_lock, NULL);
	
	is_chunkgen_running = true;
	pthread_create(&gen_thread, NULL, chunk_gen_thread, NULL);
	
	return true;
}


void trigger_generator_update (struct chunkspace_position* pos){
	chunk_gen_arg = *pos;
	pthread_cond_broadcast(&chunk_gen_lock);
}

void run_chunk_generation (struct chunkspace_position* pos){
	chunk_gen_arg = *pos;
	generate_chunk_data();
}

void run_chunk_generation_atprev (){
	generate_chunk_data();
}

void terminate_generator_thread (){
	is_chunkgen_running = false;
	trigger_generator_update(&chunk_gen_arg);
	
	pthread_cancel(gen_thread);
	pthread_join(gen_thread, NULL);
	
	pthread_mutex_destroy(&chunk_gen_mutex);
	pthread_cond_destroy (&chunk_gen_lock);
}
