#include <game.h>
#include <worlddefs.h>
#include <chunklist.h>
#include <chunkbuilder.h>
#include <dynamicarray.h>
#include <generator.h>
#include <blocktexturedef.h>
#include <windowglobals.h>
#include <bmpfont.h>
#include <ui.h>
#include <globallists.h>
#include <bmp24.h>
#include <math.h>
#include <player.h>
#include <genericlist.h>
#include <worldsave.h>

#include <xcraft_window_module.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// To be moved into a config file at some point
#define P_SPEED 10.0f
#define P_SHIFT_SPEED 20.0f

void (*input_state) (float);
void (*render_state)();
void (*debug_overlay_state)(float);
void (*overlay_state) ();

struct game_state_t gst;

void empty(){}
struct GLL current_button_list;

void menu_action_callback (bool pressed){
	if (pressed) {
		for( struct GLL_element* e = current_button_list.first; e != NULL; e = e->next){
		
		struct button_t* bt = (struct button_t*)e->data;
			if ( gst._mouse_x > bt->_x && gst._mouse_x < (bt->_x + bt->forg->width * CHARACTER_BASE_SIZE_X * bt->_scale) && gst._mouse_y > bt->_y && gst._mouse_y < ((bt->_y + bt->forg->height * CHARACTER_BASE_SIZE_Y * bt->_scale)) ){
				bt->clicked_func();
			}
		
		}
	}
}

void break_callback (bool pressed){
	if(pressed){
		block_ray_actor(&break_block_action);
	}
}
void place_callback (bool pressed){
	if(pressed){
		block_ray_actor(&place_block_action);
	}
}
void nextblock_callback (bool pressed){
	if(pressed){
		gst._selected_block++;
		if(gst._selected_block >= BLOCK_TYPE_COUNT){
			gst._selected_block = 0;
		}
	}
}
void prevblock_callback (bool pressed){
	if(pressed){
		gst._selected_block--;
		if(gst._selected_block < 0){
			gst._selected_block = BLOCK_TYPE_COUNT - 1;
		}
	}
}

void transistion_to_game () {
	xg_set_button1_callback (&break_callback);
	xg_set_button3_callback (&place_callback);
	xg_set_button4_callback (&prevblock_callback);
	xg_set_button5_callback (&nextblock_callback);
	
	xg_cursor_set(false, 0);
	
	input_state  = &world_input_state;
	render_state = &world_render_state;
	debug_overlay_state = &debug_fps_pos_state;
	overlay_state = &empty;
}

