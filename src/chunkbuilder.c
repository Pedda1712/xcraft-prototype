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
#include <worldsave.h>

#define Y_AXIS 1
#define X_AXIS 2
#define Z_AXIS 3

#define TEX_SIZE (1.0f/16.0f)

#include <blocktexturedef.h>

#include <globallists.h>
#include <genericlist.h>

#include <GL/glew.h>

#define NUM_BUILDER_THREADS 2

int builder_list [NUM_BUILDER_THREADS] = {
	1,3
};

pthread_t builder_thread [NUM_BUILDER_THREADS];
pthread_cond_t chunk_builder_lock;

bool is_running;

void emit_face (struct sync_chunk_t* in, float wx, float wy, float wz, uint8_t axis, bool mirorred, uint8_t block_t, uint8_t offset, bool shortened, uint8_t lightlevel){

	float lightmul = (Y_AXIS == axis) ? ( mirorred ? 1.0f : 0.45f) : 0.8f; // Light Level depending on block side
	lightmul *= (Z_AXIS == axis) ? 0.7f : 1.0f;
	lightmul *= ((float)lightlevel / (float)MAX_LIGHT);
	
	float y_offset = 0.0f;
	if(offset >= 2)
		y_offset = data_unique_SURFACE_OFFSET;

	float vertex_coordinates [12];
	float tex_coordinates [8];
	
	switch(axis){ // Emit the Vertex Positions
		
		case X_AXIS:{
			if(mirorred){
				vertex_coordinates[0] = wx + BLOCK_SIZE * mirorred;vertex_coordinates[1] = wy             ;vertex_coordinates[2] = wz + BLOCK_SIZE;
				vertex_coordinates[3] = wx + BLOCK_SIZE * mirorred;vertex_coordinates[4] = wy             ;vertex_coordinates[5] = wz;
				vertex_coordinates[6] = wx + BLOCK_SIZE * mirorred;vertex_coordinates[7] = wy + BLOCK_SIZE - y_offset * shortened;vertex_coordinates[8] = wz;
				vertex_coordinates[9] = wx + BLOCK_SIZE * mirorred;vertex_coordinates[10] = wy + BLOCK_SIZE - y_offset * shortened;vertex_coordinates[11] = wz + BLOCK_SIZE;
			}else{
				vertex_coordinates[0] = wx + BLOCK_SIZE * mirorred;vertex_coordinates[1] = wy             ;vertex_coordinates[2] = wz;
				vertex_coordinates[3] = wx + BLOCK_SIZE * mirorred;vertex_coordinates[4] = wy             ;vertex_coordinates[5] = wz + BLOCK_SIZE;
				vertex_coordinates[6] = wx + BLOCK_SIZE * mirorred;vertex_coordinates[7] = wy + BLOCK_SIZE - y_offset * shortened;vertex_coordinates[8] = wz + BLOCK_SIZE;
				vertex_coordinates[9] = wx + BLOCK_SIZE * mirorred;vertex_coordinates[10] = wy + BLOCK_SIZE - y_offset * shortened;vertex_coordinates[11] = wz;
			}
		break;}
		
		case Y_AXIS:{

			if(mirorred){
				vertex_coordinates[0] = wx             ;vertex_coordinates[1] = wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened;vertex_coordinates[2] = wz + BLOCK_SIZE;
				vertex_coordinates[3] = wx + BLOCK_SIZE;vertex_coordinates[4] = wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened;vertex_coordinates[5] = wz + BLOCK_SIZE;
				vertex_coordinates[6] = wx + BLOCK_SIZE;vertex_coordinates[7] = wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened;vertex_coordinates[8] = wz;
				vertex_coordinates[9] = wx             ;vertex_coordinates[10] = wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened;vertex_coordinates[11] = wz;
			}else{
				vertex_coordinates[0] = wx             ;vertex_coordinates[1] = wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened;vertex_coordinates[2] = wz;
				vertex_coordinates[3] = wx + BLOCK_SIZE;vertex_coordinates[4] = wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened;vertex_coordinates[5] = wz;
				vertex_coordinates[6] = wx + BLOCK_SIZE;vertex_coordinates[7] = wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened;vertex_coordinates[8] = wz + BLOCK_SIZE;
				vertex_coordinates[9] = wx             ;vertex_coordinates[10] = wy + BLOCK_SIZE * mirorred - y_offset * mirorred * shortened;vertex_coordinates[11] = wz + BLOCK_SIZE;
			}
		break;}
		
		default:{ // Z_AXIS
			if(mirorred){
				vertex_coordinates[0] = wx + BLOCK_SIZE;vertex_coordinates[1] = wy             ;vertex_coordinates[2] = wz + BLOCK_SIZE * !mirorred;
				vertex_coordinates[3] = wx             ;vertex_coordinates[4] = wy             ;vertex_coordinates[5] = wz + BLOCK_SIZE * !mirorred;
				vertex_coordinates[6] = wx             ;vertex_coordinates[7] = wy + BLOCK_SIZE - y_offset * shortened;vertex_coordinates[8] = wz + BLOCK_SIZE * !mirorred;
				vertex_coordinates[9] = wx + BLOCK_SIZE;vertex_coordinates[10] = wy + BLOCK_SIZE - y_offset * shortened;vertex_coordinates[11] = wz + BLOCK_SIZE * !mirorred;
			}else{
				vertex_coordinates[0] = wx             ;vertex_coordinates[1] = wy             ;vertex_coordinates[2] = wz + BLOCK_SIZE * !mirorred;
				vertex_coordinates[3] = wx + BLOCK_SIZE;vertex_coordinates[4] = wy             ;vertex_coordinates[5] = wz + BLOCK_SIZE * !mirorred;
				vertex_coordinates[6] = wx + BLOCK_SIZE;vertex_coordinates[7] = wy + BLOCK_SIZE - y_offset * shortened;vertex_coordinates[8] = wz + BLOCK_SIZE * !mirorred;
				vertex_coordinates[9] = wx             ;vertex_coordinates[10] = wy + BLOCK_SIZE - y_offset * shortened;vertex_coordinates[11] = wz + BLOCK_SIZE * !mirorred;
			}
		break;}
		
	}
	
	struct blocktexdef_t tex = btd_map[block_t];
	uint8_t tex_index   = tex.index[(axis - 1) * 2 + mirorred];
	uint8_t tex_index_x = tex_index % 16;
	uint8_t tex_index_y = tex_index / 16;
	tex_coordinates[0] = TEX_SIZE * (tex_index_x+1);tex_coordinates[1] = TEX_SIZE * (tex_index_y + 1);
	tex_coordinates[2] = TEX_SIZE * tex_index_x;tex_coordinates[3] = TEX_SIZE * (tex_index_y + 1);
	tex_coordinates[4] = TEX_SIZE * tex_index_x;tex_coordinates[5] = TEX_SIZE * tex_index_y;
	tex_coordinates[6] = TEX_SIZE * (tex_index_x+1);tex_coordinates[7] = TEX_SIZE * tex_index_y;
	
	for(int i = 0; i < 4; i++){
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[0 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[1 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[2 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], tex_coordinates[0 + i * 2]);
		DFA_add(&in[0].mesh_buffer[offset], tex_coordinates[1 + i * 2]);
		DFA_add(&in[0].mesh_buffer[offset], lightmul);
	}
}

void emit_fluid_curved_top_face (struct sync_chunk_t* in, float wx, float wy, float wz, uint8_t block_t, uint8_t offset, uint8_t* bor, uint8_t lightlevel){
	
	float lightmul = (float)lightlevel / (float)MAX_LIGHT;
	
	float y_offset = data_unique_SURFACE_OFFSET;
	
	bool xp = (bor[0] == AIR_B);
	bool xm = (bor[1] == AIR_B);
	bool zp = (bor[2] == AIR_B);
	bool zm = (bor[3] == AIR_B);
	bool xpzp = (bor[4] == AIR_B);
	bool xpzm = (bor[5] == AIR_B);
	bool xmzp = (bor[6] == AIR_B);
	bool xmzm = (bor[7] == AIR_B);
	
	float vertex_coordinates [12];
	float tex_coordinates [8];
	
	// Emit Vertices
	vertex_coordinates[0] = wx             ;vertex_coordinates[1] = wy + BLOCK_SIZE - y_offset * ((zp || xm) || xmzp);vertex_coordinates[2] = wz + BLOCK_SIZE;
	vertex_coordinates[3] = wx + BLOCK_SIZE;vertex_coordinates[4] = wy + BLOCK_SIZE - y_offset * ((zp || xp) || xpzp);vertex_coordinates[5] = wz + BLOCK_SIZE;
	vertex_coordinates[6] = wx + BLOCK_SIZE;vertex_coordinates[7] = wy + BLOCK_SIZE - y_offset * ((zm || xp) || xpzm);vertex_coordinates[8] = wz;
	vertex_coordinates[9] = wx             ;vertex_coordinates[10] = wy + BLOCK_SIZE - y_offset * ((zm || xm) || xmzm);vertex_coordinates[11] = wz;
	
	// Emit TexCoords
	struct blocktexdef_t tex = btd_map[block_t];
	uint8_t tex_index   = tex.index[(Y_AXIS - 1) * 2 + 1];
	uint8_t tex_index_x = tex_index % 16;
	uint8_t tex_index_y = tex_index / 16;
	tex_coordinates[0] = TEX_SIZE * (tex_index_x+1);tex_coordinates[1] = TEX_SIZE * (tex_index_y + 1);
	tex_coordinates[2] = TEX_SIZE * tex_index_x;tex_coordinates[3] = TEX_SIZE * (tex_index_y + 1);
	tex_coordinates[4] = TEX_SIZE * tex_index_x;tex_coordinates[5] = TEX_SIZE * tex_index_y;
	tex_coordinates[6] = TEX_SIZE * (tex_index_x+1);tex_coordinates[7] = TEX_SIZE * tex_index_y;
	
	for(int i = 0; i < 4; i++){
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[0 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[1 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[2 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], tex_coordinates[0 + i * 2]);
		DFA_add(&in[0].mesh_buffer[offset], tex_coordinates[1 + i * 2]);
		DFA_add(&in[0].mesh_buffer[offset], lightmul);
	}
}

void standard_mesh_routine (struct sync_chunk_t* in, struct chunk_t* data, struct sync_chunk_t** neighbours,struct chunk_t** neighbour_data ,uint32_t emit_offset, uint32_t m_level){
	
	int chunk_x = in->_x;
	int chunk_z = in->_z;
	
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
				if(data->block_data[ATBLOCK(x, y+1, z)] == data_unique_B){
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
	
	for(int x = 0; x < CHUNK_SIZE;++x){
		for(int y = 0; y < CHUNK_SIZE_Y;++y){
			for(int z = 0; z < CHUNK_SIZE;++z){
				
				uint8_t block_t = data->block_data[ATBLOCK(x,y,z)]; // The Block Type at the current position
				
				bool emitter_cond; // Depending on the target, the condition for spawning a block face may vary
				switch(m_level){
					case 1: {
						emitter_cond = block_t == data_unique_B; // Only data_unique Blocks can spawn faces
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
						}else if (data->block_data[ATBLOCK(x,y+1,z)] != data_unique_B){ // Special Case for data_unique only
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

void emit_x (struct sync_chunk_t* in, float wx, float wy, float wz, uint8_t axis, bool mirorred, uint8_t block_t, uint8_t offset, bool shortened, uint8_t lightlevel){
	
	float lightmul = (float)lightlevel / (float)MAX_LIGHT;
	
	float vertex_coordinates[24];
	float tex_coordinates[16];
	
	vertex_coordinates[0] = wx             ;vertex_coordinates[1] = wy              ;vertex_coordinates[2] = wz;
	vertex_coordinates[3] = wx + BLOCK_SIZE;vertex_coordinates[4] = wy              ;vertex_coordinates[5] = wz + BLOCK_SIZE;
	vertex_coordinates[6] = wx + BLOCK_SIZE;vertex_coordinates[7] = wy + BLOCK_SIZE;vertex_coordinates[8] = wz + BLOCK_SIZE;
	vertex_coordinates[9] = wx             ;vertex_coordinates[10] = wy + BLOCK_SIZE ;vertex_coordinates[11] = wz;
	
	vertex_coordinates[12] = wx             ;vertex_coordinates[13] = wy             ;vertex_coordinates[14] = wz + BLOCK_SIZE;
	vertex_coordinates[15] = wx + BLOCK_SIZE;vertex_coordinates[16] = wy             ;vertex_coordinates[17] = wz;
	vertex_coordinates[18] = wx + BLOCK_SIZE;vertex_coordinates[19] = wy + BLOCK_SIZE;vertex_coordinates[20] = wz;
	vertex_coordinates[21] = wx             ;vertex_coordinates[22] = wy + BLOCK_SIZE;vertex_coordinates[23] = wz + BLOCK_SIZE;
	
	struct blocktexdef_t tex = btd_map[block_t];
	uint8_t tex_index   = tex.index[0];
	uint8_t tex_index_x = tex_index % 16;
	uint8_t tex_index_y = tex_index / 16;
	tex_coordinates[0] = TEX_SIZE * (tex_index_x+1);tex_coordinates[1] = TEX_SIZE * (tex_index_y + 1);
	tex_coordinates[2] = TEX_SIZE * tex_index_x;tex_coordinates[3] = TEX_SIZE * (tex_index_y + 1);
	tex_coordinates[4] = TEX_SIZE * tex_index_x;tex_coordinates[5] = TEX_SIZE * tex_index_y;
	tex_coordinates[6] = TEX_SIZE * (tex_index_x+1);tex_coordinates[7] = TEX_SIZE * tex_index_y;
	tex_coordinates[8] = TEX_SIZE * (tex_index_x+1);tex_coordinates[9] = TEX_SIZE * (tex_index_y + 1);
	tex_coordinates[10] = TEX_SIZE * tex_index_x;tex_coordinates[11] = TEX_SIZE * (tex_index_y + 1);
	tex_coordinates[12] = TEX_SIZE * tex_index_x;tex_coordinates[13] = TEX_SIZE * tex_index_y;
	tex_coordinates[14] = TEX_SIZE * (tex_index_x+1);tex_coordinates[15] = TEX_SIZE * tex_index_y;
	
	for(int i = 0; i < 8; i++){
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[0 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[1 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[2 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], tex_coordinates[0 + i * 2]);
		DFA_add(&in[0].mesh_buffer[offset], tex_coordinates[1 + i * 2]);
		DFA_add(&in[0].mesh_buffer[offset], lightmul);
	}
} 

void plant_mesh_routine (struct sync_chunk_t* in, struct chunk_t* data, struct sync_chunk_t** neighbours,struct chunk_t** neighbour_data ,uint32_t emit_offset, uint32_t m_level){
	
	int chunk_x = in->_x;
	int chunk_z = in->_z;
	
	float c_x_off = (float)chunk_x - 0.5f;
	float c_z_off = (float)chunk_z - 0.5f;
	
	for(int x = 0; x < CHUNK_SIZE;++x){
		for(int y = 0; y < CHUNK_SIZE_Y;++y){
			for(int z = 0; z < CHUNK_SIZE;++z){
				
				float wx,wy,wz; // The World-Space Position
				wx = (x + c_x_off * CHUNK_SIZE) * BLOCK_SIZE;
				wz = (z + c_z_off * CHUNK_SIZE) * BLOCK_SIZE;
				wy = y * BLOCK_SIZE;
				
				uint8_t block_t = data->block_data[ATBLOCK(x,y,z)]; // The Block Type at the current position
				
				if(block_t >= XGRASS_B && block_t <= XFLOW6_B){
					emit_x(in, wx, wy, wz, Y_AXIS, true, block_t, emit_offset, false, in->light.block_data[ATBLOCK(x,y,z)]);
				}
			}
		}
	}
}

void build_chunk_mesh (struct sync_chunk_t* in, uint8_t m_level, void (*routine) (struct sync_chunk_t*, struct chunk_t*, struct sync_chunk_t**,struct chunk_t** ,uint32_t, uint32_t)){	
	
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

		case 0: {
			data = &in->data;
			for(int i = 0; i < 8; ++i){
				if(neighbours[i] != NULL){
					neighbour_data[i] = &neighbours[i]->data;
				} else{
					neighbour_data[i] = NULL;
				}
			}
			break;
		}
		default: {
			data = &in->data_unique;
			for(int i = 0; i < 8; ++i){
				if(neighbours[i] != NULL){
					neighbour_data[i] = &neighbours[i]->data_unique;
				}else{
					neighbour_data[i] = NULL;
				}
			}
			break;
		}
	}
	
	emit_offset = m_level;
	emit_offset = emit_offset * 2 + !in->updatemesh;
	
	DFA_clear(&in->mesh_buffer[emit_offset]);
	
	(*routine)(in, data, neighbours, neighbour_data, emit_offset, m_level);
		
}

void do_updates_for_list (struct CLL* list){
	lock_list(list);
	
	struct CLL_element* p;
	for(p = list->first; p != NULL; p = p->nxt){
		

		calculate_light(p->data, &skylight_func, true);
		calculate_light(p->data, &blocklight_func, false);

		chunk_data_unsync(p->data);
	}
	
	for(p = list->first; p != NULL; p = p->nxt){
		chunk_data_sync(p->data);
		
		build_chunk_mesh(p->data, 0, &standard_mesh_routine); // Build Block Mesh
		build_chunk_mesh(p->data, 1, &standard_mesh_routine); // Build data_unique Mesh
		build_chunk_mesh(p->data, 2, &plant_mesh_routine); // Build data_unique Mesh
		
		p->data->updatemesh = !p->data->updatemesh;
		p->data->vbo_update[0] = true;
		p->data->vbo_update[1] = true;
		p->data->vbo_update[2] = true;
		
		chunk_data_unsync(p->data);
	}
	CLL_freeList(list);
	unlock_list(list);
}

void* builder_thread_func (void* arg){
	
	int buildlist = *(int*)arg;
	
	pthread_mutex_t chunk_builder_mutex;
	pthread_mutex_init(&chunk_builder_mutex,NULL);
	
	pthread_mutex_lock(&chunk_builder_mutex);
	while(is_running){
		pthread_cond_wait(&chunk_builder_lock, &chunk_builder_mutex);
		
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		do_updates_for_list(&chunk_list[buildlist]);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	}
	pthread_mutex_unlock(&chunk_builder_mutex);
	pthread_mutex_destroy(&chunk_builder_mutex);
	return 0;
}

bool initialize_builder_thread (){
	
	for(int i = 0; i < NUMLISTS; i++){chunk_list[i] = CLL_init();}
	
	pthread_cond_init(&chunk_builder_lock, NULL);
	
	for(int x = -WORLD_RANGE; x <= WORLD_RANGE; ++x){
		for(int z = -WORLD_RANGE; z <= WORLD_RANGE;++z){
			struct sync_chunk_t* temp = malloc (sizeof(struct sync_chunk_t));
			if(pthread_mutex_init(&temp->c_mutex, NULL) != 0){return false;}

			for(int i = 0; i < MESH_LEVELS; ++i){
				temp->mesh_buffer[i] = DFA_init();
				glGenBuffers(1, &temp->mesh_vbo[0]);
				glGenBuffers(1, &temp->mesh_vbo[1]);
				glGenBuffers(1, &temp->mesh_vbo[2]);
			}
			
			temp->_x = x;
 			temp->_z = z;
			temp->initialized = false;
			
			CLL_add(&chunk_list[0], temp);
			
			temp->lightlist = GLL_init();
			
			temp->vbo_update[0] = false;
			temp->vbo_update[1] = false;
			
			temp->verts[0] = 0;
			temp->verts[1] = 0;
		}
	}
	
	is_running = true;
	for (int i = 0; i < NUM_BUILDER_THREADS; i++){
		pthread_create(&builder_thread[i], NULL, builder_thread_func, &builder_list[i]);
	}
	return true;
}

void terminate_builder_thread (){
	
	is_running = false;

	for(int i = 0; i < NUM_BUILDER_THREADS; i++){pthread_cancel(builder_thread[i]);}
	
	trigger_builder_update();

	for(int i = 0; i < NUM_BUILDER_THREADS;i++){pthread_join(builder_thread[i], NULL);}
		
	for(struct CLL_element* e = chunk_list[0].first; e != NULL; e = e->nxt){
		dump_chunk(e->data);
		GLL_free_rec (&e->data->lightlist);
		GLL_destroy(&e->data->lightlist);
	}	
	
	printf("Freeing Chunk Memory ...\n");
	for(int i = 0; i < NUMLISTS;i++){lock_list(&chunk_list[i]);}
	CLL_freeListAndData(&chunk_list[0]);
	for(int i = 1; i < NUMLISTS;i++){CLL_freeList(&chunk_list[i]);}
	for(int i = 0; i < NUMLISTS;i++){CLL_destroyList(&chunk_list[i]);}
	
	pthread_cond_destroy(&chunk_builder_lock);
}

void chunk_data_sync  (struct sync_chunk_t* c){
	pthread_mutex_lock(&c->c_mutex);
}

void chunk_data_unsync(struct sync_chunk_t* c){
	pthread_mutex_unlock(&c->c_mutex);
}

void trigger_builder_update (){
	pthread_cond_broadcast(&chunk_builder_lock);
}
