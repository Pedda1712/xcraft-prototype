#include <player.h>
#include <game.h>
#include <worlddefs.h>
#include <globallists.h>
#include <math.h>

void get_next_block_in_direction (float posx, float posy, float posz, float dirx, float diry, float dirz, float* sx, float* sy, float* sz, float* actx, float* acty, float* actz){
	
	int ix = (posx >= 0) ? (int)posx : (int)(posx - 1.0f);
	int iy = (posy >= 0) ? (int)posy : (int)(posy - 1.0f);
	int iz = (posz >= 0) ? (int)posz : (int)(posz - 1.0f);

	float xfac, yfac, zfac;
	float dist;

	float x_nx = 0.0f;float x_ny = 0.0f;float x_nz = 0.0f;
	if(dirx != 0.0f){
		x_nx = ix + ((dirx >= 0) ? 1 : 0);
		dist = x_nx - posx;
		yfac = diry/dirx;
		zfac = dirz/dirx;
		x_ny = posy + yfac * dist;
		x_nz = posz + zfac * dist;
	}
	
	float z_nx = 0.0f;float z_ny = 0.0f;float z_nz = 0.0f;
	if(dirz != 0.0f){
		z_nz = iz + ((dirz >= 0) ? 1 : 0);
		dist = z_nz - posz;
		yfac = diry/dirz;
		xfac = dirx/dirz;
		z_ny = posy + yfac * dist;
		z_nx = posx + xfac * dist;
	}
	
	float y_nx = 0.0f;float y_ny = 0.0f;float y_nz = 0.0f;
	if(diry != 0.0f){
		y_ny = iy + ((diry >= 0) ? 1 : 0);
		dist = y_ny - posy;
		zfac = dirz/diry;
		xfac = dirx/diry;
		y_nz = posz + zfac * dist;
		y_nx = posx + xfac * dist;
	}

	float xdist = sqrt ((x_nx - posx)*(x_nx - posx)+(x_ny - posy)*(x_ny - posy)+(x_nz - posz)*(x_nz - posz));
	float ydist = sqrt ((y_nx - posx)*(y_nx - posx)+(y_ny - posy)*(y_ny - posy)+(y_nz - posz)*(y_nz - posz));
	float zdist = sqrt ((z_nx - posx)*(z_nx - posx)+(z_ny - posy)*(z_ny - posy)+(z_nz - posz)*(z_nz - posz));
	
	float nx, ny, nz;
	float ax, ay, az;
	if (xdist <= zdist && xdist <= ydist && dirx != 0.0f){
		ax = x_nx;ay = x_ny;az = x_nz;
		x_nx = (dirx >= 0) ? x_nx : x_nx - 1.0f;
		x_nz = (x_nz >= 0) ? x_nz : x_nz - 1.0f;
		x_ny = (x_ny >= 0) ? x_ny : x_ny - 1.0f;
		nx = x_nx;ny=x_ny;nz=x_nz;
	}else if (ydist <= xdist && ydist <= zdist && diry != 0.0f){
		ax = y_nx;ay = y_ny;az = y_nz;
		y_nx = (y_nx >= 0) ? y_nx : y_nx - 1.0f;
		y_nz = (y_nz >= 0) ? y_nz : y_nz - 1.0f;
		y_ny = (diry >= 0) ? y_ny : y_ny - 1.0f;
		nx = y_nx;ny = y_ny;nz = y_nz;
	}else if (dirz != 0.0f){
		ax = z_nx;ay = z_ny;az = z_nz;
		z_nx = (z_nx >= 0) ? z_nx : z_nx - 1.0f;
		z_ny = (z_ny >= 0) ? z_ny : z_ny - 1.0f;
		z_nz = (dirz >= 0) ? z_nz : z_nz - 1.0f;
		nx = z_nx;ny = z_ny;nz = z_nz;
	}else{nx=0.0f;ny=0.0f;nz=0.0f;ax=0.0f;ay=0.0f;az=0.0f;}//Never happens (hopefully)
	
	*sx=nx;  *sy=ny;  *sz=nz;
	*actx=ax;*acty=ay;*actz=az;
	
}

bool break_block_action (int chunk_x, int chunk_z, int ccx, int ccy, int ccz, float ax, float ay, float az){
	struct sync_chunk_t* in = CLL_getDataAt (&chunk_list[0], chunk_x, chunk_z);

	if( in != NULL ){
		
		if (in->data.block_data[ATBLOCK(ccx, ccy, ccz)] != AIR_B){
		
			in->data.block_data[ATBLOCK(ccx, ccy, ccz)] = AIR_B;
			in->water.block_data[ATBLOCK(ccx, ccy, ccz)] = AIR_B;
			
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
				in->water.block_data[ATBLOCK(ccx, ccy, ccz)] = gst._selected_block;
			}else{
				in->water.block_data[ATBLOCK(ccx, ccy, ccz)] = gst._selected_block;
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
