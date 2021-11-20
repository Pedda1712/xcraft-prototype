#include <GL/glew.h>

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
#include <player.h>
#include <genericlist.h>
#include <worldsave.h>
#include <pnoise.h>
#include <shader.h>

#include <xcraft_window_module.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// To be moved into a config file at some point
#define P_SPEED 20.0f
#define P_SHIFT_SPEED 40.0f

struct GLL state_stack;
struct GLL state_update_stack;

struct game_state_t gst;

void empty(){}

struct GLL world_selection_button_list;
struct GLL main_menu_button_list;
struct GLL current_button_list;

static void menu_action_callback (bool pressed){
	if (pressed) {
		for( struct GLL_element* e = current_button_list.first; e != NULL; e = e->next){

		struct button_t* bt = (struct button_t*)e->data;
			if ( gst._mouse_x > bt->_x && gst._mouse_x < (bt->_x + bt->forg->width * CHARACTER_BASE_SIZE_X * bt->_scale) && gst._mouse_y > bt->_y && gst._mouse_y < ((bt->_y + bt->forg->height * CHARACTER_BASE_SIZE_Y * bt->_scale)) ){
				bt->clicked_func(bt);
			}

		}
	}
}

static void break_callback (bool pressed){
	if(pressed){
		block_ray_actor(&break_block_action);
	}
}

static void place_callback (bool pressed){
	if(pressed){
		block_ray_actor(&place_block_action);
	}
}

static void nextblock_callback (bool pressed){
	if(pressed){
		gst._selected_block++;
		if(gst._selected_block >= BLOCK_TYPE_COUNT){
			gst._selected_block = 0;
		}
	}
}
static void prevblock_callback (bool pressed){
	if(pressed){
		gst._selected_block--;
		if(gst._selected_block < 0){
			gst._selected_block = BLOCK_TYPE_COUNT - 1;
		}
	}
}

static void tt_selection () {
	GLL_free(&state_stack);
	GLL_add(&state_stack, &menu_input_state);
	GLL_add(&state_stack, &world_render_state);
	GLL_add(&state_stack, &debug_fps_pos_state);
	GLL_add(&state_stack, &menu_overlay_state);

	xg_cursor_set(true, 132);

	xg_set_button1_callback (&menu_action_callback);
	current_button_list = world_selection_button_list;
}

static void transition_to_selection (){

	GLL_add(&state_update_stack, &tt_selection);
}

static void tt_main_menu () {
	GLL_free(&state_stack);
	GLL_add(&state_stack, &menu_input_state);
	GLL_add(&state_stack, &world_render_state);
	GLL_add(&state_stack, &debug_fps_pos_state);
	GLL_add(&state_stack, &menu_overlay_state);

	xg_cursor_set(true, 132);

	xg_set_button1_callback (&menu_action_callback);

	current_button_list = main_menu_button_list;
}

static void transition_to_main_menu (){

	GLL_add(&state_update_stack, &tt_main_menu);
}

static void tt_game () {
	xg_set_button1_callback (&break_callback);
	xg_set_button3_callback (&place_callback);
	xg_set_button4_callback (&prevblock_callback);
	xg_set_button5_callback (&nextblock_callback);

	xg_cursor_set(false, 0);

	GLL_free(&state_stack);
	GLL_add(&state_stack, &world_input_state);
	GLL_add(&state_stack, &world_render_state);
	GLL_add(&state_stack, &world_overlay_state);
	GLL_add(&state_stack, &debug_fps_pos_state);
}

static void continue_to_game (){

	GLL_add(&state_update_stack, &tt_game);
}

static void tt_regen () {
	gst._player_x = 0.0f;
	gst._player_y = 126.0f;
	gst._player_z = 0.0f;
	gst._p_chunk_x = 0;
	gst._p_chunk_z = 0;
	gst._angle_x = -0.5f;
	gst._angle_y = 0.0f;

	srand(time(NULL));
	seeded_noise_shuffle (rand());
	delete_current_world();
	set_world_name ("default");

	for(struct CLL_element* e = chunk_list[0].first; e != NULL; e = e->nxt){
		e->data->initialized = false;
	}

	// Generate the initial Chunks on the main thread
	//run_chunk_generation_atprev();
	struct chunkspace_position pos = {0,0};
	// Generate the initial Chunks on the main thread
	run_chunk_generation(&pos);
}

