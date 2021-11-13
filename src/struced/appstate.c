#include <struced/appstate.h>
#include <struced/octree.h>
#include <struced/clicktool.h>

#include <GL/glew.h>
#include <xcraft_window_module.h>

#include <windowglobals.h>
#include <bmp24.h>
#include <bmpfont.h>
#include <ui.h>
#include <worlddefs.h>
#include <raycast.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_NUMBER_DIGITS 10

#define LINE_LENGTH 10

#define EDITOR_FOV 70.0f

struct appstate_info_t ast = {
	0,
	0,
	{0,0},
	false,
	{0.0f,0.0f},
	1.0f,
	70.0f,
	1,
	0,
	NULL,
	NULL
};

struct GLL current_button_list;

struct GLL app_state_stack;
struct GLL app_state_modifier_stack;

struct GLL main_button_list;
struct GLL blockset_input_button_list;

struct OCT_node_t* block_tree = NULL;

bool enable_text_input = false;
char* input_dest = NULL;
int on_char = 0;

void rem_block_octree (struct block_t n_block){

	if(OCT_check(block_tree, n_block))
	if(ast._placed_blocks > 0){
		block_tree = OCT_remove(block_tree, n_block);
		ast._placed_blocks--;
	}
}

void add_block_octree (struct block_t n_block){
	if(ast._placed_blocks == 0){
		block_tree = OCT_alloc (n_block);
		ast._placed_blocks++;
	}else{

		if (OCT_check(block_tree, n_block)){ // If block already exists at position
			rem_block_octree(n_block);
			add_block_octree(n_block);
			return;
		}else{
			OCT_insert(block_tree, n_block);
			ast._placed_blocks++;
		}
	}
}

static void leftclick_callback (bool p){
	float rel_mouse [2];
	rel_mouse[0] = ((float)ast._mouse_position[0] / width ) * 2;
	rel_mouse[1] = ((float)ast._mouse_position[1] / height) * 2;
	
	if (ast._left_click_callback != NULL){
		(*ast._left_click_callback)(p, rel_mouse[0]-1, rel_mouse[1]-1);
	}

	ast._current_mouse_status = p;
	for( struct GLL_element* e = current_button_list.first; e != NULL; e = e->next){
		struct button_t* bt = (struct button_t*) e->data;

		if ( rel_mouse[0] > bt->_x && rel_mouse[0] < (bt->_x + bt->forg->width * CHARACTER_BASE_SIZE_X * bt->_scale) && rel_mouse[1] > bt->_y && rel_mouse[1] < ((bt->_y + bt->forg->height * CHARACTER_BASE_SIZE_Y * bt->_scale))){
			if(p){
				(*bt->clicked_func)(bt);
			}
		}
	}
}

static void rightclick_callback (bool p) {

	float rel_mouse [2];
	rel_mouse[0] = ((float)ast._mouse_position[0] / width ) * 2;
	rel_mouse[1] = ((float)ast._mouse_position[1] / height) * 2;
	
	if (ast._right_click_callback != NULL){
		(*ast._right_click_callback)(p, rel_mouse[0]-1, rel_mouse[1]-1);
	}
}

static void scroll_pos_callback (bool p){
	if(p){
		ast._camera_rad += 0.5f;
	}
}

static void scroll_neg_callback (bool p){
	if(p){
		ast._camera_rad -= 0.5f;

		ast._camera_rad = (ast._camera_rad > 0.0f) ? ast._camera_rad : 0.0f;
	}
}

static void text_input_field_enable_callback ( struct button_t* bt){ // When a Text Input field is selected
	if(!enable_text_input){ // If no field was previously selected

		enable_text_input = !enable_text_input;
		input_dest = bt->forg->chars + bt->forg->width + 1;
		struct paragraph_t* temp = bt->backg;
		bt->backg = bt->backh;
		bt->backh = temp;
		on_char = 0;

	}else { // If a field was previously selected

		if((bt->forg->chars + bt->forg->width + 1) == input_dest){ // If it was the current field then just disable it and stop accepting text input
			enable_text_input = ! enable_text_input;
			struct paragraph_t* temp = bt->backg;
			bt->backg = bt->backh;
			bt->backh = temp;
		}else{ // If it was a different one ...
			for(struct GLL_element* e = current_button_list.first; e != NULL; e = e->next){ // Search for the corresponding input
				struct button_t* cur = e->data;
				if((cur->forg->chars + cur->forg->width + 1) == input_dest){ // If found, disable the old input, and recall function to enable new one
					enable_text_input = ! enable_text_input;
					struct paragraph_t* temp = cur->backg;
					cur->backg = cur->backh;
					cur->backh = temp;
					text_input_field_enable_callback (bt);
					break;
				}
			}
		}
	}
}

