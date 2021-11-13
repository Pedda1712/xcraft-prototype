#include <player.h>
#include <game.h>
#include <worlddefs.h>
#include <globallists.h>
#include <raycast.h>

#include <math.h>
#include <stdlib.h>

bool break_block_action (int chunk_x, int chunk_z, int ccx, int ccy, int ccz, float ax, float ay, float az){
	struct sync_chunk_t* in = CLL_getDataAt (&chunk_list[0], chunk_x, chunk_z);

	if( in != NULL ){
		
		if (in->data_unique.block_data[ATBLOCK(ccx, ccy, ccz)] != AIR_B){
		
			if (in->data_unique.block_data[ATBLOCK(ccx, ccy, ccz)] == LIGHT_B){
				GLL_lock(&in->lightlist);
				for(struct GLL_element* e = in->lightlist.first; e != NULL; e = e->next){
					struct ipos3* d = e->data;
					if(d->_x == ccx && d->_y == ccy && d->_z == ccz){
						GLL_rem (&in->lightlist, d);
						free(d);
						break;
					}
				}
				GLL_unlock(&in->lightlist);
			}
			
			in->data.block_data[ATBLOCK(ccx, ccy, ccz)] = AIR_B;
			in->data_unique.block_data[ATBLOCK(ccx, ccy, ccz)] = AIR_B;
			
			lock_list(&chunk_list[3]);
			int chunk_distance = (int)(MAX_LIGHT / CHUNK_SIZE) + 1;
			for (int cx = chunk_x - chunk_distance; cx <= chunk_x + chunk_distance; cx++){
				for(int cz = chunk_z - chunk_distance; cz <= chunk_z + chunk_distance; cz++){
					struct sync_chunk_t* current = CLL_getDataAt (&chunk_list[0], cx, cz);
					
					if (current != NULL) {
						CLL_add (&chunk_list[3], current);
					}
					
				}
			}

			unlock_list(&chunk_list[3]);
			trigger_builder_update();
			
			return true; // abort the loop
		}
		
	}
	return false;
}

bool place_block_action (int chunk_x, int chunk_z, int ccx, int ccy, int ccz, float ax, float ay, float az){
	
	struct sync_chunk_t* in = CLL_getDataAt (&chunk_list[0], chunk_x, chunk_z);
	
	if (in->data.block_data[ATBLOCK(ccx, ccy, ccz)] == AIR_B){
		return false;
	}
	
	float iax = ax - (int)ax;
	float iay = ay - (int)ay;
	float iaz = az - (int)az;

	if (iax == 0.0f){ccx = (gst._dir_x > 0) ? ccx - 1 : ccx + 1;}
	if (iay == 0.0f){ccy = (gst._dir_y > 0) ? ccy - 1 : ccy + 1;}
	if (iaz == 0.0f){ccz = (gst._dir_z > 0) ? ccz - 1 : ccz + 1;}
	
	if(ccy < 0 || ccy >= CHUNK_SIZE_Y){
		return false;
	}
	
	bool in_correct_chunk = false;
	while(!in_correct_chunk){
		in_correct_chunk = true;
		if(ccx >= CHUNK_SIZE){ chunk_x++;in_correct_chunk = false;ccx -= CHUNK_SIZE;}
		if(ccz >= CHUNK_SIZE){ chunk_z++;in_correct_chunk = false;ccz -= CHUNK_SIZE;}
		if(ccx < 0) {chunk_x--;in_correct_chunk = false;ccx = (ccx + CHUNK_SIZE) % CHUNK_SIZE;}
		if(ccz < 0) {chunk_z--;in_correct_chunk = false;ccz = (ccz + CHUNK_SIZE) % CHUNK_SIZE;}
	}
	
	in = CLL_getDataAt (&chunk_list[0], chunk_x, chunk_z);
	
	if( in != NULL ){
		
		if (in->data.block_data[ATBLOCK(ccx, ccy, ccz)] == AIR_B){
			
			if(gst._selected_block != WATER_B){
				in->data.block_data[ATBLOCK(ccx, ccy, ccz)] = gst._selected_block;
				in->data_unique.block_data[ATBLOCK(ccx, ccy, ccz)] = gst._selected_block;
				
				if(gst._selected_block == LIGHT_B){
					struct ipos3* npos = malloc (sizeof(struct ipos3));
					npos->_x = ccx;npos->_y = ccy;npos->_z = ccz;
					GLL_add(&in->lightlist, npos);
				}
				
			}else{
				in->data_unique.block_data[ATBLOCK(ccx, ccy, ccz)] = gst._selected_block;
			}
			
			lock_list(&chunk_list[3]);
			int chunk_distance = (int)(MAX_LIGHT / CHUNK_SIZE) + 1;
			for (int cx = chunk_x - chunk_distance; cx <= chunk_x + chunk_distance; cx++){
				for(int cz = chunk_z - chunk_distance; cz <= chunk_z + chunk_distance; cz++){
					struct sync_chunk_t* current = CLL_getDataAt (&chunk_list[0], cx, cz);
					
					if (current != NULL) {
						CLL_add (&chunk_list[3], current);
					}
					
				}
			}

			unlock_list(&chunk_list[3]);
			trigger_builder_update();
			
			return true; // abort the loop
		}
		
	}
	return false;
}

void block_ray_actor (bool (*action_func)(int, int, int, int, int, float, float, float)) {
	float ax = gst._player_x;
	float ay = gst._player_y;
	float az = gst._player_z;
	
	for(int i = 0; i < RAY_ACTION_LENGTH; i++){
		
		ax += gst._dir_x * 0.01f; // If we dont do this offset bias, get_next_block_in_direction wont be able to travel accross block boundaries for negative directions 
		ay += gst._dir_y * 0.01f;
		az += gst._dir_z * 0.01f;
		
		float sx,sy,sz;
		get_next_block_in_direction (ax, ay, az, gst._dir_x, gst._dir_y, gst._dir_z, &sx, &sy, &sz, &ax, &ay, &az);

		int isx = (int)(sx);
		int isy = (int)(sy);
		int isz = (int)(sz);
		
		// Determining the chunk of the block
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
		
		if ((*action_func)(chunk_x, chunk_z, ccx, isy, ccz, ax, ay, az)) break;
		
	}
}