static void transition_regen (){

	GLL_add(&state_update_stack, &tt_regen);
}

void world_input_state  (float frameTime){

	if(xg_keyboard_ascii((char)XK_Escape)) {

		transition_to_selection();
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
		vely = -P_SPEED;
	}
	if(xg_keyboard_ascii(' ')){
		vely  = P_SPEED;
	}
	if(xg_keyboard_ascii('w')){
		velx += P_SPEED * (gst._dir_x / dirwalkcomplength);
		velz += P_SPEED * (gst._dir_z / dirwalkcomplength);
	}
	if(xg_keyboard_ascii('s')){
		velx -= P_SPEED * (gst._dir_x / dirwalkcomplength);
		velz -= P_SPEED * (gst._dir_z / dirwalkcomplength);
	}
	if(xg_keyboard_ascii('d')){
		velx += P_SPEED * _strafe_x;
		velz += P_SPEED * _strafe_z;
	}
	if(xg_keyboard_ascii('a')){
		velx -= P_SPEED * _strafe_x;
		velz -= P_SPEED * _strafe_z;
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

void world_render_state (float fTime){
	//Check if Player crossed a chunk border

	int update_chunk_vbo (struct sync_chunk_t* c, uint8_t level){

		glBindBuffer(GL_ARRAY_BUFFER, c->mesh_vbo[level]);
		glBufferData(GL_ARRAY_BUFFER, c->mesh_buffer[level * 2 + c->updatemesh].size * sizeof(float), c->mesh_buffer[level * 2 + c->updatemesh].data, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return c->mesh_buffer[level * 2 + c->updatemesh].size/6;
	}

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

	glEnable  (GL_DEPTH_TEST);
	glEnable  (GL_CULL_FACE);

	glUseProgram(gst._shader_prg);
	struct CLL_element* p;
	for(p = chunk_list[0].first; p!= NULL; p = p->nxt){
		struct sync_chunk_t* ch = p->data;

		if(ch->vbo_update[0]){
			chunk_data_sync(ch);
			ch->verts[0] = update_chunk_vbo(ch, false);
			ch->vbo_update[0] = false;
			chunk_data_unsync(ch);
		}


		// Render Chunk
		glBindBuffer(GL_ARRAY_BUFFER, ch->mesh_vbo[0]);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void*)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void*)(3* sizeof(float)));
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void*)(5* sizeof(float)));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glDrawArrays(GL_QUADS, 0, ch->verts[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

	}

	glEnable(GL_BLEND);
	for(p = chunk_list[0].first; p!= NULL; p = p->nxt){
		struct sync_chunk_t* ch = p->data;
		// Render water

		if(ch->vbo_update[1]){
			chunk_data_sync(ch);
			ch->verts[1] = update_chunk_vbo(ch, 1);
			ch->vbo_update[1] = false;
			chunk_data_unsync(ch);
		}

		// Render Chunk
		glBindBuffer(GL_ARRAY_BUFFER, ch->mesh_vbo[1]);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void*)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void*)(3* sizeof(float)));
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void*)(5* sizeof(float)));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glDrawArrays(GL_QUADS, 0, ch->verts[1]);

	}
	glDisable(GL_BLEND);

	glDisable(GL_CULL_FACE);
	for(p = chunk_list[0].first; p!= NULL; p = p->nxt){
		struct sync_chunk_t* ch = p->data;
		// Render plants

		if(ch->vbo_update[2]){
			chunk_data_sync(ch);
			ch->verts[2] = update_chunk_vbo(ch, 2);
			ch->vbo_update[2] = false;
			chunk_data_unsync(ch);
		}

		// Render Chunk
		glBindBuffer(GL_ARRAY_BUFFER, ch->mesh_vbo[2]);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void*)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void*)(3* sizeof(float)));
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_TRUE, 6 * sizeof(float), (void*)(5* sizeof(float)));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glDrawArrays(GL_QUADS, 0, ch->verts[2]);

	}
	glEnable(GL_CULL_FACE);

	glUseProgram(0);
}

