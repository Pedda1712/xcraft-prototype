#include <struced/clicktool.h>
#include <struced/appstate.h>
#include <struced/octree.h>
#include <blocktexturedef.h>

#include <raycast.h>
#include <windowglobals.h>
#include <worlddefs.h>
#include <math.h>

/*
	rotate_direction_by_ssoffset: rotates the dir-vector so it matches where user is pointing
	off1/2: offset from center 1 to -1, off1 is horizontal, off2 is vertical
	dir<x-z>: current direction
	res<x-z>: result return
*/

void rotate_direction_by_ssoffset (float off1, float off2, float dirx, float diry, float dirz, float* resx, float* resy, float* resz){

	
	// Todo: come up with better way to do this lol (calculate the vector in camera space, and translate it into the world?? might be good)	
	
	float len = sqrtf(dirx * dirx + diry * diry + dirz * dirz);
	dirx /= len;diry /= len; dirz /= len; // Z-Axis of camera space


	float aspect_ratio = (float)width / (float)height;
	float fov = (ast._fov / 180.0f) * 3.14159f;
	float distance = 1.0f / tan (fov/2.f);
	off1 *= aspect_ratio;
	
	float local_length = sqrtf(off1 * off1 + off2 * off2 + distance * distance);
	off1 /= local_length; off2 /= local_length; distance /= local_length;
	
	// off1, off2, distance are the x, y, z coordinates of the vector we want but its in camera space
	
	float x1,x2,x3; // x-axis of camera space
	float y1,y2,y3; // y-axis of camera space
	
	x1 = dirz; x2 = 0.0f; x3 = -dirx;
	float xlength = sqrtf(x1*x1 + x2 * x2 + x3 * x3);
	x1 /= xlength; x2 /= xlength; x3 /= xlength;
	
	y1 = dirx * diry; y2 = (-dirx * dirx - dirz * dirz); y3 = dirz * diry;
	float ylength = sqrtf(y1*y1 + y2 * y2 + y3 * y3);
	y1 /= ylength; y2 /= ylength; y3 /= ylength;
	
	*resx = x1*off1 + y1*off2 + dirx*distance; // Transform from camera into world space
	*resy = x2*off1 + y2*off2 + diry*distance;
	*resz = x3*off1 + y3*off2 + dirz*distance;
	

}

void place_clicktool (bool p, float x,float y){

	if(!p)return;
	
	/*
		To know into which direction the highlighted area needs to be offset, we need to recheck the collision (this is kind of ass, move selection_orientation to ast maybe?)
	 */

	float posx = sin(ast._camera_pos[0]) * ast._camera_rad * cos(ast._camera_pos[1]);
	float posy = sin(ast._camera_pos[1]) * ast._camera_rad;
	float posz = cos(ast._camera_pos[0]) * ast._camera_rad * cos(ast._camera_pos[1]);

	float dirx, diry, dirz;
	rotate_direction_by_ssoffset(-x, y, -posx, -posy, -posz, &dirx, &diry, &dirz);
	
	int selection_orientation[3] = {0,0,0};
	bool place_procedure (int ix, int iy, int iz, float ax, float ay, float az){

		float iax = ax - (int)ax;
		float iay = ay - (int)ay;
		float iaz = az - (int)az;
		
		struct block_t block = {ix, iy, iz, btd_map[ast._selected_block].complete_id};
		if(OCT_check(block_tree, block)){
			selection_orientation[0] = (iax == 0.0f) ? ((dirx < 0) ? 1 : -1) : 0;
			selection_orientation[1] = (iay == 0.0f) ? ((diry < 0) ? 1 : -1) : 0;
			selection_orientation[2] = (iaz == 0.0f) ? ((dirz < 0) ? 1 : -1) : 0;
			return true;
		}

		return false;
	}
	
	block_ray_actor_chunkfree (512, posx, posy, posz, dirx, diry, dirz, &place_procedure);
	
	/*
		Go over each highlighted block (, offset it according to the side the mouse cursor is on,) and add that block to the octree
	 */
	
	for(struct GLL_element* e = ast._highlight_list.first; e != NULL; e = e->next){
		struct block_t block = *((struct block_t*)e->data);
		
		block._x += selection_orientation[0];
		block._y += selection_orientation[1];
		block._z += selection_orientation[2];
		
		if(!OCT_check(block_tree, block))
			add_block_octree (block);

	}

}

void break_clicktool (bool p, float x,float y){

	if (!p)return;

	/*
		Simply go over each highlighted block and remove it, if there is an entry in the octree for it
	 */

	for(struct GLL_element* e = ast._highlight_list.first; e != NULL; e = e->next){

		struct block_t block = *((struct block_t*)e->data);
		if(OCT_check(block_tree, block)){

			rem_block_octree (block);
		}

	}
	

}
