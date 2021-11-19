#include <physics.h>
#include <math.h>
#include <worlddefs.h>
#include <chunklist.h>
#include <globallists.h>

void get_world_section_around (int x, int y, int z, uint8_t* sec, int* posx, int* posy, int* posz, int*ox, int*oy, int*oz){
	
	x = (x > 0) ? x : (x - 1);
	y = (y > 0) ? y : (y - 1);
	z = (z > 0) ? z : (z - 1);
	
	*ox = x-1;
	*oy = y-3;
	*oz = z-1;
	
	int index = 0;
	for(int isx = x - 1; isx <= x + 1; isx++){
		for(int isz = z - 1; isz <= z + 1; isz++){
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
			
			struct sync_chunk_t* current = CLL_getDataAt(&chunk_list[0], chunk_x, chunk_z);
			
			for(int isy = y-3; isy <= y + 3; isy++){
				if(current != NULL){
					sec[index] = IS_SOLID(current->data_unique.block_data[ATBLOCK(ccx, isy, ccz)]) ? STONE_B : 0;
					
					if(isy >= CHUNK_SIZE_Y ) sec[index] = AIR_B;
					
				} else {
					sec[index] = STONE_B;
				}
				posx[index] = isx - *ox;
				posy[index] = isy - *oy;
				posz[index] = isz - *oz;
				index++;
				
			}
			
		}
	}
}

#include <stdio.h>
float x_swept_collision ( struct pbox_t mov, struct pbox_t sta, float velx){
	
	if (velx == 0.0f) return 1.0f;
	
	float distance = (velx > 0.0f) ? (sta._x - (mov._x + mov._w)) : ((sta._x + sta._w) - mov._x);
	
	float z1distance = sta._z - mov._z;
	float z2distance = (sta._z + sta._l) - (mov._z + mov._l);
	bool zcontained = (mov._z > sta._z) && (mov._z + mov._l < sta._z + sta._l);
	
	float y1distance = sta._y - mov._y;
	float y2distance = (sta._y + sta._h) - (mov._y + mov._h);
	bool ycontained = (sta._y > mov._y) && (sta._y + sta._h < mov._y + mov._h);
	
	float coltime = distance / velx;
	
	if (coltime < 0.0f || coltime > 1.0f || ((fabs(z1distance) >= mov._l && fabs(z2distance) >= mov._l && !zcontained) || (fabs(y1distance) >= sta._h && fabs(y2distance) >= sta._h && !ycontained))) {return 1.0f;}
	else {return coltime - 0.001;}
	
}

float z_swept_collision ( struct pbox_t mov, struct pbox_t sta, float velz){
	
	if (velz == 0.0f) return 1.0f;
	
	float distance = (velz > 0.0f) ? (sta._z - (mov._z + mov._l)) : ((sta._z + sta._l) - mov._z);
	
	float x1distance = sta._x - mov._x;
	float x2distance = (sta._x + sta._w) - (mov._x + mov._w);
	bool  xcontained = (mov._x > sta._x) && (mov._x + mov._w < sta._x + sta._w);
	
	float y1distance = sta._y - mov._y;
	float y2distance = (sta._y + sta._h) - (mov._y + mov._h);
	bool ycontained = (sta._y > mov._y) && (sta._y + sta._h < mov._y + mov._h);
	
	float coltime = distance / velz;
	
	if (sta._z == 0.0f && sta._x == 1.0f && sta._y == 1.0f){
	}
	
	if (coltime < -0.0f || coltime > 1.0f || ((fabs(x1distance) >= mov._w && fabs(x2distance) >= mov._w && !xcontained) || (fabs(y1distance) >= sta._h && fabs(y2distance) >= sta._h && !ycontained))) 
	{return 1.0f;}
	else {return coltime - 0.001;}
	
}

float y_swept_collision ( struct pbox_t mov, struct pbox_t sta, float vely){
	
	if (vely == 0.0f) return 1.0f;
	
	float distance = (vely > 0.0f) ? (sta._y - (mov._y + mov._h)) : ((sta._y + sta._h) - mov._y);
	
	float x1distance = sta._x - mov._x;
	float x2distance = (sta._x + sta._w) - (mov._x + mov._w);
	bool  xcontained = (mov._x > sta._x) && (mov._x + mov._w < sta._x + sta._w);
	
	float z1distance = sta._z - mov._z;
	float z2distance = (sta._z + sta._l) - (mov._z + mov._l);
	bool zcontained = (mov._z > sta._z) && (mov._z + mov._l < sta._z + sta._l);
	
	float coltime = distance / vely;
	
	if (coltime < 0.0f || coltime > 1.0f || ((fabs(x1distance) >= mov._w && fabs(x2distance) >= mov._w && !xcontained) || (fabs(z1distance) >= mov._l && fabs(z2distance) >= mov._l && !zcontained))) {return 1.0f;}
	else{return coltime - 0.001;} // This bias is needed or some collisions WILL fail (this is why i hate programming physics simulations ...)
	
}
