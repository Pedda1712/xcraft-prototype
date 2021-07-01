#include <chunkbuilder.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <chunklist.h>

#include <pnoise.h>

#include <pthread.h>
#include <string.h>
#include <math.h>

#include <lightcalc.h>

#define Y_AXIS 1
#define X_AXIS 2
#define Z_AXIS 3

#define TEX_SIZE (1.0f/16.0f)

#include <blocktexturedef.h>

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

void emit_face (struct sync_chunk_t* in, float wx, float wy, float wz, uint8_t axis, bool mirorred, uint8_t block_t, uint8_t offset, bool shortened, uint8_t lightlevel){

	float lightmul = (Y_AXIS == axis) ? ( mirorred ? 1.0f : 0.45f) : 0.8f; // Light Level depending on block side
	float diroffset =  (Z_AXIS == axis) ? 0.7f : 1.0f;
	lightmul *= diroffset;
	lightmul *= ((float)lightlevel / (float)MAX_LIGHT);
	
	float y_offset = 0.0f;
	if(offset >= 2)
		y_offset = WATER_SURFACE_OFFSET;

	
	switch(axis){ // Emit the Vertex Positions and get the lightvalue
		
		case X_AXIS:{
			if(mirorred){
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
			}else{
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE * mirorred);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz);
			}
		break;}
		
		case Y_AXIS:{

			if(mirorred){
				DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz);
				DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz);
			}else{
				DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
				DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
			}
		break;}
		
		case Z_AXIS:{
			if(mirorred){
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * !mirorred);
				DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * !mirorred);
				DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * !mirorred);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * !mirorred);
			}else{
				DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * !mirorred);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy             );DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * !mirorred);
				DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * !mirorred);
				DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * shortened);DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE * !mirorred);
			}
		break;}
		
	}
	
	struct blocktexdef_t tex = btd_map[block_t];
	uint8_t tex_index   = tex.index[(axis - 1) * 2 + mirorred];
	uint8_t tex_index_x = tex_index % 16;
	uint8_t tex_index_y = tex_index / 16;
	DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * (tex_index_x+1));DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * (tex_index_y + 1));
	DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * tex_index_x);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * (tex_index_y + 1));
	DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * tex_index_x);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * tex_index_y);
	DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * (tex_index_x+1));DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * tex_index_y);

	// Emit the Light Level
	for(int i = 0; i < 12; i++){DFA_add(&in[0].lightl_array[offset], lightmul);};
}

void emit_fluid_curved_top_face (struct sync_chunk_t* in, float wx, float wy, float wz, uint8_t block_t, uint8_t offset, uint8_t* bor, uint8_t lightlevel){
	
	float lightmul = (float)lightlevel / (float)MAX_LIGHT;
	
	float y_offset = WATER_SURFACE_OFFSET;
	
	bool xp = (bor[0] == AIR_B);
	bool xm = (bor[1] == AIR_B);
	bool zp = (bor[2] == AIR_B);
	bool zm = (bor[3] == AIR_B);
	bool xpzp = (bor[4] == AIR_B);
	bool xpzm = (bor[5] == AIR_B);
	bool xmzp = (bor[6] == AIR_B);
	bool xmzm = (bor[7] == AIR_B);
	
	// Emit Vertices
	DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * ((zp || xm) || xmzp));DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
	DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * ((zp || xp) || xpzp));DFA_add(&in->vertex_array[offset], wz + BLOCK_SIZE);
	DFA_add(&in->vertex_array[offset], wx + BLOCK_SIZE);DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * ((zm || xp) || xpzm));DFA_add(&in->vertex_array[offset], wz);
	DFA_add(&in->vertex_array[offset], wx             );DFA_add(&in->vertex_array[offset], wy + BLOCK_SIZE - y_offset * ((zm || xm) || xmzm));DFA_add(&in->vertex_array[offset], wz);
	
	// Emit TexCoords
	struct blocktexdef_t tex = btd_map[block_t];
	uint8_t tex_index   = tex.index[(Y_AXIS - 1) * 2 + 1];
	uint8_t tex_index_x = tex_index % 16;
	uint8_t tex_index_y = tex_index / 16;
	DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * (tex_index_x+1));DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * (tex_index_y + 1));
	DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * tex_index_x);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * (tex_index_y + 1));
	DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * tex_index_x);DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * tex_index_y);
	DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * (tex_index_x+1));DFA_add(&in[0].texcrd_array[offset], TEX_SIZE * tex_index_y);
	
	for(int i = 0; i < 12; i++){DFA_add(&in[0].lightl_array[offset], lightmul);};
}