static void text_input_callback (bool p, KeySym k, char c){
	if(p && enable_text_input){
		if((c >= '0' && c <= '9' || c == '-') && on_char < MAX_NUMBER_DIGITS){ // If c is a number and MAX_NUMBER_DIGITS is not reached
			input_dest[on_char] = c;
			on_char++;
		}else{
			memset(input_dest, 0, MAX_NUMBER_DIGITS);
			on_char = 0;
		}
	}
}

static void addblock_callback () {
	struct GLL_element* e = blockset_input_button_list.first;
	int pos [3];
	for(int i = 0; i < 3; i++){
		struct button_t* bt = e->data;

		char  f [MAX_NUMBER_DIGITS];
		char* o = bt->forg->chars + bt->forg->width + 1;

		memcpy (f, o, MAX_NUMBER_DIGITS);

		pos[i] = atoi (f);

		e = e->next;
	}

	struct block_t n_block = {pos[0],pos[1],pos[2], ast._selected_block};
	add_block_octree(n_block);
}

static void blockset_callback (){
	current_button_list = blockset_input_button_list;
}

static void blockset_cancel_callback () {

	if (enable_text_input)
		for(struct GLL_element* e = blockset_input_button_list.first; e != NULL; e = e->next){
			struct button_t* cur = e->data;
			if((cur->forg->chars + cur->forg->width + 1) == input_dest){

				struct paragraph_t* temp = cur->backg;
				cur->backg = cur->backh;
				cur->backh = temp;

				break;
			}
		}

	current_button_list = main_button_list;
	enable_text_input = false;
	input_dest = NULL;
}

static void mousetool_button_callback (struct button_t* bt) {

	struct paragraph_t* temp = bt->backh;
	bt->backh = bt->backg;
	bt->backg = temp;	

	if(ast._left_click_callback == NULL){
		ast._left_click_callback = &break_clicktool;
		ast._right_click_callback = &place_clicktool;
	}else{
		ast._left_click_callback = NULL;
		ast._right_click_callback = NULL;
	}
}

static void increment_selected_block_callback () {
	if(ast._selected_block < BLOCK_TYPE_COUNT - 1){
		ast._selected_block++;
	}
	else if(ast._selected_block == BLOCK_TYPE_COUNT - 1){ast._selected_block = 0;}
}
static void decrement_selected_block_callback () {
	if(ast._selected_block > 0){
		ast._selected_block--;
	}
	else if(ast._selected_block == 0){ast._selected_block = BLOCK_TYPE_COUNT - 1;}
}

void app_input_state (float){

	int32_t new_pos [2];
	xg_mouse_position (&new_pos[0], &new_pos[1]);
	if( ast._current_mouse_status){
		int sign [2] = {1, -1};
		for (int i = 0; i < 2; i++) {ast._camera_pos[i] += ((float)ast._mouse_position[i] - (float)new_pos[i]) * 0.001f * sign[i];}

		if(ast._camera_pos[1] >  0.5f * 3.14159f) ast._camera_pos[1] =  0.5f * 3.14159f;
		if(ast._camera_pos[1] < 0.5f * -3.14159f) ast._camera_pos[1] = 0.5f * -3.14159f;

	}
	for(int i = 0; i < 2; i++) {ast._mouse_position[i] = new_pos[i];}
}

void app_overlay_draw_state (float){
	setupfont();

	setfont(ast._atlas_font);
	char str [1];
	*str = btd_map[ast._selected_block].index[2];
	drawstring (str, 2.0f - UI_SCALE * CHARACTER_BASE_SIZE_X * 3, UI_SCALE * CHARACTER_BASE_SIZE_Y * 3, UI_SCALE * 3);

	setfont(ast._text_font);
	drawstring("XCraft Structure Editor", 0.0f, 0.0f, 0.44f);
	for( struct GLL_element* e = current_button_list.first; e != NULL; e = e->next){
		struct button_t* bt = (struct button_t*) e->data;

		float rel_mouse [2];
		rel_mouse[0] = ((float)ast._mouse_position[0] / width ) * 2;
		rel_mouse[1] = ((float)ast._mouse_position[1] / height) * 2;
		if ( rel_mouse[0] > bt->_x && rel_mouse[0] < (bt->_x + bt->forg->width * CHARACTER_BASE_SIZE_X * bt->_scale) && rel_mouse[1] > bt->_y && rel_mouse[1] < ((bt->_y + bt->forg->height * CHARACTER_BASE_SIZE_Y * bt->_scale))){
			drawtextpg (bt->backh, bt->_x, bt->_y, bt->_scale);
		}else{
			drawtextpg (bt->backg, bt->_x, bt->_y, bt->_scale);
		}

		drawtextpg (bt->forg, bt->_x, bt->_y, bt->_scale);
	}
	revertfont();
}