void menu_input_state (float fTime){
	int32_t offset_x, offset_y;
	xg_mouse_position(&offset_x, &offset_y);

	gst._mouse_x = ((float)offset_x / width) * 2;
	gst._mouse_y = ((float)offset_y / height) * 2;
}

void menu_overlay_state (float fTime){

	setupfont();
	setfont(gst._gfx_font);

	for( struct GLL_element* e = current_button_list.first; e != NULL; e = e->next){

		struct button_t* bt = (struct button_t*)e->data;
		if ( gst._mouse_x > bt->_x && gst._mouse_x < (bt->_x + bt->forg->width * CHARACTER_BASE_SIZE_X * bt->_scale) && gst._mouse_y > bt->_y && gst._mouse_y < ((bt->_y + bt->forg->height * CHARACTER_BASE_SIZE_Y * bt->_scale)) ){
			drawtextpg(bt->backh, bt->_x, bt->_y, bt->_scale);
			bt->highlighted_func(bt);
		} else {
			drawtextpg(bt->backg, bt->_x, bt->_y, bt->_scale);
		}
		drawtextpg(bt->forg, bt->_x, bt->_y + (bt->_scale * CHARACTER_BASE_SIZE_Y * 0.5f), bt->_scale);

	}

	revertfont();
}

void world_overlay_state (float fTime){

	setupfont();

	drawstring(blockname_map[gst._selected_block], 0.0f, 2.0f - CHARACTER_BASE_SIZE_Y * 0.44f, 0.44f);

	drawstring("+", 1.0f - 0.11f * CHARACTER_BASE_SIZE_X, 1.0f - 0.11f * CHARACTER_BASE_SIZE_Y, 0.22f);

	revertfont();

}

void debug_fps_pos_state(float frameTime){
	setupfont();

	char fps_txt [50];
	setfont(gst._gfx_font);
	drawstring("XCraft build-18/11/21", 0.0f, 0.0f, 0.44f);
	sprintf(fps_txt, "FPS: %f", 1.0f / frameTime);
	drawstring(fps_txt, 0.0f, CHARACTER_BASE_SIZE_Y * 0.44f, 0.44f);
 	sprintf(fps_txt, "X:%f, Y:%f, Z:%f", gst._player_x, gst._player_y, gst._player_z);
	drawstring(fps_txt, 0.0f, CHARACTER_BASE_SIZE_Y * 0.44f * 2, 0.44f);
	sprintf(fps_txt, "YAW:%f, PITCH:%f", gst._angle_y, gst._angle_x);
	drawstring(fps_txt, 0.0f, CHARACTER_BASE_SIZE_Y * 0.44f * 3, 0.44f);

	revertfont();
}

