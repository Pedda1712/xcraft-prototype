#include <generator.h>
#include <chunklist.h>
#include <chunkbuilder.h>
#include <pnoise.h>
#include <lightcalc.h>
#include <worldsave.h>
#include <genericlist.h>
#include <blocktexturedef.h>
#include <struced/octree.h>

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

void determine_chunk_space_coords (int isx, int isz, int* ccx_r, int* ccz_r, int* chunk_x_r, int* chunk_z_r){
	float _add_x = (isx > 0) ? CHUNK_SIZE / 2.0f : -CHUNK_SIZE / 2.0f;
	float _add_z = (isz > 0) ? CHUNK_SIZE / 2.0f : -CHUNK_SIZE / 2.0f;

	int32_t chunk_x = (isx + _add_x) / CHUNK_SIZE;
	int32_t chunk_z = (isz + _add_z) / CHUNK_SIZE;
	
	float c_x_off = (float)chunk_x - 0.5f;
	float c_z_off = (float)chunk_z - 0.5f;
	
	int ccx = (int)((float)isx/BLOCK_SIZE - c_x_off * CHUNK_SIZE)%CHUNK_SIZE; // world space coordinate to chunk space coordinate
	int ccz = (int)((float)isz/BLOCK_SIZE - c_z_off * CHUNK_SIZE)%CHUNK_SIZE;
	
	if(isx < 0 && ccx == 0) chunk_x++; // negative integer rounding ... -.-
	if(isz < 0 && ccz == 0) chunk_z++;
	
	*ccx_r = ccx;
	*ccz_r = ccz;
	*chunk_x_r = chunk_x;
	*chunk_z_r = chunk_z;
}