void app_editor_draw_state  (float){
	// Push Matricies

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(sin(ast._camera_pos[0]) * ast._camera_rad * cos(ast._camera_pos[1]), sin(ast._camera_pos[1]) * ast._camera_rad, cos(ast._camera_pos[0]) * ast._camera_rad * cos(ast._camera_pos[1]), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(ast._fov,(GLfloat)width/(GLfloat)height,0.1,500.0);


	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);

	glBindTexture(GL_TEXTURE_2D, ast._atlas_font);

	if(ast._placed_blocks > 0){

		void draw_cycle (struct OCT_node_t* n){

			struct blocktexdef_t tex = btd_map[n->data->_type];

			float get_texX (uint8_t ind) {return ((ind * 16) % 256) / 256.0f;}
			float get_texY (uint8_t ind) {return ((ind * 16) / 256) / 256.0f;}

			float ctx, cty;

			glBegin (GL_QUADS);

			ctx = get_texX (tex.index[0]); cty = get_texY (tex.index[0]);
			glTexCoord2f (ctx, cty);
			glVertex3f(0.0f + n->data->_x, 0.0f + n->data->_y, 0.0f + n->data->_z);
			glTexCoord2f (ctx + 1.f/16.f, cty);
			glVertex3f(1.0f + n->data->_x, 0.0f + n->data->_y, 0.0f + n->data->_z);
			glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
			glVertex3f(1.0f + n->data->_x, 0.0f + n->data->_y, 1.0f + n->data->_z);
			glTexCoord2f (ctx, cty + 1.f/16.f);
			glVertex3f(0.0f + n->data->_x, 0.0f + n->data->_y, 1.0f + n->data->_z);

			ctx = get_texX (tex.index[1]); cty = get_texY (tex.index[1]);
			glTexCoord2f (ctx, cty);
			glVertex3f(0.0f + n->data->_x, 1.0f + n->data->_y, 0.0f + n->data->_z);
			glTexCoord2f (ctx + 1.f/16.f, cty);
			glVertex3f(1.0f + n->data->_x, 1.0f + n->data->_y, 0.0f + n->data->_z);
			glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
			glVertex3f(1.0f + n->data->_x, 1.0f + n->data->_y, 1.0f + n->data->_z);
			glTexCoord2f (ctx, cty + 1.f/16.f);
			glVertex3f(0.0f + n->data->_x, 1.0f + n->data->_y, 1.0f + n->data->_z);

			ctx = get_texX (tex.index[2]); cty = get_texY (tex.index[2]);
			glTexCoord2f (ctx, cty + 1.f/16.f);
			glVertex3f(0.0f + n->data->_x,0.0f  + n->data->_y,0.0f  + n->data->_z);
			glTexCoord2f (ctx, cty);
			glVertex3f(0.0f + n->data->_x,1.0f  + n->data->_y,0.0f  + n->data->_z);
			glTexCoord2f (ctx + 1.f/16.f, cty);
			glVertex3f(0.0f + n->data->_x,1.0f  + n->data->_y,1.0f  + n->data->_z);
			glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
			glVertex3f(0.0f + n->data->_x,0.0f  + n->data->_y,1.0f  + n->data->_z);

			ctx = get_texX (tex.index[3]); cty = get_texY (tex.index[3]);
			glTexCoord2f (ctx, cty + 1.f/16.f);
			glVertex3f(1.0f + n->data->_x,0.0f  + n->data->_y,0.0f  + n->data->_z);
			glTexCoord2f (ctx, cty);
			glVertex3f(1.0f + n->data->_x,1.0f  + n->data->_y,0.0f  + n->data->_z);
			glTexCoord2f (ctx + 1.f/16.f, cty);
			glVertex3f(1.0f + n->data->_x,1.0f  + n->data->_y,1.0f  + n->data->_z);
			glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
			glVertex3f(1.0f + n->data->_x,0.0f  + n->data->_y,1.0f  + n->data->_z);

			ctx = get_texX (tex.index[4]); cty = get_texY (tex.index[4]);
			glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
			glVertex3f(0.0f + n->data->_x,0.0f  + n->data->_y,0.0f  + n->data->_z);
			glTexCoord2f (ctx, cty + 1.f/16.f);
			glVertex3f(1.0f + n->data->_x,0.0f  + n->data->_y,0.0f  + n->data->_z);
			glTexCoord2f (ctx, cty);
			glVertex3f(1.0f + n->data->_x,1.0f  + n->data->_y,0.0f  + n->data->_z);
			glTexCoord2f (ctx + 1.f/16.f, cty);
			glVertex3f(0.0f + n->data->_x,1.0f  + n->data->_y,0.0f  + n->data->_z);

			ctx = get_texX (tex.index[5]); cty = get_texY (tex.index[5]);
			glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
			glVertex3f(0.0f + n->data->_x,0.0f  + n->data->_y,1.0f  + n->data->_z);
			glTexCoord2f (ctx, cty + 1.f/16.f);
			glVertex3f(1.0f + n->data->_x,0.0f  + n->data->_y,1.0f  + n->data->_z);
			glTexCoord2f (ctx, cty);
			glVertex3f(1.0f + n->data->_x,1.0f  + n->data->_y,1.0f  + n->data->_z);
			glTexCoord2f (ctx + 1.f/16.f, cty);
			glVertex3f(0.0f + n->data->_x,1.0f  + n->data->_y,1.0f  + n->data->_z);

			glEnd();

		}

		OCT_operation (block_tree, &draw_cycle);

	}
	
	glDisable(GL_TEXTURE_2D);

	glBegin(GL_LINES);

	glColor3f  (0.0f, 1.0f, 0.0f);
	glVertex3f (1.0f  * LINE_LENGTH, 0.0f, 0.0f);
	glColor3f  (0.0f, 1.0f, 0.0f);
	glVertex3f (-1.0f * LINE_LENGTH,0.0f, 0.0f);

	glColor3f  (1.0f, 0.0f, 0.0f);
	glVertex3f (0.0f, 1.0f * LINE_LENGTH, 0.0f);
	glColor3f  (1.0f, 0.0f, 0.0f);
	glVertex3f (0.0f,-1.0f * LINE_LENGTH, 0.0f);

	glColor3f  (0.0f, 0.0f, 1.0f);
	glVertex3f (0.0f, 0.0f, 1.0f * LINE_LENGTH);
	glColor3f  (0.0f, 0.0f, 1.0f);
	glVertex3f (0.0f, 0.0f,-1.0f * LINE_LENGTH);
	
	for(int i = -LINE_LENGTH;i <= LINE_LENGTH; i++){
		glColor3f  (1.0f, 0.0f, 1.0f);
		glVertex3f ((float)i, 0.0f, 1.0f * LINE_LENGTH);
		glColor3f  (1.0f, 0.0f, 1.0f);
		glVertex3f ((float)i, 0.0f,-1.0f * LINE_LENGTH);
	}
	for(int i = -LINE_LENGTH;i <= LINE_LENGTH; i++){
		glColor3f  (1.0f, 0.0f, 1.0f);
		glVertex3f (1.0f  * LINE_LENGTH, 0.0f, (float)i);
		glColor3f  (1.0f, 0.0f, 1.0f);
		glVertex3f (-1.0f * LINE_LENGTH, 0.0f, (float)i);
	}

	glColor3f(1.0f, 1.0f, 1.0f);

	glEnd();
	
	glEnable(GL_TEXTURE_2D);

}