void init_game () {
	// Can be overriden by saved values ..
	gst._player_x = 0.0f;
	gst._player_y = 126.0f;
	gst._player_z = 0.0f;
	gst._p_chunk_x = 0;
	gst._p_chunk_z = 0;
	gst._angle_x = -0.5f;
	gst._angle_y = 0.0f;
	gst._dir_x = cos(gst._angle_x) * cos(gst._angle_y);
	gst._dir_y = sin(gst._angle_x);
	gst._dir_z = cos(gst._angle_x) * sin(gst._angle_y);
	gst._player_fov = 70.0f;
	gst._player_range = 10;
	gst._selected_block = 1;

	gst._shader_prg = load_shader("shader/clientside.v", "shader/clientside.f");
	glUseProgram(gst._shader_prg);
	glBindAttribLocation(gst._shader_prg, 0, "apos");
	glBindAttribLocation(gst._shader_prg, 1, "atex");
	glBindAttribLocation(gst._shader_prg, 2, "acol");
	glLinkProgram(gst._shader_prg);

	gst._player_box._w = 0.5;
	gst._player_box._h = 1.8;
	gst._player_box._l = 0.5;

	set_world_name ("default");
	init_structure_cache();

	struct chunkspace_position pos = {0,0};
	// Generate the initial Chunks on the main thread
	run_chunk_generation(&pos);

	gst._skycolor[0] = 0.0f;
	gst._skycolor[1] = 0.5f;
	gst._skycolor[2] = 1.0f;
	gst._skycolor[3] = 1.0f;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glBlendFunc(GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_COLOR); // Configure Blending
	glBlendColor(0.33f,0.33f,0.0f,1.0f);

	//Loading a Texture
	gst._atlas_texture = loadfont("atlas.bmp", GL_NEAREST);
	gst._text_font     = loadfont("font.bmp", GL_LINEAR);
	gst._gfx_font      = loadfont("font.bmp", GL_NEAREST);

	glClearColor(gst._skycolor[0],gst._skycolor[1],gst._skycolor[2],gst._skycolor[3]);

	// Creating UI Button Lists

	inline float ccaligned ( char* s, float sc ) {return (1.0f - (strlen(s) + 2) * sc * CHARACTER_BASE_SIZE_X * 0.5f);}

	/*
		Button List for the Main Menu:
	 */

	main_menu_button_list = GLL_init();
	GLL_add ( &main_menu_button_list,
			  ui_create_button_fit (ccaligned("XCraft", UI_TITLE_SCALE),
									0.5f - 3 * UI_TITLE_SCALE * CHARACTER_BASE_SIZE_Y * 0.5f,
									UI_TITLE_SCALE,
									BUTTON1_OFFSET,
									BUTTON1_OFFSET,
									"XCraft",
									&empty,
									&empty) );
	GLL_add ( &main_menu_button_list,
			  ui_create_button_fit ( ccaligned("Play", UI_SCALE),
									 1.0f,
									UI_SCALE,
									BUTTON6_OFFSET,
									BUTTON7_OFFSET,
									"Play",
									&empty,
									&transition_to_selection));
	GLL_add ( &main_menu_button_list,
			  ui_create_button_fit ( ccaligned("Quit", UI_SCALE),
									 1.0f + 2*UI_SCALE*CHARACTER_BASE_SIZE_Y,
									UI_SCALE,
									BUTTON4_OFFSET,
									BUTTON5_OFFSET,
									"Quit",
									&empty,
									&xg_window_stop) );

	/*
		Button List for the "Play"-Submenu
	 */

	world_selection_button_list = GLL_init();
	GLL_add ( &world_selection_button_list,
			  ui_create_button_fit ( ccaligned("XCraft", UI_TITLE_SCALE),
									 0.5f - 3 * UI_TITLE_SCALE * CHARACTER_BASE_SIZE_Y * 0.5f,
									UI_TITLE_SCALE,
									BUTTON1_OFFSET,
									BUTTON1_OFFSET,
									"XCraft",
									&empty,
									&empty) );
	GLL_add ( &world_selection_button_list,
			  ui_create_button_fit ( ccaligned(" Continue ", UI_SCALE),
									 1.0f,
									UI_SCALE,
									BUTTON2_OFFSET,
									BUTTON3_OFFSET,
									" Continue ",
									&empty,
									&continue_to_game));
	GLL_add ( &world_selection_button_list,
			  ui_create_button_fit ( ccaligned("Regenerate", UI_SCALE),
									 1.0f + 2*UI_SCALE*CHARACTER_BASE_SIZE_Y,
									UI_SCALE,
									BUTTON2_OFFSET,
									BUTTON3_OFFSET,
									"Regenerate",
									&empty,
									&transition_regen) );
	GLL_add ( &world_selection_button_list,
			  ui_create_button_fit ( ccaligned("   Back   ", UI_SCALE),
									 1.0f + 4*UI_SCALE*CHARACTER_BASE_SIZE_Y,
									UI_SCALE,
									BUTTON2_OFFSET,
									BUTTON3_OFFSET,
									"   Back   ",
									&empty,
									&transition_to_main_menu) );

	state_stack = GLL_init();
	state_update_stack = GLL_init();

	GLL_add(&state_update_stack, &transition_to_main_menu);

}


void exit_game () {

	dump_player_data();
	
	clean_structure_cache();

	GLL_free_rec_btn (&main_menu_button_list);
	GLL_destroy (&main_menu_button_list);
	GLL_free_rec_btn (&world_selection_button_list);
	GLL_destroy(&world_selection_button_list);
	GLL_free (&state_stack);
	GLL_destroy(&state_stack);
	GLL_free (&state_update_stack);
	GLL_destroy (&state_update_stack);
}
