#include <raycast.h>

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

void block_ray_actor_chunkfree (int bmax, float ax, float ay, float az, float dx, float dy, float dz,  bool (*action_func)(int, int, int, float, float ,float)) {

	for(int i = 0; i < bmax; i++){
		
		ax += dx * 0.01f; // If we dont do this offset bias, get_next_block_in_direction wont be able to travel accross block boundaries for negative directions 
		ay += dy * 0.01f;
		az += dz * 0.01f;
		
		float sx,sy,sz;
		get_next_block_in_direction (ax, ay, az, dx, dy, dz, &sx, &sy, &sz, &ax, &ay, &az);
		
		//("%f %f %f %f %f %f\n", sx, sy, sz, ax, ay, az);
		
		int isx = (int)(sx);
		int isy = (int)(sy);
		int isz = (int)(sz);
		
		if ((*action_func)(isx, isy, isz, ax, ay, az)) break;
		
	}
} 