static void empty(){}
void init_app_state_system (){

	app_state_stack =            GLL_init();
	app_state_modifier_stack =   GLL_init();
	main_button_list =           GLL_init();
	blockset_input_button_list = GLL_init();
	ast._text_font =  loadfont("font.bmp" , GL_NEAREST);
	ast._atlas_font = loadfont("atlas.bmp", GL_NEAREST);
	loadblockdef("blockdef.btd");
	setfont(ast._text_font);

	/*
		Creating Buttons for the Tools
	 */
	 
	 GLL_add(&main_button_list,
			ui_create_button_fit(
				0.0f,
				UI_SCALE * CHARACTER_BASE_SIZE_Y * 6,
				UI_SCALE,
				BUTTON2_OFFSET,
				BUTTON6_OFFSET,
				"Click",
				&empty,
				&mousetool_button_callback
			));
	 
	GLL_add(&main_button_list,
			ui_create_button_fit(
				0.0f,
				UI_SCALE * CHARACTER_BASE_SIZE_Y * 2,
				UI_SCALE,
				BUTTON2_OFFSET,
				BUTTON3_OFFSET,
				" Set ",
				&empty,
				&blockset_callback
			));
	GLL_add(&main_button_list,
			ui_create_button_fit(
				2.0f - UI_SCALE * CHARACTER_BASE_SIZE_X * 3,
				0.0f,
				UI_SCALE,
				BUTTON6_OFFSET,
				BUTTON7_OFFSET,
				"+",
				&empty,
				&increment_selected_block_callback
			));
	GLL_add(&main_button_list,
			ui_create_button_fit(
				2.0f - UI_SCALE * CHARACTER_BASE_SIZE_X * 3,
				UI_SCALE * CHARACTER_BASE_SIZE_Y * 6,
				UI_SCALE,
				BUTTON4_OFFSET,
				BUTTON5_OFFSET,
				"-",
				&empty,
				&decrement_selected_block_callback
			));

	/*
	 * Creating Buttons for the "Set Block" input menu
	 */
	float ccaligned ( char* s, float sc ) {return (1.0f - (strlen(s) + 2) * sc * CHARACTER_BASE_SIZE_X * 0.5f);}
	GLL_add(&blockset_input_button_list,
			ui_create_button_fit(
				ccaligned("          ", UI_SCALE),
				1.0f - UI_SCALE * CHARACTER_BASE_SIZE_Y * 6,
				UI_SCALE,
				BUTTON3_OFFSET,
				BUTTON2_OFFSET,
				"          ",
				&empty,
				&text_input_field_enable_callback
			));
	GLL_add(&blockset_input_button_list,
			ui_create_button_fit(
				ccaligned("          ", UI_SCALE),
				1.0f - UI_SCALE * CHARACTER_BASE_SIZE_Y * 3,
				UI_SCALE,
				BUTTON3_OFFSET,
				BUTTON2_OFFSET,
				"          ",
				&empty,
				&text_input_field_enable_callback
			));
	GLL_add(&blockset_input_button_list,
			ui_create_button_fit(
				ccaligned("          ", UI_SCALE),
				1.0f,
				UI_SCALE,
				BUTTON3_OFFSET,
				BUTTON2_OFFSET,
				"          ",
				&empty,
				&text_input_field_enable_callback
			));
	GLL_add(&blockset_input_button_list,
			ui_create_button_fit(
				ccaligned("+", UI_SCALE),
				1.0f + UI_SCALE * CHARACTER_BASE_SIZE_Y * 3,
				UI_SCALE,
				BUTTON6_OFFSET,
				BUTTON7_OFFSET,
				"+",
				&empty,
				&addblock_callback
			));
	GLL_add(&blockset_input_button_list,
			ui_create_button_fit(
				ccaligned("   Back   ", UI_SCALE),
				1.0f + UI_SCALE * CHARACTER_BASE_SIZE_Y * 6,
				UI_SCALE,
				BUTTON4_OFFSET,
				BUTTON5_OFFSET,
				"   Back   ",
				&empty,
				&blockset_cancel_callback
			));
	GLL_add(&blockset_input_button_list,
			ui_create_button_fit(
				2.0f - UI_SCALE * CHARACTER_BASE_SIZE_X * 3,
				0.0f,
				UI_SCALE,
				BUTTON6_OFFSET,
				BUTTON7_OFFSET,
				"+",
				&empty,
				&increment_selected_block_callback
			));
	GLL_add(&blockset_input_button_list,
			ui_create_button_fit(
				2.0f - UI_SCALE * CHARACTER_BASE_SIZE_X * 3,
				UI_SCALE * CHARACTER_BASE_SIZE_Y * 6,
				UI_SCALE,
				BUTTON4_OFFSET,
				BUTTON5_OFFSET,
				"-",
				&empty,
				&decrement_selected_block_callback
			));

	current_button_list = main_button_list;

	xg_set_button1_callback (&leftclick_callback);
	xg_set_button3_callback (&rightclick_callback);
	xg_set_button4_callback (&scroll_pos_callback);
	xg_set_button5_callback (&scroll_neg_callback);

	xg_set_keyboard_callback(&text_input_callback);
	
	for(int x = -1; x <= 1; x++){
		for(int z = -1; z <= 1; z++){
			struct block_t btt = {x, -1, z, GRASS_B};
			add_block_octree(btt);
		}
	}

}

void exit_app_state_system (){
	GLL_free(&app_state_stack);                   GLL_destroy(&app_state_stack);
	GLL_free(&app_state_modifier_stack);          GLL_destroy(&app_state_modifier_stack);
	GLL_free_rec_btn(&main_button_list);          GLL_destroy(&main_button_list);
	GLL_free_rec_btn(&blockset_input_button_list);GLL_destroy(&blockset_input_button_list);

	OCT_operation(block_tree, &OCT_operator_free);
}
