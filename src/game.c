#include <game.h>
#include <worlddefs.h>
#include <chunklist.h>
#include <chunkbuilder.h>
#include <dynamicarray.h>
#include <generator.h>
#include <blocktexturedef.h>
#include <windowglobals.h>
#include <bmpfont.h>
#include <globallists.h>
#include <bmp24.h>
#include <math.h>
#include <xgame.h>

#include <stdlib.h>
#include <stdio.h>

// To be moved into a config file at some point
#define P_SPEED 10.0f
#define P_SHIFT_SPEED 20.0f

void (*input_state) (float);
void (*render_state)();
void (*debug_overlay_state)(float);

struct game_state_t {
	float _player_fov;
	float _player_x;
	float _player_y;
	float _player_z;
	float _dir_x;
	float _dir_y;
	float _dir_z;
	float _angle_x;
	float _angle_y;
	int32_t _p_chunk_x;
	int32_t _p_chunk_z;
	int32_t _player_range;
	GLuint _atlas_texture;
	float _skycolor[4];
} gst;

/*
 get_next_block_in_direction:
 Overly complicated function (because integer rounding is a bitch) for determining the next block in a given direction
 sx,sy,sz -> these values are the actual block position (when cast to an int)
 ax,ay,az -> these values are the intersection point (used for chaining get_next_block_in_direction calls)
 */