void build_chunk_mesh (struct sync_chunk_t* in, uint8_t m_level){	
	
	int chunk_x = in->_x;
	int chunk_z = in->_z;

	/*
	0->the chunk in +X direction (or NULL)
	1->the chunk in -X direction (or NULL)
	2->the chunk in +Z direction (or NULL)
	3->the chunk in -Z direction (or NULL)
	4->the chunk in +X+Z direction (or NULL)
	5->the chunk in +X-Z direction (or NULL)
	6->the chunk in -X+Z direction (or NULL)
	7->the chunk in -X-Z direction (or NULL)
	*/
	
	struct sync_chunk_t* neighbours [8];
	neighbours[0] = CLL_getDataAt(&chunk_list[0], chunk_x + 1, chunk_z);
	neighbours[1] = CLL_getDataAt(&chunk_list[0], chunk_x - 1, chunk_z);
	neighbours[2] = CLL_getDataAt(&chunk_list[0], chunk_x, chunk_z + 1);
	neighbours[3] = CLL_getDataAt(&chunk_list[0], chunk_x, chunk_z - 1);
	neighbours[4] = CLL_getDataAt(&chunk_list[0], chunk_x + 1, chunk_z + 1);
	neighbours[5] = CLL_getDataAt(&chunk_list[0], chunk_x + 1, chunk_z - 1);
	neighbours[6] = CLL_getDataAt(&chunk_list[0], chunk_x - 1, chunk_z + 1);
	neighbours[7] = CLL_getDataAt(&chunk_list[0], chunk_x - 1, chunk_z - 1);
	
	struct chunk_t* data; // Block Data from which the Mesh is built (varies depending on target (blocks or warer))
	struct chunk_t* neighbour_data [8]; // also varies depending on target
	uint8_t emit_offset; // Which mesh to write into (the target mesh)
	
	switch (m_level){
		case 1: {
			data = &in->water;
			emit_offset = 1;
			for(int i = 0; i < 8; ++i){
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
			for(int i = 0; i < 8; ++i){
				if(neighbours[i] != NULL){
					neighbour_data[i] = &neighbours[i]->data;
				} else{
					neighbour_data[i] = NULL;
				}
			}
			break;
		}
	}
	
	emit_offset = emit_offset * 2 + !in->rendermesh;
	
	uint8_t border_block_type [8];
	void load_borders (int x, int y, int z){ // This method gets the bordering blocks of the block at x,y,z (including corner pieces) 
				
		bool xc_p, xc_m, zc_p, zc_m;
		xc_p = (x+1 >= CHUNK_SIZE);
		xc_m = (x-1 < 0);
		zc_p = (z+1 >= CHUNK_SIZE);
		zc_m = (z-1 < 0);
		
		uint8_t block_type = 0;

		if(xc_p){
			if(neighbour_data[0] != NULL)
				block_type = neighbour_data[0]->block_data[ATBLOCK(0, y, z)];
		}else{
			block_type = data->block_data[ATBLOCK(x+1, y, z)];
		}
		border_block_type [0] = block_type;

		if(xc_m){
			if(neighbour_data[1] != NULL)
				block_type = neighbour_data[1]->block_data[ATBLOCK(CHUNK_SIZE-1, y, z)];
		}else{
			block_type = data->block_data[ATBLOCK(x-1, y, z)];
		}
		border_block_type [1] = block_type;

		if(zc_p){
			if(neighbour_data[2] != NULL)
				block_type = neighbour_data[2]->block_data[ATBLOCK(x, y, 0)];
		}else{
			block_type = data->block_data[ATBLOCK(x, y, z+1)];
		}
		border_block_type [2] = block_type;

		if(zc_m){
			if(neighbour_data[3] != NULL)
				block_type = neighbour_data[3]->block_data[ATBLOCK(x, y, CHUNK_SIZE-1)];
		}else{
			block_type = data->block_data[ATBLOCK(x, y, z-1)];
		}
		border_block_type [3] = block_type;
		
		/* Corner Blocks (Chunk Borders are the worst thing EVVEEEEER)*/
		
		//X+Z+
		int _x = x + 1;
		int _z = z + 1;
		struct chunk_t* from_chunk;
		
		if(xc_p){
			_x = 0;
			from_chunk = neighbour_data[0];
			
			if(zc_p){
				_z = 0;
				from_chunk = neighbour_data[4];
			}
			
		}else{
			from_chunk = data;
			if(zc_p){
				_z = 0;
				from_chunk = neighbour_data[2];
			}
		}
		if(from_chunk != NULL)
			border_block_type[4] = from_chunk->block_data[ATBLOCK(_x, y, _z)];
		
		//X+Z-
		_x = x + 1;
		_z = z - 1;
		if(xc_p){
			_x = 0;
			from_chunk = neighbour_data[0];
			
			if(zc_m){
				_z = CHUNK_SIZE - 1;
				from_chunk = neighbour_data[5];
			}
			
		}else{
			from_chunk = data;
			if(zc_m){
				_z = CHUNK_SIZE - 1;
				from_chunk = neighbour_data[3];
			}
		}
		if(from_chunk != NULL)
			border_block_type[5] = from_chunk->block_data[ATBLOCK(_x, y, _z)];
		
		//X-Z+
		_x = x - 1;
		_z = z + 1;
		if(xc_m){
			_x = CHUNK_SIZE - 1;
			from_chunk = neighbour_data[1];
			
			if(zc_p){
				_z = 0;
				from_chunk = neighbour_data[6];
			}
			
		}else{
			from_chunk = data;
			if(zc_p){
				_z = 0;
				from_chunk = neighbour_data[2];
			}
		}
		if(from_chunk != NULL)
			border_block_type[6] = from_chunk->block_data[ATBLOCK(_x, y, _z)];
		
		//X-Z-
		_x = x - 1;
		_z = z - 1;
		if(xc_m){
			_x = CHUNK_SIZE - 1;
			from_chunk = neighbour_data[1];
			
			if(zc_m){
				_z = CHUNK_SIZE - 1;
				from_chunk = neighbour_data[7];
			}
			
		}else{
			from_chunk = data;
			if(zc_m){
				_z = CHUNK_SIZE - 1;
				from_chunk = neighbour_data[3];
			}
		}
		if(from_chunk != NULL)
			border_block_type[7] = from_chunk->block_data[ATBLOCK(_x, y, _z)];
		
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
	
	// Chunk Offset Coordinates (Are Used to Calculate the World Position of each Block)
	float c_x_off = (float)chunk_x - 0.5f;
	float c_z_off = (float)chunk_z - 0.5f;
	
	DFA_clear(&in->vertex_array[emit_offset]);
	DFA_clear(&in->texcrd_array[emit_offset]);
	DFA_clear(&in->lightl_array[emit_offset]);
	
	for(int x = 0; x < CHUNK_SIZE;++x){
		for(int y = 0; y < CHUNK_SIZE_Y;++y){
			for(int z = 0; z < CHUNK_SIZE;++z){
				
				uint8_t block_t = data->block_data[ATBLOCK(x,y,z)]; // The Block Type at the current position
				
				bool emitter_cond; // Depending on the target, the condition for spawning a block face may vary
				switch(m_level){
					case 1: {
						emitter_cond = block_t == WATER_B; // Only Water Blocks can spawn faces
						break;
					}
					default: {
						emitter_cond = block_t != 0; // Any Non-Air Block can spawn faces
						break;
					}
				}
				
				if (emitter_cond){
					float wx,wy,wz; // The World-Space Position
					wx = (x + c_x_off * CHUNK_SIZE) * BLOCK_SIZE;
					wz = (z + c_z_off * CHUNK_SIZE) * BLOCK_SIZE;
					wy = y * BLOCK_SIZE;
					
					bool shortened_block = is_shortened (x, y, z); // Determine if the top side of the block is shifted down
					
					if(!((y + 1) == CHUNK_SIZE_Y)){
						if(data->block_data[ATBLOCK(x,y+1,z)] == 0){
							emit_face(in, wx, wy, wz, Y_AXIS, true, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y+1,z)]);
						}else if (data->block_data[ATBLOCK(x,y+1,z)] != WATER_B){ // Special Case for Water only
							if(m_level == 1){
								load_borders (x, y+1, z);
								emit_fluid_curved_top_face (in, wx, wy, wz, block_t, emit_offset, border_block_type, in->light.block_data[ATBLOCK(x,y,z)]);
							}
						}
					}else{
						emit_face(in, wx, wy, wz, Y_AXIS, true, block_t, emit_offset, shortened_block, MAX_LIGHT);
					}
					
					if(!((y - 1) < 0)){
						if(data->block_data[ATBLOCK(x,y-1,z)] == 0){
							emit_face(in, wx, wy, wz, Y_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y-1,z)]);
						}
					}else{
						//emit_face(in, wx, wy, wz, Y_AXIS, false, block_t);
					}
					
					if(!((x + 1) == CHUNK_SIZE)){
						if(data->block_data[ATBLOCK(x+1,y,z)] == 0){
							emit_face(in, wx, wy, wz, X_AXIS, true, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x+1,y,z)]);
						}
					}else{
						if(neighbour_data[0] != NULL)
							if(neighbour_data[0]->block_data[ATBLOCK(0,y,z)] == 0){
								emit_face(in, wx, wy, wz, X_AXIS, true, block_t, emit_offset, shortened_block, neighbours[0]->light.block_data[ATBLOCK(0,y,z)]);
							}
					}
					
					if(!((x - 1) < 0)){
						if(data->block_data[ATBLOCK(x-1,y,z)] == 0){
							emit_face(in, wx, wy, wz, X_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x-1,y,z)]);
						}
					}else{
						if(neighbour_data[1] != NULL){
							if(neighbour_data[1]->block_data[ATBLOCK(CHUNK_SIZE-1,y,z)] == 0){
								emit_face(in, wx, wy, wz, X_AXIS, false, block_t, emit_offset, shortened_block, neighbours[1]->light.block_data[ATBLOCK(CHUNK_SIZE-1,y,z)]);
							}
						}
					}
					
					if(!((z + 1) == CHUNK_SIZE)){
						if(data->block_data[ATBLOCK(x,y,z+1)] == 0){
							emit_face(in, wx, wy, wz, Z_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y,z+1)]);
						}
					}else{
						if(neighbour_data[2] != NULL){
							if(neighbour_data[2]->block_data[ATBLOCK(x,y,0)] == 0){
								emit_face(in, wx, wy, wz, Z_AXIS, false, block_t, emit_offset, shortened_block, neighbours[2]->light.block_data[ATBLOCK(x,y,0)]);
							}
						}
					}
					
					if(!((z - 1) < 0)){
						if(data->block_data[ATBLOCK(x,y,z-1)] == 0){
							emit_face(in, wx, wy, wz, Z_AXIS, true, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y,z-1)]);
						}
					}else{
						if(neighbour_data[3] != NULL){
							if(neighbour_data[3]->block_data[ATBLOCK(x,y,CHUNK_SIZE-1)] == 0){
								emit_face(in, wx, wy, wz, Z_AXIS, true, block_t, emit_offset, shortened_block, neighbours[3]->light.block_data[ATBLOCK(x,y,CHUNK_SIZE-1)]);
							}
						}
					}
				}
				
			}
		}
	}
}

void* chunk_thread_func (){
	pthread_mutex_lock(&chunk_builder_mutex);
	while(is_running){
		pthread_cond_wait(&chunk_builder_lock, &chunk_builder_mutex);
		
		lock_list(&chunk_list[1]);
			
		struct CLL_element* p;
		for(p = chunk_list[1].first; p != NULL; p = p->nxt){
			chunk_data_sync(p->data);
			
			calculate_light(p->data, &skylight_func);
			
			chunk_data_unsync(p->data);
		}
		for(p = chunk_list[1].first; p != NULL; p = p->nxt){
			chunk_data_sync(p->data);
			
			build_chunk_mesh(p->data, 0); // Build Block Mesh
			build_chunk_mesh(p->data, 1); // Build Water Mesh
			
			p->data->rendermesh = !p->data->rendermesh;
			
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