static float block_noise (float x, float y, float z, float slope, float median_height){ 
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
	
	struct GLL structure_insertion_list = GLL_init();

	struct CLL_element* p;
	for(p = chunk_list[2].first; p != NULL; p = p->nxt){
		chunk_data_sync(p->data);
		
		memset(p->data->data_unique.block_data, 0, CHUNK_MEM);
		memset(p->data->light.block_data, 0, CHUNK_MEM);
		
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
					
					uint16_t top_layer_block = btd_map[SNOW_B].complete_id; //Grass
					uint16_t sec_layer_block = btd_map[SNOW_B].complete_id;  //Dirt 
					uint16_t thr_layer_block = btd_map[STONE_B].complete_id; //Stone
					uint16_t liquid_layer    = btd_map[WATER_B].complete_id; //data_unique
					
					float ocean_modif = (noise((cx + x * CHUNK_SIZE) * 0.005f, 0, (cz + z * CHUNK_SIZE) * 0.005f)) * 0.5f + (noise((cx + x * CHUNK_SIZE) * 0.00125f, 0, (cz + z * CHUNK_SIZE) * 0.00125f)) * 0.5f;
					ocean_modif-=0.05125f;
					
					float ocean_slope = (ocean_modif < 0.0f) ? 0.0f : ocean_modif;
					float slope_modifier = (noise((cx + x * CHUNK_SIZE) * 0.005f, 300.0f, (cz + z * CHUNK_SIZE) * 0.005f) + 1.125f) + ocean_slope * (5 * ocean_slope + 6); // how strong the height based fade out is (higher fadeout -> flatter terrain, ocean contributes to flatness to have less islands)
					slope_modifier = pow(slope_modifier, 16); // make distinction between flat and hilly areas more distinctive by raising to power
					
					float median_f = pow(2, -(slope_modifier-6.01f)) + 32; // 32 is standard "plains" level, more mountain, higher median level
					median_f -= 16 * ocean_slope * (5 * ocean_slope + 6); // subtract up to 16 from median when ocean
					
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
					
					uint32_t top_y = 0;
					
					for(int cy = CHUNK_SIZE_Y - 1; cy >= 0;--cy){
						
						if( cy < SNOW_LEVEL + lake_modif * 16){
							top_layer_block = btd_map[SGRASS_B].complete_id;
							sec_layer_block = btd_map[DIRT_B].complete_id;
						}
						if( cy < SGRASS_LEVEL + lake_modif * 16){
							top_layer_block = btd_map[GRASS_B].complete_id;
							sec_layer_block = btd_map[DIRT_B].complete_id;
						}
						
						if( cy < WATER_LEVEL + 1){
							top_layer_block = btd_map[GRAVEL_B].complete_id; //Gravel
							sec_layer_block = btd_map[GRAVEL_B].complete_id; //Gravel
						}
						
						if( block_noise((cx + x * CHUNK_SIZE), (cy), (cz + z * CHUNK_SIZE), slope_modifier, median_f) > 0){
							if(!under_sky){
								if(depth_below < 3){
									p->data->data_unique.block_data[ATBLOCK(cx,cy,cz)] = sec_layer_block;
									depth_below++;
								}
								else{
									p->data->data_unique.block_data[ATBLOCK(cx,cy,cz)] = thr_layer_block;
								}
							}else{
								p->data->data_unique.block_data[ATBLOCK(cx,cy,cz)] = top_layer_block;
								
								if((BLOCK_ID(top_layer_block) == GRASS_B || BLOCK_ID(top_layer_block) == SGRASS_B) && cy < CHUNK_SIZE_Y - 1 && ngrass_modif > 0.0f){
									
									uint16_t grass_type = (grass_modif > 0.0f) ? btd_map[XGRASS_B].complete_id : btd_map[XGRAST_B].complete_id;
									
									if(nflow_modif > 0.28f){
										grass_type += (flow_modif > -0.75f) ? 2 : 0;
										grass_type += (flow_modif > -0.25f) ? 2 : 0;
										grass_type += (flow_modif > 0.25f && flow_modif < 0.75f) ? 2 : 0;
									}
									
									p->data->data_unique.block_data[ATBLOCK(cx, cy + 1, cz)] = grass_type;
								}
								
								top_y = cy;
								under_sky = false;
							}
						}else{
							if(cy == 0){
								p->data->data_unique.block_data[ATBLOCK(cx,cy,cz)] = btd_map[GRAVEL_B].complete_id;
							}
							else if(cy < WATER_LEVEL){
								p->data->data_unique.block_data[ATBLOCK(cx, cy, cz)] = liquid_layer;
							}
							depth_below = 0;
						}
					}
					
					float structure1_modif = (noise((cx + x * CHUNK_SIZE) * 0.9f, 0, (cz + z * CHUNK_SIZE) * 0.9f));
					
					if(structure1_modif > 0.65f && top_y > WATER_LEVEL){
						struct structure_t* new = malloc(sizeof(struct structure_t));
						struct ipos3 p = {cx + (x - .5f) * CHUNK_SIZE, top_y, cz + (z - .5f) * CHUNK_SIZE};
						new->base = p;
						new->index = 0;
						GLL_add(&structure_insertion_list, new);
					}
					
				}
			}
		}else{
			for(int x = 0; x < CHUNK_SIZE; x++){
				for(int z = 0; z < CHUNK_SIZE; z++){
					for(int y = 0; y < CHUNK_SIZE_Y; y++){
						int i = x  + z * CHUNK_SIZE + y * CHUNK_LAYER;

						if(BLOCK_ID(p->data->data_unique.block_data[i]) == LIGHT_B){
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
		
		/*
			Check if Structures have been generated in this chunk while it was out of range
		 */
		
		struct GLL possible_structures = GLL_init();
		
		if(read_structure_log_into_gll(x, z, &possible_structures)){ // there are structure-blocks!
			for(struct GLL_element* e = possible_structures.first; e != NULL; e = e->next){
				struct block_t* current = e->data;
				p->data->data_unique.block_data[ATBLOCK(current->_x, current->_y, current->_z)] = current->_type;
			}
		}
		
		GLL_free_rec(&possible_structures);
		GLL_destroy(&possible_structures);
		
		chunk_data_unsync(p->data);
	}
	
	struct GLL or_structure_log = GLL_init();
	
	// Insert the Structures
	lock_list(&chunk_list[1]);
	for (struct GLL_element* e = structure_insertion_list.first; e != NULL; e = e->next) { // the newly generated structures
		struct structure_t* current = e->data;
		struct structure_log_t* current_log = NULL;
		
		int t, chunk_x, chunk_z;
		determine_chunk_space_coords(current->base._x, current->base._z, &t, &t, &chunk_x, &chunk_z);
		
		struct sync_chunk_t* patient = CLL_getDataAt(&chunk_list[0], chunk_x, chunk_z); // Start operating on the chunk of the base block
		for(struct GLL_element* p = structure_cache[current->index].first; p != NULL; p = p->next){
			
			struct block_t* current_block = p->data;
			
			int ccx;
			int ccz;
			int new_chunk_x;
			int new_chunk_z;
			determine_chunk_space_coords(current_block->_x + current->base._x, current_block->_z + current->base._z, &ccx, &ccz, &new_chunk_x, &new_chunk_z);
			
			if(new_chunk_x != chunk_x || new_chunk_z != chunk_z){
				chunk_x = new_chunk_x;
				chunk_z = new_chunk_z;
				patient = CLL_getDataAt(&chunk_list[0], chunk_x, chunk_z); // Start operating on the chunk of the base block
				
				if(patient != NULL){
					CLL_add(&chunk_list[1], patient);
				}else{ // Chunk is Out of Range, add entry to the or_structure_log if it doesnt already exist
					bool exists = false;
					for(struct GLL_element* e = or_structure_log.first; e != NULL; e = e->next){
						struct structure_log_t* log = e->data;
						if(log->_x == chunk_x && log->_z == chunk_z){
							current_log = log;
							exists = true;
							break;
						}
					}
					if(!exists){
						struct structure_log_t* log = malloc(sizeof(struct structure_log_t));
						log->_x = chunk_x;
						log->_z = chunk_z;
						log->blocks = GLL_init();
						GLL_add(&or_structure_log, log);
						current_log = log;
					}
				}
			}
						
			if(patient != NULL){
				patient->data_unique.block_data[ATBLOCK(ccx, current_block->_y + current->base._y, ccz)] = current_block->_type;
			}else{ // Chunk with blocks in it is not loaded
				
				struct block_t* local_block = malloc (sizeof(struct block_t));
				struct block_t t = {ccx, current_block->_y + current->base._y, ccz, current_block->_type}; 
				*local_block = t;
				
				if(current_log != NULL){
					GLL_add(&current_log->blocks, local_block);
				}
				
			}
			
		}
	}
	unlock_list(&chunk_list[1]);
	
	// Push the Out-Of-Range Parts of the Structures to a file
	
	for(struct GLL_element* e = or_structure_log.first; e != NULL; e = e->next){ // destroy the lists;
		struct structure_log_t* log = e->data;
		
		save_structure_log_into_file (log);
		
		GLL_free_rec(&log->blocks);
		GLL_destroy(&log->blocks);
	}
	GLL_free_rec(&or_structure_log);
	GLL_destroy(&or_structure_log);
	
	GLL_free_rec(&structure_insertion_list);
	GLL_destroy(&structure_insertion_list);

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

static void* chunk_gen_thread (){
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
