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

enum AXIS_IDENTIFIERS {
	Y_AXIS=1,
	X_AXIS,
	Z_AXIS
};

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

static void emit_face (struct sync_chunk_t* in, float wx, float wy, float wz, uint8_t axis, bool mirorred, uint8_t block_t, uint8_t offset, bool shortened, uint8_t lightlevel, uint16_t surrounding [3] [3] [3]){

	float lightmul = (Y_AXIS == axis) ? ( mirorred ? 1.0f : 0.45f) : 0.8f; // Light Level depending on block side
	lightmul *= (Z_AXIS == axis) ? 0.7f : 1.0f;
	float llevel = lightmul * ((float)lightlevel / (float)MAX_LIGHT);
	
	float y_offset = 0.0f;
	if(offset >= 2)
		y_offset = WATER_SURFACE_OFFSET;

	float vertex_coordinates [12];
	float tex_coordinates [8];
	float light_buffer [4] = {llevel,llevel,llevel,llevel};
	
	switch(axis){ // Emit the Vertex Positions
		
		case X_AXIS:{
			if(mirorred){
				vertex_coordinates[0] = wx + 1.0f * mirorred;vertex_coordinates[1] = wy             ;vertex_coordinates[2] = wz + 1.0f;
				vertex_coordinates[3] = wx + 1.0f * mirorred;vertex_coordinates[4] = wy             ;vertex_coordinates[5] = wz;
				vertex_coordinates[6] = wx + 1.0f * mirorred;vertex_coordinates[7] = wy + 1.0f - y_offset * shortened;vertex_coordinates[8] = wz;
				vertex_coordinates[9] = wx + 1.0f * mirorred;vertex_coordinates[10] = wy + 1.0f - y_offset * shortened;vertex_coordinates[11] = wz + 1.0f;
			}else{
				vertex_coordinates[0] = wx + 1.0f * mirorred;vertex_coordinates[1] = wy             ;vertex_coordinates[2] = wz;
				vertex_coordinates[3] = wx + 1.0f * mirorred;vertex_coordinates[4] = wy             ;vertex_coordinates[5] = wz + 1.0f;
				vertex_coordinates[6] = wx + 1.0f * mirorred;vertex_coordinates[7] = wy + 1.0f - y_offset * shortened;vertex_coordinates[8] = wz + 1.0f;
				vertex_coordinates[9] = wx + 1.0f * mirorred;vertex_coordinates[10] = wy + 1.0f - y_offset * shortened;vertex_coordinates[11] = wz;
			}
		break;}
		
		case Y_AXIS:{

			if(mirorred){
				vertex_coordinates[0] = wx             ;vertex_coordinates[1] = wy + 1.0f * mirorred - y_offset * mirorred * shortened;vertex_coordinates[2] = wz + 1.0f;
				vertex_coordinates[3] = wx + 1.0f;vertex_coordinates[4] = wy + 1.0f * mirorred - y_offset * mirorred * shortened;vertex_coordinates[5] = wz + 1.0f;
				vertex_coordinates[6] = wx + 1.0f;vertex_coordinates[7] = wy + 1.0f * mirorred - y_offset * mirorred * shortened;vertex_coordinates[8] = wz;
				vertex_coordinates[9] = wx             ;vertex_coordinates[10] = wy + 1.0f * mirorred - y_offset * mirorred * shortened;vertex_coordinates[11] = wz;
			}else{
				vertex_coordinates[0] = wx             ;vertex_coordinates[1] = wy + 1.0f * mirorred - y_offset * mirorred * shortened;vertex_coordinates[2] = wz;
				vertex_coordinates[3] = wx + 1.0f;vertex_coordinates[4] = wy + 1.0f * mirorred - y_offset * mirorred * shortened;vertex_coordinates[5] = wz;
				vertex_coordinates[6] = wx + 1.0f;vertex_coordinates[7] = wy + 1.0f * mirorred - y_offset * mirorred * shortened;vertex_coordinates[8] = wz + 1.0f;
				vertex_coordinates[9] = wx             ;vertex_coordinates[10] = wy + 1.0f * mirorred - y_offset * mirorred * shortened;vertex_coordinates[11] = wz + 1.0f;
			}
		break;}
		
		default:{ // Z_AXIS
			if(mirorred){
				vertex_coordinates[0] = wx + 1.0f;vertex_coordinates[1] = wy             ;vertex_coordinates[2] = wz + 1.0f * !mirorred;
				vertex_coordinates[3] = wx             ;vertex_coordinates[4] = wy             ;vertex_coordinates[5] = wz + 1.0f * !mirorred;
				vertex_coordinates[6] = wx             ;vertex_coordinates[7] = wy + 1.0f - y_offset * shortened;vertex_coordinates[8] = wz + 1.0f * !mirorred;
				vertex_coordinates[9] = wx + 1.0f;vertex_coordinates[10] = wy + 1.0f - y_offset * shortened;vertex_coordinates[11] = wz + 1.0f * !mirorred;
			}else{
				vertex_coordinates[0] = wx             ;vertex_coordinates[1] = wy             ;vertex_coordinates[2] = wz + 1.0f * !mirorred;
				vertex_coordinates[3] = wx + 1.0f;vertex_coordinates[4] = wy             ;vertex_coordinates[5] = wz + 1.0f * !mirorred;
				vertex_coordinates[6] = wx + 1.0f;vertex_coordinates[7] = wy + 1.0f - y_offset * shortened;vertex_coordinates[8] = wz + 1.0f * !mirorred;
				vertex_coordinates[9] = wx             ;vertex_coordinates[10] = wy + 1.0f - y_offset * shortened;vertex_coordinates[11] = wz + 1.0f * !mirorred;
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
	
	if(SMOOTH_LIGHTING_ENABLED){
		switch(axis){
			case X_AXIS:{
				if(mirorred){
					light_buffer[0] = 0.0f + lightlevel;
					light_buffer[0] += surrounding[2][0][1];light_buffer[0] += surrounding[2][0][2];light_buffer[0] += surrounding[2][1][2];
					
					light_buffer[1] = 0.0f + lightlevel;
					light_buffer[1] += surrounding[2][0][1];light_buffer[1] += surrounding[2][0][0];light_buffer[1] += surrounding[2][1][0];
					
					light_buffer[2] = 0.0f + lightlevel;
					light_buffer[2] += surrounding[2][1][0];light_buffer[2] += surrounding[2][2][0];light_buffer[2] += surrounding[2][2][1];
					
					light_buffer[3] = 0.0f + lightlevel;
					light_buffer[3] += surrounding[2][1][2];light_buffer[3] += surrounding[2][2][2];light_buffer[3] += surrounding[2][2][1];
				}else{
					light_buffer[1] = 0.0f + lightlevel;
					light_buffer[1] += surrounding[0][0][1];light_buffer[1] += surrounding[0][0][2];light_buffer[1] += surrounding[0][1][2];
					
					light_buffer[0] = 0.0f + lightlevel;
					light_buffer[0] += surrounding[0][0][1];light_buffer[0] += surrounding[0][0][0];light_buffer[0] += surrounding[0][1][0];
					
					light_buffer[3] = 0.0f + lightlevel;
					light_buffer[3] += surrounding[0][1][0];light_buffer[3] += surrounding[0][2][0];light_buffer[3] += surrounding[0][2][1];
					
					light_buffer[2] = 0.0f + lightlevel;
					light_buffer[2] += surrounding[0][1][2];light_buffer[2] += surrounding[0][2][2];light_buffer[2] += surrounding[0][2][1];
				}
				
				break;
			}
			case Y_AXIS:{
				
				if(mirorred){
					light_buffer[0] = 0.0f + lightlevel;
					light_buffer[0] += surrounding[0][2][2];light_buffer[0] += surrounding[1][2][2];light_buffer[0] += surrounding[0][2][1];
					
					light_buffer[1] = 0.0f + lightlevel;
					light_buffer[1] += surrounding[2][2][2];light_buffer[1] += surrounding[2][2][1];light_buffer[1] += surrounding[1][2][2];
					
					light_buffer[2] = 0.0f + lightlevel;
					light_buffer[2] += surrounding[2][2][1];light_buffer[2] += surrounding[1][2][0];light_buffer[2] += surrounding[2][2][0];
					
					light_buffer[3] = 0.0f + lightlevel;
					light_buffer[3] += surrounding[0][2][1];light_buffer[3] += surrounding[0][2][0];light_buffer[3] += surrounding[1][2][0];
				}else{
					light_buffer[3] = 0.0f + lightlevel;
					light_buffer[3] += surrounding[0][0][2];light_buffer[3] += surrounding[1][0][2];light_buffer[3] += surrounding[0][0][1];
					
					light_buffer[2] = 0.0f + lightlevel;
					light_buffer[2] += surrounding[2][0][2];light_buffer[2] += surrounding[2][0][1];light_buffer[2] += surrounding[1][0][2];
					
					light_buffer[1] = 0.0f + lightlevel;
					light_buffer[1] += surrounding[2][0][1];light_buffer[1] += surrounding[1][0][0];light_buffer[1] += surrounding[2][0][0];
					
					light_buffer[0] = 0.0f + lightlevel;
					light_buffer[0] += surrounding[0][0][1];light_buffer[0] += surrounding[0][0][0];light_buffer[0] += surrounding[1][0][0];
				}
				
				break;
			}
			default :{
				if(mirorred){
					light_buffer[0] = 0.0f + lightlevel;
					light_buffer[0] += surrounding[1][0][0];light_buffer[0] += surrounding[2][0][0];light_buffer[0] += surrounding[2][1][0];
					
					light_buffer[1] = 0.0f + lightlevel;
					light_buffer[1] += surrounding[1][0][0];light_buffer[1] += surrounding[0][0][0];light_buffer[1] += surrounding[0][1][0];
					
					light_buffer[2] = 0.0f + lightlevel;
					light_buffer[2] += surrounding[1][2][0];light_buffer[2] += surrounding[0][2][0];light_buffer[2] += surrounding[0][1][0];
					
					light_buffer[3] = 0.0f + lightlevel;
					light_buffer[3] += surrounding[1][2][0];light_buffer[3] += surrounding[2][2][0];light_buffer[3] += surrounding[2][1][0];
				}else{
					light_buffer[1] = 0.0f + lightlevel;
					light_buffer[1] += surrounding[1][0][2];light_buffer[1] += surrounding[2][0][2];light_buffer[1] += surrounding[2][1][2];
					
					light_buffer[0] = 0.0f + lightlevel;
					light_buffer[0] += surrounding[1][0][2];light_buffer[0] += surrounding[0][0][2];light_buffer[0] += surrounding[0][1][2];
					
					light_buffer[3] = 0.0f + lightlevel;
					light_buffer[3] += surrounding[1][2][2];light_buffer[3] += surrounding[0][2][2];light_buffer[3] += surrounding[0][1][2];
					
					light_buffer[2] = 0.0f + lightlevel;
					light_buffer[2] += surrounding[1][2][2];light_buffer[2] += surrounding[2][2][2];light_buffer[2] += surrounding[2][1][2];
				}
				
				break;
			}
		}
		for(int i = 0; i < 4; i++){light_buffer[i] = ((light_buffer[i] / 4.0f) / (float)MAX_LIGHT) *lightmul;}
	}
	
	for(int i = 0; i < 4; i++){
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[0 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[1 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], vertex_coordinates[2 + i * 3]);
		DFA_add(&in[0].mesh_buffer[offset], tex_coordinates[0 + i * 2]);
		DFA_add(&in[0].mesh_buffer[offset], tex_coordinates[1 + i * 2]);
		DFA_add(&in[0].mesh_buffer[offset], light_buffer[i]);
	}
}

static void emit_fluid_curved_top_face (struct sync_chunk_t* in, float wx, float wy, float wz, uint8_t block_t, uint8_t offset, uint16_t* bor, uint8_t lightlevel, bool n_logic, uint8_t comp){
	
	float lightmul = (float)lightlevel / (float)MAX_LIGHT;
	
	float y_offset = WATER_SURFACE_OFFSET;
	
	bool xp = (BLOCK_ID(bor[0]) == comp);
	bool xm = (BLOCK_ID(bor[1]) == comp);
	bool zp = (BLOCK_ID(bor[2]) == comp);
	bool zm = (BLOCK_ID(bor[3]) == comp);
	bool xpzp = (BLOCK_ID(bor[4]) == comp);
	bool xpzm = (BLOCK_ID(bor[5]) == comp);
	bool xmzp = (BLOCK_ID(bor[6]) == comp);
	bool xmzm = (BLOCK_ID(bor[7]) == comp);
	
	float vertex_coordinates [12];
	float tex_coordinates [8];
	
	bool conds [4] = {((zp || xm) || xmzp),((zp || xp) || xpzp),((zm || xp) || xpzm),((zm || xm) || xmzm)};
	if (n_logic){
		conds[0] = !((zp || xm) || xmzp);
		conds[1] = !((zp || xp) || xpzp);
		conds[2] = !((zm || xp) || xpzm);
		conds[3] = !((zm || xm) || xmzm);
	}
	
	// Emit Vertices
	vertex_coordinates[0] = wx             ;vertex_coordinates[1] = wy + 1.0f - y_offset * conds[0];vertex_coordinates[2] = wz + 1.0f;
	vertex_coordinates[3] = wx + 1.0f;vertex_coordinates[4] = wy + 1.0f - y_offset * conds[1];vertex_coordinates[5] = wz + 1.0f;
	vertex_coordinates[6] = wx + 1.0f;vertex_coordinates[7] = wy + 1.0f - y_offset * conds[2];vertex_coordinates[8] = wz;
	vertex_coordinates[9] = wx             ;vertex_coordinates[10] = wy + 1.0f - y_offset * conds[3];vertex_coordinates[11] = wz;
	
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

void load_borders (int x, int y, int z, uint16_t* border_block_type, struct chunk_t* data, struct chunk_t** neighbour_data ){ // This method gets the bordering blocks of the block at x,y,z (including corner pieces) 
	bool xc_p, xc_m, zc_p, zc_m;
	xc_p = (x+1 >= CHUNK_SIZE);
	xc_m = (x-1 < 0);
	zc_p = (z+1 >= CHUNK_SIZE);
	zc_m = (z-1 < 0);
	
	uint16_t block_type = 0;
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

void load_surrounding_data (int x, int y, int z, struct sync_chunk_t* in, uint16_t surrounding_light_data [3][3][3], struct sync_chunk_t** neighbours){

	if(SMOOTH_LIGHTING_ENABLED){
		int32_t base_chunk_x = in->_x;
		int32_t base_chunk_z = in->_z;
		int32_t current_chunk_x = base_chunk_x;
		int32_t current_chunk_z = base_chunk_z;
		struct sync_chunk_t* patient = in;
		for(int cx = x-1; cx <= x+1; cx++){
			for(int cy = y-1; cy <= y+1; cy++){
				for(int cz = z-1; cz <= z+1; cz++){
					int cs_x = cx % CHUNK_SIZE; cs_x = (cs_x >= 0) ? cs_x : (CHUNK_SIZE + cs_x);
					int cs_y = cy;
					int cs_z = cz % CHUNK_SIZE; cs_z = (cs_z >= 0) ? cs_z : (CHUNK_SIZE + cs_z);
					int new_chunk_x = (cx / 16) + base_chunk_x + ((cx >= 0) ? 0 : -1);
					int new_chunk_z = (cz / 16) + base_chunk_z + ((cz >= 0) ? 0 : -1);
					
					if(new_chunk_x != current_chunk_x || new_chunk_z != current_chunk_z){ // if we are in a different chunk now
						current_chunk_x = new_chunk_x;
						current_chunk_z = new_chunk_z;
						
						/*
							NOTE: This Abomination of an If-Statement is substantially faster that doing CLL_getDataAt ... 
						 */
						
						if(current_chunk_x > base_chunk_x && current_chunk_z > base_chunk_z)
							patient = neighbours[4];
						else if(current_chunk_x > base_chunk_x && current_chunk_z < base_chunk_z)
							patient = neighbours[5];
						else if(current_chunk_x < base_chunk_x && current_chunk_z > base_chunk_z)
							patient = neighbours[6];
						else if(current_chunk_x < base_chunk_x && current_chunk_z < base_chunk_z)
							patient = neighbours[7];
						else if(current_chunk_x > base_chunk_x && current_chunk_z == base_chunk_z)
							patient = neighbours[0];
						else if(current_chunk_x < base_chunk_x && current_chunk_z == base_chunk_z)
							patient = neighbours[1];
						else if(current_chunk_z > base_chunk_z && current_chunk_x == base_chunk_x)
							patient = neighbours[2];
						else if(current_chunk_z < base_chunk_z && current_chunk_x == base_chunk_x)
							patient = neighbours[3];
						else
							patient = in;
					}
					
					if(patient != NULL){
						if(cs_y < CHUNK_SIZE_Y){
							if(!IS_P(patient->data_unique.block_data[ATBLOCK(cs_x, cs_y, cs_z)])){
								surrounding_light_data[(cx -x)+1][(cy - y)+1][(cz- z)+1] = patient->light.block_data[ATBLOCK(cs_x, cs_y, cs_z)];
							}
							else{
								surrounding_light_data[(cx -x)+1][(cy - y)+1][(cz- z)+1] = MIN_LIGHT;
							}
							
						}
						else{
							surrounding_light_data[(cx -x)+1][(cy - y)+1][(cz- z)+1] = MAX_LIGHT;
						}
					}else{
						surrounding_light_data[(cx -x)+1][(cy - y)+1][(cz- z)+1] = MAX_LIGHT;
					}
					
				}
			}
		}
	}else{
		uint16_t llevel = in->light.block_data[ATBLOCK(x, y, z)];
		for(int j = 0; j < 3; j++)
			for(int i = 0; i < 3;i++)
				for(int h = 0; h < 3; h++)
					surrounding_light_data[j][i][h] = llevel;
	}
}

static void water_mesh_routine (struct sync_chunk_t* in, struct chunk_t* data, struct sync_chunk_t** neighbours,struct chunk_t** neighbour_data ,uint32_t emit_offset, uint32_t m_level){
	
	int chunk_x = in->_x;
	int chunk_z = in->_z;
	
	uint16_t border_block_type [8];
	
	uint16_t surrounding_light_data[3][3][3];
	
	bool is_shortened (int x, int y, int z){
		if(y + 1 > CHUNK_SIZE_Y){
			return true;
		}else{
			if(BLOCK_ID(data->block_data[ATBLOCK(x, y+1, z)]) == WATER_B){
				return false;
			}else{
				return true;
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
				
				uint8_t block_t = BLOCK_ID(data->block_data[ATBLOCK(x,y,z)]); // The Block Type at the current position
				
				bool emitter_cond = IS_TRANS(data->block_data[ATBLOCK(x,y,z)]); // Only Transparent Blocks can spawn faces
				
				if (emitter_cond){
					float wx,wy,wz; // The World-Space Position
					wx = (x + c_x_off * CHUNK_SIZE) * 1.0f;
					wz = (z + c_z_off * CHUNK_SIZE) * 1.0f;
					wy = y * 1.0f;
					
					bool shortened_block = is_shortened (x, y, z); // Determine if the top side of the block is shifted down
					
					if(!((y + 1) == CHUNK_SIZE_Y)){
						if(!IS_TRANS(data->block_data[ATBLOCK(x,y+1,z)])){
							load_borders (x, y+1, z, border_block_type, data, neighbour_data);
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_fluid_curved_top_face (in, wx, wy, wz, block_t, emit_offset, border_block_type, in->light.block_data[ATBLOCK(x,y,z)], true, WATER_B);
						}
					}else{
						load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
						emit_face(in, wx, wy, wz, Y_AXIS, true, block_t, emit_offset, shortened_block, MAX_LIGHT, surrounding_light_data);
					}
					
					if(!((y - 1) < 0)){
						if(!IS_TRANS(data->block_data[ATBLOCK(x,y-1,z)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, Y_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y-1,z)], surrounding_light_data);
						}
					}else{
						//emit_face(in, wx, wy, wz, Y_AXIS, false, block_t);
					}
					
					if(!((x + 1) == CHUNK_SIZE)){
						if(!IS_TRANS(data->block_data[ATBLOCK(x+1,y,z)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, X_AXIS, true, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x+1,y,z)], surrounding_light_data);
						}
					}else{
						if(neighbour_data[0] != NULL)
							if(!IS_TRANS(neighbour_data[0]->block_data[ATBLOCK(0,y,z)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, X_AXIS, true, block_t, emit_offset, shortened_block, neighbours[0]->light.block_data[ATBLOCK(0,y,z)], surrounding_light_data);
							}
					}
					
					if(!((x - 1) < 0)){
						if(!IS_TRANS(data->block_data[ATBLOCK(x-1,y,z)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, X_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x-1,y,z)], surrounding_light_data);
						}
					}else{
						if(neighbour_data[1] != NULL){
							if(!IS_TRANS(neighbour_data[1]->block_data[ATBLOCK(CHUNK_SIZE-1,y,z)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, X_AXIS, false, block_t, emit_offset, shortened_block, neighbours[1]->light.block_data[ATBLOCK(CHUNK_SIZE-1,y,z)], surrounding_light_data);
							}
						}
					}
					
					if(!((z + 1) == CHUNK_SIZE)){
						if(!IS_TRANS(data->block_data[ATBLOCK(x,y,z+1)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, Z_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y,z+1)], surrounding_light_data);
						}
					}else{
						if(neighbour_data[2] != NULL){
							if(!IS_TRANS(neighbour_data[2]->block_data[ATBLOCK(x,y,0)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, Z_AXIS, false, block_t, emit_offset, shortened_block, neighbours[2]->light.block_data[ATBLOCK(x,y,0)], surrounding_light_data);
							}
						}
					}
					
					if(!((z - 1) < 0)){
						if(!IS_TRANS(data->block_data[ATBLOCK(x,y,z-1)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, Z_AXIS, true, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y,z-1)], surrounding_light_data);
						}
					}else{
						if(neighbour_data[3] != NULL){
							if(!IS_TRANS(neighbour_data[3]->block_data[ATBLOCK(x,y,CHUNK_SIZE-1)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, Z_AXIS, true, block_t, emit_offset, shortened_block, neighbours[3]->light.block_data[ATBLOCK(x,y,CHUNK_SIZE-1)] , surrounding_light_data);
							}
						}
					}
				}
				
			}
		}
	}
}

static void standard_mesh_routine (struct sync_chunk_t* in, struct chunk_t* data, struct sync_chunk_t** neighbours,struct chunk_t** neighbour_data ,uint32_t emit_offset, uint32_t m_level){
	
	int chunk_x = in->_x;
	int chunk_z = in->_z;
	
	// Chunk Offset Coordinates (Are Used to Calculate the World Position of each Block)
	float c_x_off = (float)chunk_x - 0.5f;
	float c_z_off = (float)chunk_z - 0.5f;
	
	uint16_t surrounding_light_data [3][3][3];

	
	for(int x = 0; x < CHUNK_SIZE;++x){
		for(int y = 0; y < CHUNK_SIZE_Y;++y){
			for(int z = 0; z < CHUNK_SIZE;++z){
				
				uint8_t block_t = BLOCK_ID(data->block_data[ATBLOCK(x,y,z)]); // The Block Type at the current position
				
				bool emitter_cond = IS_SOLID(data->block_data[ATBLOCK(x,y,z)]); // Any Non-Air Block can spawn faces
				
				if (emitter_cond){
					float wx,wy,wz; // The World-Space Position
					wx = (x + c_x_off * CHUNK_SIZE) * 1.0f;
					wz = (z + c_z_off * CHUNK_SIZE) * 1.0f;
					wy = y * 1.0f;
					
					bool shortened_block = false; // Determine if the top side of the block is shifted down
					
					if(!((y + 1) == CHUNK_SIZE_Y)){
						if(!IS_SOLID(data->block_data[ATBLOCK(x,y+1,z)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, Y_AXIS, true, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y+1,z)], surrounding_light_data);
						}
					}else{
						load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
						emit_face(in, wx, wy, wz, Y_AXIS, true, block_t, emit_offset, shortened_block, MAX_LIGHT, surrounding_light_data);
					}
					
					if(!((y - 1) < 0)){
						if(!IS_SOLID(data->block_data[ATBLOCK(x,y-1,z)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, Y_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y-1,z)], surrounding_light_data);
						}
					}else{
						//emit_face(in, wx, wy, wz, Y_AXIS, false, block_t);
					}
					
					if(!((x + 1) == CHUNK_SIZE)){
						if(!IS_SOLID(data->block_data[ATBLOCK(x+1,y,z)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, X_AXIS, true, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x+1,y,z)], surrounding_light_data);
						}
					}else{
						if(neighbour_data[0] != NULL)
							if(!IS_SOLID(neighbour_data[0]->block_data[ATBLOCK(0,y,z)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, X_AXIS, true, block_t, emit_offset, shortened_block, neighbours[0]->light.block_data[ATBLOCK(0,y,z)], surrounding_light_data);
							}
					}
					
					if(!((x - 1) < 0)){
						if(!IS_SOLID(data->block_data[ATBLOCK(x-1,y,z)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, X_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x-1,y,z)], surrounding_light_data);
						}
					}else{
						if(neighbour_data[1] != NULL){
							if(!IS_SOLID(neighbour_data[1]->block_data[ATBLOCK(CHUNK_SIZE-1,y,z)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, X_AXIS, false, block_t, emit_offset, shortened_block, neighbours[1]->light.block_data[ATBLOCK(CHUNK_SIZE-1,y,z)], surrounding_light_data);
							}
						}
					}
					
					if(!((z + 1) == CHUNK_SIZE)){
						if(!IS_SOLID(data->block_data[ATBLOCK(x,y,z+1)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, Z_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y,z+1)], surrounding_light_data);
						}
					}else{
						if(neighbour_data[2] != NULL){
							if(!IS_SOLID(neighbour_data[2]->block_data[ATBLOCK(x,y,0)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, Z_AXIS, false, block_t, emit_offset, shortened_block, neighbours[2]->light.block_data[ATBLOCK(x,y,0)], surrounding_light_data);
							}
						}
					}
					
					if(!((z - 1) < 0)){
						if(!IS_SOLID(data->block_data[ATBLOCK(x,y,z-1)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, Z_AXIS, true, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y,z-1)], surrounding_light_data);
						}
					}else{
						if(neighbour_data[3] != NULL){
							if(!IS_SOLID(neighbour_data[3]->block_data[ATBLOCK(x,y,CHUNK_SIZE-1)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, Z_AXIS, true, block_t, emit_offset, shortened_block, neighbours[3]->light.block_data[ATBLOCK(x,y,CHUNK_SIZE-1)], surrounding_light_data);
							}
						}
					}
				}
				
			}
		}
	}
}

static void emit_x (struct sync_chunk_t* in, float wx, float wy, float wz, uint8_t axis, bool mirorred, uint8_t block_t, uint8_t offset, bool shortened, uint8_t lightlevel){
	
	float lightmul = (float)lightlevel / (float)MAX_LIGHT;
	
	float vertex_coordinates[24];
	float tex_coordinates[16];
	
	vertex_coordinates[0] = wx             ;vertex_coordinates[1] = wy              ;vertex_coordinates[2] = wz;
	vertex_coordinates[3] = wx + 1.0f;vertex_coordinates[4] = wy              ;vertex_coordinates[5] = wz + 1.0f;
	vertex_coordinates[6] = wx + 1.0f;vertex_coordinates[7] = wy + 1.0f;vertex_coordinates[8] = wz + 1.0f;
	vertex_coordinates[9] = wx             ;vertex_coordinates[10] = wy + 1.0f ;vertex_coordinates[11] = wz;
	
	vertex_coordinates[12] = wx             ;vertex_coordinates[13] = wy             ;vertex_coordinates[14] = wz + 1.0f;
	vertex_coordinates[15] = wx + 1.0f;vertex_coordinates[16] = wy             ;vertex_coordinates[17] = wz;
	vertex_coordinates[18] = wx + 1.0f;vertex_coordinates[19] = wy + 1.0f;vertex_coordinates[20] = wz;
	vertex_coordinates[21] = wx             ;vertex_coordinates[22] = wy + 1.0f;vertex_coordinates[23] = wz + 1.0f;
	
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

static void plant_mesh_routine (struct sync_chunk_t* in, struct chunk_t* data, struct sync_chunk_t** neighbours,struct chunk_t** neighbour_data ,uint32_t emit_offset, uint32_t m_level){
	
	int chunk_x = in->_x;
	int chunk_z = in->_z;
	
	float c_x_off = (float)chunk_x - 0.5f;
	float c_z_off = (float)chunk_z - 0.5f;
	
	uint16_t surrounding_light_data [3][3][3];
	
	for(int x = 0; x < CHUNK_SIZE;++x){
		for(int y = 0; y < CHUNK_SIZE_Y;++y){
			for(int z = 0; z < CHUNK_SIZE;++z){
				
				float wx,wy,wz; // The World-Space Position
				wx = (x + c_x_off * CHUNK_SIZE) * 1.0f;
				wz = (z + c_z_off * CHUNK_SIZE) * 1.0f;
				wy = y * 1.0f;
				
				uint16_t block_t = data->block_data[ATBLOCK(x,y,z)]; // The Block Type at the current position
				if(IS_X(block_t)){
					emit_x(in, wx, wy, wz, Y_AXIS, true, BLOCK_ID(block_t), emit_offset, false, in->light.block_data[ATBLOCK(x,y,z)]);
				}else if(IS_P(block_t)){
					bool shortened_block = false; // Determine if the top side of the block is shifted down
					
					if(!((y + 1) == CHUNK_SIZE_Y)){
						if(!IS_SOLID(data->block_data[ATBLOCK(x,y+1,z)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, Y_AXIS, true, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y+1,z)], surrounding_light_data);}
					}else{
						load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
						emit_face(in, wx, wy, wz, Y_AXIS, true, block_t, emit_offset, shortened_block, MAX_LIGHT, surrounding_light_data);
					}
					
					if(!((y - 1) < 0)){
						if(!IS_P(data->block_data[ATBLOCK(x,y-1,z)]) && !IS_SOLID(data->block_data[ATBLOCK(x,y-1,z)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, Y_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y-1,z)], surrounding_light_data);
						}
					}
					
					if(!((x + 1) == CHUNK_SIZE)){
						if(!IS_SOLID(data->block_data[ATBLOCK(x+1,y,z)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, X_AXIS, true, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x+1,y,z)], surrounding_light_data);
							
						}
					}else{
						if(neighbour_data[0] != NULL)
							if(!IS_SOLID(neighbour_data[0]->block_data[ATBLOCK(0,y,z)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, X_AXIS, true, block_t, emit_offset, shortened_block, neighbours[0]->light.block_data[ATBLOCK(0,y,z)], surrounding_light_data);
								
							}
					}
					
					if(!((x - 1) < 0)){
						if(!IS_P(data->block_data[ATBLOCK(x-1,y,z)]) && !IS_SOLID(data->block_data[ATBLOCK(x-1,y,z)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, X_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x-1,y,z)], surrounding_light_data);
						}
					}else{
						if(neighbour_data[1] != NULL){
							if(!IS_P(neighbour_data[1]->block_data[ATBLOCK(CHUNK_SIZE-1,y,z)]) && !IS_SOLID(neighbour_data[1]->block_data[ATBLOCK(CHUNK_SIZE-1,y,z)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, X_AXIS, false, block_t, emit_offset, shortened_block, neighbours[1]->light.block_data[ATBLOCK(CHUNK_SIZE-1,y,z)], surrounding_light_data);
							}
						}
					}
					
					if(!((z + 1) == CHUNK_SIZE)){
						if(!IS_SOLID(data->block_data[ATBLOCK(x,y,z+1)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, Z_AXIS, false, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y,z+1)], surrounding_light_data);
							
						}
					}else{
						if(neighbour_data[2] != NULL){
							if(!IS_SOLID(neighbour_data[2]->block_data[ATBLOCK(x,y,0)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, Z_AXIS, false, block_t, emit_offset, shortened_block, neighbours[2]->light.block_data[ATBLOCK(x,y,0)], surrounding_light_data);
								
							}
						}
					}
					
					if(!((z - 1) < 0)){
						if(!IS_P(data->block_data[ATBLOCK(x,y,z-1)]) && !IS_SOLID(data->block_data[ATBLOCK(x,y,z-1)])){
							load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
							emit_face(in, wx, wy, wz, Z_AXIS, true, block_t, emit_offset, shortened_block, in->light.block_data[ATBLOCK(x,y,z-1)], surrounding_light_data);
						}
					}else{
						if(neighbour_data[3] != NULL){
							if(!IS_P(neighbour_data[3]->block_data[ATBLOCK(x,y,CHUNK_SIZE-1)]) && !IS_SOLID(neighbour_data[3]->block_data[ATBLOCK(x,y,CHUNK_SIZE-1)])){
								load_surrounding_data(x,y,z, in, surrounding_light_data, neighbours);
								emit_face(in, wx, wy, wz, Z_AXIS, true, block_t, emit_offset, shortened_block, neighbours[3]->light.block_data[ATBLOCK(x,y,CHUNK_SIZE-1)], surrounding_light_data);
							}
						}
					}
				}
			}
		}
	}
}

static void build_chunk_mesh (struct sync_chunk_t* in, uint8_t m_level, void (*routine) (struct sync_chunk_t*, struct chunk_t*, struct sync_chunk_t**,struct chunk_t** ,uint32_t, uint32_t)){	
	
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
	
	struct sync_chunk_t* neighbours [8]; // this is dangerous, these chunks arent synced, might lead to weird artifacts
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
	
	
	data = &in->data_unique;
	for(int i = 0; i < 8; ++i){
		if(neighbours[i] != NULL){
			neighbour_data[i] = &neighbours[i]->data_unique;
		}else{
			neighbour_data[i] = NULL;
		}
	}
	
	
	emit_offset = m_level;
	emit_offset = emit_offset * 2 + !in->updatemesh;
	
	DFA_clear(&in->mesh_buffer[emit_offset]);
	
	(*routine)(in, data, neighbours, neighbour_data, emit_offset, m_level);
		
}

void do_updates_for_list (struct CLL* list){
	lock_list(list);

	
	for(struct CLL_element* p = list->first; p != NULL; p = p->nxt){
		chunk_data_sync(p->data);
		
		calculate_light(p->data, &skylight_func, true);
		calculate_light(p->data, &blocklight_func, false);
		
		build_chunk_mesh(p->data, 0, &standard_mesh_routine); // Build Block Mesh
		build_chunk_mesh(p->data, 1, &water_mesh_routine); // Build Water Mesh
		build_chunk_mesh(p->data, 2, &plant_mesh_routine); // Build Plant Mesh
		
		p->data->updatemesh = !p->data->updatemesh;
		p->data->vbo_update[0] = true;
		p->data->vbo_update[1] = true;
		p->data->vbo_update[2] = true;
		
		chunk_data_unsync(p->data);
	}
	CLL_freeList(list);
	unlock_list(list);
}

static void* builder_thread_func (void* arg){
	
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