void world_input_state  (float frameTime){
	
	if(xg_keyboard_ascii((char)XK_Escape)) {
		xg_window_stop ();
	}

	int32_t offset_x, offset_y;
	xg_mouse_position(&offset_x, &offset_y);
	offset_x -= width/2;
	offset_y -= height/2;
	gst._angle_x += offset_y * -0.001f;
	gst._angle_y += offset_x * 0.001f;
	if(gst._angle_x >  3.14159f/2.01f) gst._angle_x = 3.14159f/2.01f;
	if(gst._angle_x < -3.14159f/2.01f) gst._angle_x = -3.14159f/2.01f;
	if(gst._angle_y > 6.28318f) gst._angle_y = 0.0f;
	if(gst._angle_y < 0.0f    ) gst._angle_y = 6.28318f;
	xg_set_mouse_position( width/2, height/2 );
	
	gst._dir_x = cos(gst._angle_x) * cos(gst._angle_y);
	gst._dir_y = sin(gst._angle_x);
	gst._dir_z = cos(gst._angle_x) * sin(gst._angle_y);
	
	float dirwalkcomplength = sqrt(gst._dir_x * gst._dir_x + gst._dir_z * gst._dir_z);
	float _strafe_x, _strafe_z; // Cross Product of _dir and (0, 1, 0) -> _strafe_y would always be 0
	_strafe_x = -gst._dir_z;
	_strafe_z = gst._dir_x;
	float _strafe_length = sqrt(_strafe_x * _strafe_x + _strafe_z * _strafe_z);
	_strafe_x /= _strafe_length;
	_strafe_z /= _strafe_length;
	
	float vely = 0.0f;
	float velx = 0.0f;
	float velz = 0.0f;
	if(xg_keyboard_modif(XK_Shift_L)){
		//p_speed = P_SHIFT_SPEED;
		vely = -10.f;
	}
	if(xg_keyboard_ascii(' ')){
		vely  = 10.f;
	}
	if(xg_keyboard_ascii('w')){
		velx += 10.0f * (gst._dir_x / dirwalkcomplength);
		velz += 10.0f * (gst._dir_z / dirwalkcomplength);
	}
	if(xg_keyboard_ascii('s')){
		velx -= 10.0f * (gst._dir_x / dirwalkcomplength);
		velz -= 10.0f * (gst._dir_z / dirwalkcomplength);
	}
	if(xg_keyboard_ascii('d')){
		velx += 10.0f * _strafe_x;
		velz += 10.0f * _strafe_z;
	}
	if(xg_keyboard_ascii('a')){
		velx -= 10.0f * _strafe_x;
		velz -= 10.0f * _strafe_z;
	}
	
	/*
		Collision Detection:
	 */
	
	uint8_t potential_colliders [63];
	int colx [63];int coly [63];int colz [63];
	int ox, oy, oz;
	get_world_section_around ((int)gst._player_x, (int)gst._player_y, (int)gst._player_z, potential_colliders, colx, coly, colz, &ox, &oy, &oz);
	
	gst._player_box._x = (gst._player_x - ox) - gst._player_box._w/2;
	gst._player_box._y = (gst._player_y - oy) - gst._player_box._h + 0.15f;
	gst._player_box._z = (gst._player_z - oz) - gst._player_box._l/2;
	
	float smallest = 1.0f;
	for (int i = 0; i < 63; i++){ // Test for any X-Collisions
		
		if(potential_colliders[i] != AIR_B){
			
			struct pbox_t block_collider = {
				colx[i], coly[i], colz[i],
				1.0f, 1.0f, 1.0f
			};
			
			float coltime = x_swept_collision (gst._player_box, block_collider, velx * frameTime);
			
			if (coltime < smallest){
				smallest = coltime;
			}
			
		}
		
	}
	
	gst._player_x += velx * frameTime * smallest; // Move along X-Axis

	gst._player_box._x = (gst._player_x - ox) - gst._player_box._w/2;
	gst._player_box._y = (gst._player_y - oy) - gst._player_box._h + 0.15f;
	gst._player_box._z = (gst._player_z - oz) - gst._player_box._l/2;
	
	smallest = 1.0f;
	for (int i = 0; i < 63; i++){ // Test for any Z-Collisions
		
		if(potential_colliders[i] != AIR_B){
			
			struct pbox_t block_collider = {
				colx[i], coly[i], colz[i],
				1.0f, 1.0f, 1.0f
			};
						
			float coltime = z_swept_collision (gst._player_box, block_collider, velz * frameTime);
			
			if (coltime < smallest){
				smallest = coltime;
			}
			
		}
		
	}
	gst._player_z += velz * frameTime * smallest; // Move along Z-Axis
	
	gst._player_box._x = (gst._player_x - ox) - gst._player_box._w/2;
	gst._player_box._y = (gst._player_y - oy) - gst._player_box._h + 0.15f;
	gst._player_box._z = (gst._player_z - oz) - gst._player_box._l/2;
	
	smallest = 1.0f;
	for (int i = 0; i < 63; i++){ // Test for any Y-Collisions
		
		if(potential_colliders[i] != AIR_B){
			
			struct pbox_t block_collider = {
				colx[i], coly[i], colz[i],
				1.0, 1.0, 1.0
			};
			
			float coltime = y_swept_collision (gst._player_box, block_collider, vely * frameTime);
			
			if (coltime < smallest){
				smallest = coltime;
			}
			
		}
		
	}
	gst._player_y += vely * frameTime * smallest; // Finally: Move along Y-Axis to complete the movement

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
	gluPerspective(gst._player_fov,(GLfloat)width/(GLfloat)height,0.1,500.0);
	
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

void menu_input_state (float fTime){
	int32_t offset_x, offset_y;
	xg_mouse_position(&offset_x, &offset_y);
	
	gst._mouse_x = ((float)offset_x / width) * 2;
	gst._mouse_y = ((float)offset_y / height) * 2;
}

void menu_overlay_state (){
	
	setupfont();
	setfont(gst._gfx_font);
	
	for( struct GLL_element* e = current_button_list.first; e != NULL; e = e->next){
		
		struct button_t* bt = (struct button_t*)e->data;
		if ( gst._mouse_x > bt->_x && gst._mouse_x < (bt->_x + bt->forg->width * CHARACTER_BASE_SIZE_X * bt->_scale) && gst._mouse_y > bt->_y && gst._mouse_y < ((bt->_y + bt->forg->height * CHARACTER_BASE_SIZE_Y * bt->_scale)) ){
			drawtextpg(bt->backh, bt->_x, bt->_y, bt->_scale);
			bt->highlighted_func();
		} else {
			drawtextpg(bt->backg, bt->_x, bt->_y, bt->_scale);
		}
		drawtextpg(bt->forg, bt->_x, bt->_y, bt->_scale);
		
	}
	
	revertfont();
}

void debug_fps_pos_state(float frameTime){
	setupfont();
	
	//char fps_txt [50];
	setfont(gst._gfx_font);
	drawstring("XCraft build-28/08/21", 0.0f, 0.0f, 0.44f);
	/*sprintf(fps_txt, "FPS: %f", 1.0f / frameTime);
	drawstring(fps_txt, 0.0f, CHARACTER_BASE_SIZE_Y * 0.44f, 0.44f);
 	sprintf(fps_txt, "X:%f, Y:%f, Z:%f", gst._player_x, gst._player_y, gst._player_z);
	drawstring(fps_txt, 0.0f, CHARACTER_BASE_SIZE_Y * 0.44f * 2, 0.44f);
	sprintf(fps_txt, "YAW:%f, PITCH:%f", gst._angle_y, gst._angle_x);
	drawstring(fps_txt, 0.0f, CHARACTER_BASE_SIZE_Y * 0.44f * 3, 0.44f);*/
	drawstring(blockname_map[gst._selected_block], 0.0f, 2.0f - CHARACTER_BASE_SIZE_Y * 0.44f, 0.44f);
	
	revertfont();
}

void init_game () {
	// To be moved into Save/Config files eventually ...
	gst._player_x = 6.32f;
	gst._player_y = 52.7f;
	gst._player_z = 26.8f;
	gst._p_chunk_x = 0;
	gst._p_chunk_z = 0;
	gst._angle_x = 0.31f;
	gst._angle_y = 5.44f;
	gst._dir_x = cos(gst._angle_x) * cos(gst._angle_y);
	gst._dir_y = sin(gst._angle_x);
	gst._dir_z = cos(gst._angle_x) * sin(gst._angle_y);
	gst._player_fov = 70.0f;
	gst._player_range = 10;
	gst._selected_block = 1;
	
	gst._player_box._w = 0.5;
	gst._player_box._h = 1.8;
	gst._player_box._l = 0.5;
	
	xg_set_button1_callback (&menu_action_callback);
	
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
	gst._atlas_texture = loadfont("atlas.bmp", GL_NEAREST);
	gst._text_font     = loadfont("font.bmp", GL_LINEAR);
	gst._gfx_font      = loadfont("font.bmp", GL_NEAREST);
	
	glClearColor(gst._skycolor[0],gst._skycolor[1],gst._skycolor[2],gst._skycolor[3]);
	
	// Creating Main Menu Button List 
	
	float ccaligned ( char* s, float sc ) {return (1.0f - (strlen(s) + 2) * sc * CHARACTER_BASE_SIZE_X * 0.5f);}
	
	current_button_list = GLL_init();
	GLL_add ( &current_button_list, ui_create_button_fit ( ccaligned("XCraft", UI_TITLE_SCALE), 0.5f - 3 * UI_TITLE_SCALE * CHARACTER_BASE_SIZE_Y * 0.5f, UI_TITLE_SCALE, BUTTON1_OFFSET, BUTTON1_OFFSET, "XCraft", &empty, &empty) );
	GLL_add ( &current_button_list, ui_create_button_fit ( ccaligned("Play", UI_SCALE)        , 1.0f, UI_SCALE      , BUTTON6_OFFSET, BUTTON7_OFFSET, "Play"  , &empty, &transistion_to_game
	) );
	GLL_add ( &current_button_list, ui_create_button_fit ( ccaligned("Quit", UI_SCALE)        , 1.0f + 3*UI_SCALE*CHARACTER_BASE_SIZE_Y, UI_SCALE      , BUTTON4_OFFSET, BUTTON5_OFFSET, "Quit"  , &empty, &xg_window_stop) );
	
}

void exit_game () {
	GLL_free_rec (&current_button_list);
}