void get_next_block_in_direction (float posx, float posy, float posz, float dirx, float diry, float dirz, float* sx, float* sy, float* sz, float* actx, float* acty, float* actz){
	
	int ix = (posx >= 0) ? (int)posx : (int)(posx - 1.0f);
	int iy = (posy >= 0) ? (int)posy : (int)(posy - 1.0f);
	int iz = (posz >= 0) ? (int)posz : (int)(posz - 1.0f);

	float xfac, yfac, zfac;
	float dist;
	int sign;

	float x_nx = 0.0f;float x_ny = 0.0f;float x_nz = 0.0f;
	if(dirx != 0.0f){
		sign = (dirx >= 0) ? 1 : 0;
		x_nx = ix + sign;
		dist = x_nx - posx;
		yfac = diry/dirx;
		zfac = dirz/dirx;
		x_ny = posy + yfac * dist;
		x_nz = posz + zfac * dist;
	}
	
	float z_nx = 0.0f;float z_ny = 0.0f;float z_nz = 0.0f;
	if(dirz != 0.0f){
		sign = (dirz >= 0) ? 1 : 0;
		z_nz = iz + sign;
		dist = z_nz - posz;
		yfac = diry/dirz;
		xfac = dirx/dirz;
		z_ny = posy + yfac * dist;
		z_nx = posx + xfac * dist;
	}
	
	float y_nx = 0.0f;float y_ny = 0.0f;float y_nz = 0.0f;
	if(diry != 0.0f){
		sign = (diry >= 0) ? 1 : 0;
		y_ny = iy + sign;
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
		
		if (in->data.block_data[ATBLOCK(ccx, ccy, ccz)] != 0){
		
			in->data.block_data[ATBLOCK(ccx, ccy, ccz)] = 0;
			in->water.block_data[ATBLOCK(ccx, ccy, ccz)] = 0;
			
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
	
	if (in->data.block_data[ATBLOCK(ccx, ccy, ccz)] == 0){
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
		
		if (in->data.block_data[ATBLOCK(ccx, ccy, ccz)] == 0){
			
			in->data.block_data[ATBLOCK(ccx, ccy, ccz)] = STONE_B;
			in->water.block_data[ATBLOCK(ccx, ccy, ccz)] = STONE_B;
			
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

void block_ray_action (bool (*action_func)(int, int, int, int, int, float, float, float)) {
	float ax = gst._player_x;
	float ay = gst._player_y;
	float az = gst._player_z;
	
	for(int i = 0; i < 10; i++){
		
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
		
		int ccx = (int)((float)isx/BLOCK_SIZE - c_x_off * CHUNK_SIZE)%16; // world space coordinate to chunk space coordinate
		int ccz = (int)((float)isz/BLOCK_SIZE - c_z_off * CHUNK_SIZE)%16;
		
		if(isx < 0 && ccx == 0) chunk_x++; // negative integer rounding ... -.-
		if(isz < 0 && ccz == 0) chunk_z++;
		
		if ((*action_func)(chunk_x, chunk_z, ccx, isy, ccz, ax, ay, az)) break;
		
	}
}

void break_callback (bool pressed){
	if(pressed){
		block_ray_action(&break_block_action);
	}
}
void place_callback (bool pressed){
	if(pressed){
		block_ray_action(&place_block_action);
	}
}

void world_input_state  (float frameTime){
	
	if(xg_keyboard_ascii((char)XK_Escape)) {
		xg_window_stop ();
	}

	int32_t offset_x, offset_y;
	xg_mouse_position(&offset_x, &offset_y);
	offset_x -= width/2;
	offset_y -= height/2;
	gst._angle_x += offset_x * 0.001f;
	gst._angle_y += offset_y * -0.001f;
	if(gst._angle_y > 3.14159f/2.01f)  gst._angle_y = 3.14159f/2.01f;
	if(gst._angle_y < -3.14159f/2.01f) gst._angle_y = -3.14159f/2.01f;
	xg_set_mouse_position( width/2, height/2 );
	
	gst._dir_x = cos(gst._angle_y) * cos(gst._angle_x);
	gst._dir_y = sin(gst._angle_y);
	gst._dir_z = cos(gst._angle_y) * sin(gst._angle_x);
	
	float _strafe_x, _strafe_z; // Cross Product of _dir and (0, 1, 0) -> _strafe_y would always be 0
	_strafe_x = -gst._dir_z;
	_strafe_z = gst._dir_x;
	float _strafe_length = sqrt(_strafe_x * _strafe_x + _strafe_z * _strafe_z);
	_strafe_x /= _strafe_length;
	_strafe_z /= _strafe_length;

	float p_speed;
	if(xg_keyboard_modif(XK_Shift_L)){
		p_speed = P_SHIFT_SPEED;
	}else{
		p_speed = P_SPEED;
	}
	if(xg_keyboard_ascii('w')){
		gst._player_x += gst._dir_x * frameTime * p_speed;
		gst._player_y += gst._dir_y * frameTime * p_speed;
		gst._player_z += gst._dir_z * frameTime * p_speed;
	}  
	if(xg_keyboard_ascii('s')){
		gst._player_x -= gst._dir_x * frameTime * p_speed;
		gst._player_y -= gst._dir_y * frameTime * p_speed;
		gst._player_z -= gst._dir_z * frameTime * p_speed;
	}
	if(xg_keyboard_ascii('d')){
		gst._player_x += _strafe_x * frameTime * p_speed;
		gst._player_z += _strafe_z * frameTime * p_speed;
	}
	if(xg_keyboard_ascii('a')){
		gst._player_x -= _strafe_x * frameTime * p_speed;
		gst._player_z -= _strafe_z * frameTime * p_speed;
	}
	
}

void world_render_state (){
	//Check if Player crossed a chunk border
	
	float _add_x = (gst._player_x > 0) ? CHUNK_SIZE / 2.0f : -CHUNK_SIZE / 2.0f;
	float _add_z = (gst._player_z > 0) ? CHUNK_SIZE / 2.0f : -CHUNK_SIZE / 2.0f;
	
	int32_t n_p_chunk_x = (gst._player_x + _add_x) / CHUNK_SIZE;
	int32_t n_p_chunk_z = (gst._player_z + _add_z) / CHUNK_SIZE;
		
	if(n_p_chunk_x != gst._p_chunk_x || n_p_chunk_z != gst._p_chunk_z){
		gst._p_chunk_x = n_p_chunk_x;
		gst._p_chunk_z = n_p_chunk_z;
		struct chunkspace_position pos;
		pos._x = gst._p_chunk_x;
		pos._z = gst._p_chunk_z;
		trigger_generator_update(&pos);
	}
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(gst._player_x,gst._player_y,gst._player_z,gst._player_x + gst._dir_x,gst._player_y + gst._dir_y, gst._player_z + gst._dir_z,0,1,0);
	
	glMatrixMode(GL_PROJECTION);  
	glLoadIdentity();
	gluPerspective(gst._player_fov,(GLdouble)width/(GLdouble)height,0.1,500.0);
	
	glBindTexture(GL_TEXTURE_2D, gst._atlas_texture);
	
	glEnable(GL_FOG);
	glEnable  (GL_DEPTH_TEST);
	glEnable  (GL_CULL_FACE);
	
	struct CLL_element* p;
	for(p = chunk_list[0].first; p!= NULL; p = p->nxt){
		struct sync_chunk_t* ch = p->data;
		
		// Render Chunk
		glVertexPointer(3, GL_FLOAT, 0, ch->vertex_array[ch->rendermesh].data);
		glTexCoordPointer (2, GL_FLOAT, 0, ch->texcrd_array[ch->rendermesh].data);
		glColorPointer (3, GL_FLOAT, 0, ch->lightl_array[ch->rendermesh].data);
		glDrawArrays(GL_QUADS, 0, ch->vertex_array[ch->rendermesh].size/3 );
	}
	for(p = chunk_list[0].first; p!= NULL; p = p->nxt){
		struct sync_chunk_t* ch = p->data;
		// Render Water
			
		glEnable(GL_BLEND);
		
		glVertexPointer(3, GL_FLOAT, 0, ch->vertex_array[2 + ch->rendermesh].data);
		glTexCoordPointer (2, GL_FLOAT, 0, ch->texcrd_array[2 + ch->rendermesh].data);
		glColorPointer (3, GL_FLOAT, 0, ch->lightl_array[2 + ch->rendermesh].data);
		glDrawArrays(GL_QUADS, 0, ch->vertex_array[2 + ch->rendermesh].size/3 );
		
		glDisable(GL_BLEND);
	}
}

void debug_fps_pos_state(float frameTime){
	
	setupfont();
	
	char fps_txt [50];
	sprintf(fps_txt, "FPS: %f", 1.0f / frameTime);
	drawstring(fps_txt, 0.0f, 0.0f, 0.44f);
	sprintf(fps_txt, "CX: %i, CZ: %i", gst._p_chunk_x, gst._p_chunk_z);
	drawstring(fps_txt, 0.0f, CHARACTER_BASE_SIZE_Y * 0.44f, 0.44f);
	drawstring("BUILD-04/07/21", 0.0f, CHARACTER_BASE_SIZE_Y * 0.44f * 2, 0.44f);
	
	revertfont();
	
}

void init_game () {
	// To be moved into Save/Config files eventually ...
	gst._player_x = 0.0f;
	gst._player_y = 64.0f;
	gst._player_z = 0.0f;
	gst._p_chunk_x = 0;
	gst._p_chunk_z = 0;
	gst._dir_x = 0.0f;
	gst._dir_y = 0.0f;
	gst._dir_z = 0.0f;
	gst._angle_x = 3.14159f/2;
	gst._angle_y = 0.0f;
	gst._player_fov = 70.0f;
	gst._player_range = 10;
	
	xg_set_button1_callback (&break_callback);
	xg_set_button3_callback (&place_callback);
	
	struct chunkspace_position pos = {0,0};
	// Generate the initial Chunks on the main thread
	run_chunk_generation(&pos);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	gst._skycolor[0] = 0.0f;
	gst._skycolor[1] = 0.5f;
	gst._skycolor[2] = 1.0f;
	gst._skycolor[3] = 1.0f;
	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	
	glEnable(GL_FOG); // Configure Fog
	glFogfv(GL_FOG_COLOR, gst._skycolor);
	glFogi (GL_FOG_MODE , GL_LINEAR);
	glFogf (GL_FOG_END  , (WORLD_RANGE/1.2f) * CHUNK_SIZE);
	glFogf (GL_FOG_START, (WORLD_RANGE/1.7f) * CHUNK_SIZE);
	
	glBlendFunc(GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_COLOR); // Configure Blending
	glBlendColor(0.33f,0.33f,0.0f,1.0f);
	
	//Loading a Texture
	uint32_t iw, ih;
	uint8_t* image = loadBMP("atlas.bmp",&iw,&ih);
	glGenTextures(1, &gst._atlas_texture);
	glBindTexture(GL_TEXTURE_2D, gst._atlas_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, iw, ih, 0, GL_BGR, GL_UNSIGNED_BYTE, image);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	free (image);
	
	loadfont("font.bmp", GL_LINEAR);
	
	glClearColor(gst._skycolor[0],gst._skycolor[1],gst._skycolor[2],gst._skycolor[3]);
}
