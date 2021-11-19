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

#include <ftw.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_INPUT_LENGTH 10

#define LINE_LENGTH 10

#define EDITOR_FOV 70.0f

#define POINT_CLICK_SHAPE 0
#define CIRCLE_CLICK_SHAPE 1
#define SQUARE_CLICK_SHAPE 2

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
	NULL,
	{},
	1,
	POINT_CLICK_SHAPE
};

struct GLL app_state_stack;
struct GLL app_state_modifier_stack;

struct GLL current_button_list;
struct GLL main_button_list;
struct GLL blockset_input_button_list;
struct GLL io_interface_list;

struct OCT_node_t* block_tree = NULL;

bool enable_text_input = false;
bool (*character_validator_fnc)(char);
char* input_dest = NULL;
int on_char = 0;

void rem_block_octree (struct block_t n_block){ // removes a block from the octree

	if(OCT_check(block_tree, n_block)) // If there is a block at the given location
	if(ast._placed_blocks > 0){
		block_tree = OCT_remove(block_tree, n_block);
		ast._placed_blocks--;
	}
}

void add_block_octree (struct block_t n_block){ // adds a block to the octree
	if(ast._placed_blocks == 0){ // If there arent any blocks currently
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

static void leftclick_callback (bool p){ // gets called on every left click p is true when leftclick is performed, false when it is lifted
	float rel_mouse [2];
	rel_mouse[0] = ((float)ast._mouse_position[0] / width ) * 2;
	rel_mouse[1] = ((float)ast._mouse_position[1] / height) * 2;

	bool button_priority = false;
	
	ast._current_mouse_status = p; // update the current mouse status
	for( struct GLL_element* e = current_button_list.first; e != NULL; e = e->next){ // go through the current button list 
		struct button_t* bt = (struct button_t*) e->data;

		if ( rel_mouse[0] > bt->_x && rel_mouse[0] < (bt->_x + bt->forg->width * CHARACTER_BASE_SIZE_X * bt->_scale) && rel_mouse[1] > bt->_y && rel_mouse[1] < ((bt->_y + bt->forg->height * CHARACTER_BASE_SIZE_Y * bt->_scale))){ // If mouse pointer is withing bounds of current button
			if(p){ // and leftclick is being pressed
				button_priority = true;
				(*bt->clicked_func)(bt); // call the buttons corresponding leftclick function pointer
			}
		}
	}
	
	if (ast._left_click_callback != NULL && !button_priority){ // If there is a user-defined callback (for example block manipulation) call it
		(*ast._left_click_callback)(p, rel_mouse[0]-1, rel_mouse[1]-1);
	}
}

static void rightclick_callback (bool p) {

	float rel_mouse [2];
	rel_mouse[0] = ((float)ast._mouse_position[0] / width ) * 2;
	rel_mouse[1] = ((float)ast._mouse_position[1] / height) * 2;
	
	if (ast._right_click_callback != NULL){ // when a user-defined rightclick_callback exists, call it
		(*ast._right_click_callback)(p, rel_mouse[0]-1, rel_mouse[1]-1);
	}
}

static void scroll_pos_zoom_callback (bool p){ // change distance from center when scrolling ... (mouse wheel up callback), when Shift is pressed change Selection Radius instead
	if(p){
		if(!xg_keyboard_modif(XK_Shift_L)){
			ast._camera_rad += 0.5f;
		}else{
			if(ast._scroll_radius <= 32)
				ast._scroll_radius++;
			else
				ast._scroll_radius = 1;
		}
	}
}

static void scroll_neg_zoom_callback (bool p){ // change distance from center when scrolling ... (mouse wheel down callback), when Shift is pressed change Selection Radius instead
	if(p){
		if(!xg_keyboard_modif(XK_Shift_L)){
			ast._camera_rad -= 0.5f;
			ast._camera_rad = (ast._camera_rad > 0.0f) ? ast._camera_rad : 0.0f;
			
		}else{
			if(ast._scroll_radius >= 1)
				ast._scroll_radius--;
		}
	}
}

static void rectangle_callback (struct button_t* bt) {
	if(ast._click_shape == POINT_CLICK_SHAPE){
		ast._click_shape = SQUARE_CLICK_SHAPE;
		struct paragraph_t* temp = bt->backg;
		bt->backg = bt->backh;
		bt->backh = temp;
		on_char = 0;
		
	}else if(ast._click_shape == SQUARE_CLICK_SHAPE){
		ast._click_shape = POINT_CLICK_SHAPE;
		struct paragraph_t* temp = bt->backg;
		bt->backg = bt->backh;
		bt->backh = temp;
		on_char = 0;
	}
}

static void circle_callback (struct button_t* bt) {
	if(ast._click_shape == POINT_CLICK_SHAPE){
		ast._click_shape = CIRCLE_CLICK_SHAPE;
		struct paragraph_t* temp = bt->backg;
		bt->backg = bt->backh;
		bt->backh = temp;
		on_char = 0;
	}else if (ast._click_shape == CIRCLE_CLICK_SHAPE){
		ast._click_shape = POINT_CLICK_SHAPE;
		struct paragraph_t* temp = bt->backg;
		bt->backg = bt->backh;
		bt->backh = temp;
		on_char = 0;
	}
}

static void text_input_field_enable_callback ( struct button_t* bt){ // When a Text Input field is selected
	if(!enable_text_input){ // If no field was previously selected

		enable_text_input = !enable_text_input;
		input_dest = bt->forg->chars + 1;
		struct paragraph_t* temp = bt->backg;
		bt->backg = bt->backh;
		bt->backh = temp;
		on_char = 0;

	}else { // If a field was previously selected

		if((bt->forg->chars + 1) == input_dest){ // If it was the current field then just disable it and stop accepting text input
			enable_text_input = ! enable_text_input;
			struct paragraph_t* temp = bt->backg;
			bt->backg = bt->backh;
			bt->backh = temp;
		}else{ // If it was a different one ...
			for(struct GLL_element* e = current_button_list.first; e != NULL; e = e->next){ // Search for the corresponding input
				struct button_t* cur = e->data;
				if((cur->forg->chars + 1) == input_dest){ // If found, disable the old input, and recall function to enable new one
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

static bool character_validator_number (char c){
	return (c >= '0' && c <= '9' || c == '-');
}

static bool character_validator_filename (char c){
	return (c >= '0' && c <= '9' || c == '-'|| c >= 'a' && c <= 'z');
}

static void text_input_callback (bool p, KeySym k, char c){ // gets called when a KeyEvent happens 
	if(p && enable_text_input){ // when key is pressed and textinput is enabled (by button for example)
		if( (*character_validator_fnc)(c) && on_char < MAX_INPUT_LENGTH){ // If c is a number and MAX_INPUT_LENGTH is not reached
			input_dest[on_char] = c; // write it to the destination pointer (has to be set by button that enables text input)
			on_char++;
		}else{
			memset(input_dest, 0, MAX_INPUT_LENGTH);
			on_char = 0;
		}
	}
}

static void addblock_callback () { // when the "+" button in the Set-Submenu gets clicked
	struct GLL_element* e = blockset_input_button_list.first;
	int pos [3];
	for(int i = 0; i < 3; i++){ // retrieve the content of the first three buttons of the current list (these have to be the input text box buttons)
		struct button_t* bt = e->data;

		char  f [MAX_INPUT_LENGTH];
		char* o = bt->forg->chars + 1;

		memcpy (f, o, MAX_INPUT_LENGTH);

		pos[i] = atoi (f);

		e = e->next;
	}

	struct block_t n_block = {pos[0],pos[1],pos[2], ast._selected_block};
	add_block_octree(n_block);
}

static void blockset_callback (){ // "Set" Button
	character_validator_fnc = &character_validator_number;
	if(ast._left_click_callback == NULL) // Only allow changing to Set-Menue from Default editor state
		current_button_list = blockset_input_button_list;
}

static void blockset_cancel_callback () { // This callback leads back from the "Set" Submenu to the Main Overlay

	if (enable_text_input)
		for(struct GLL_element* e = blockset_input_button_list.first; e != NULL; e = e->next){ // If any Text Fields are still enabled
			struct button_t* cur = e->data;
			if((cur->forg->chars + 1) == input_dest){

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

static void state_change_select_highlight_remove (){
	GLL_rem(&app_state_stack, &app_selection_draw_state);
}

static void mousetool_button_callback (struct button_t* bt) { // When the "Click" Button is selected

	struct paragraph_t* temp = bt->backh;
	bt->backh = bt->backg;
	bt->backg = temp;	

	// behaves like a toggle-switch:
	if(ast._left_click_callback == NULL){
		ast._left_click_callback = &break_clicktool;
		ast._right_click_callback = &place_clicktool;
		GLL_add(&app_state_stack, &app_selection_draw_state);

	}else{
		ast._left_click_callback = NULL;
		ast._right_click_callback = NULL;
		
		GLL_add(&app_state_modifier_stack, &state_change_select_highlight_remove);
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

static void io_interface_callback () {
	character_validator_fnc = &character_validator_filename;
	current_button_list = io_interface_list;
}

static void io_interface_cancel_callback () {
	current_button_list = main_button_list;
	
	if (enable_text_input){
		for(struct GLL_element* e = io_interface_list.first; e != NULL; e = e->next){ // If any Text Fields are still enabled
			struct button_t* cur = e->data;
			if((cur->forg->chars + 1) == input_dest){

				struct paragraph_t* temp = cur->backg;
				cur->backg = cur->backh;
				cur->backh = temp;

				break;
			}
		}
		enable_text_input = false;
	}
}

static void construct_fname_from_tinput (char* str){
	
	struct button_t* text_field = io_interface_list.first->data;
	memcpy(str + 6, text_field->forg->chars + 1, MAX_INPUT_LENGTH);
	
	int len;
	for(len = 0; len < MAX_INPUT_LENGTH; len++){if(!character_validator_filename(str[len+6])){break;}}
	memcpy(str + len + 6, ".struc\0", 7);
}

static void save_file_callback () {
	
	struct stat sb;
	if (!(stat("struc", &sb) == 0 && S_ISDIR(sb.st_mode))){
		mkdir("struc", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	
	char fname [MAX_INPUT_LENGTH + 12] = "struc/";
	construct_fname_from_tinput(fname);
	
	FILE* f = fopen(fname, "wb");
	if (f != NULL){
		fwrite(&ast._placed_blocks, sizeof(int32_t), 1, f); // Write how many blocks are in file at the beginning
		
		void OCT_save_operator (struct OCT_node_t* n){
			struct block_t* cur = n->data;
			fwrite (cur, sizeof(struct block_t), 1, f);
		}
		
		OCT_operation(block_tree, &OCT_save_operator);
		
		fclose(f);
	}else{
		printf("Error opening file: %s\n", fname);
	}
}

static void open_file_callback () {
	char fname [MAX_INPUT_LENGTH + 12] = "struc/";
	construct_fname_from_tinput(fname);
	
	FILE* f = fopen(fname, "rb");
	if (f != NULL){
		
		ast._placed_blocks = 0;
		OCT_operation(block_tree, &OCT_operator_free);
		
		int bcount;
		fread(&bcount, sizeof(int32_t), 1, f); // Write how many blocks are in file at the beginning
		
		for(int i = 0; i < bcount; i++){
			struct block_t cur;
			fread (&cur, sizeof(struct block_t), 1, f);
			add_block_octree(cur);
		}
				
		fclose(f);
	}else{
		printf("Error opening file: %s\n", fname);
	}
}

void app_selection_draw_state   (float){
	
	/*
		Find the center block of the selection, and keep track of which side you hit it from
	 */
	
	float rel_mouse [2];
	rel_mouse[0] = ((float)ast._mouse_position[0] / width ) * 2 -1;
	rel_mouse[1] = ((float)ast._mouse_position[1] / height) * 2 -1;

	float posx = sin(ast._camera_pos[0]) * ast._camera_rad * cos(ast._camera_pos[1]);
	float posy = sin(ast._camera_pos[1]) * ast._camera_rad;
	float posz = cos(ast._camera_pos[0]) * ast._camera_rad * cos(ast._camera_pos[1]);

	float dirx, diry, dirz;
	rotate_direction_by_ssoffset(-rel_mouse[0], rel_mouse[1], -posx, -posy, -posz, &dirx, &diry, &dirz);
	
	
	struct block_t* selection = malloc(sizeof(struct block_t));
	bool selection_orientation[3] = {0,0,0};
	bool select_procedure (int ix, int iy, int iz, float ax, float ay, float az){
		
		float iax = ax - (int)ax;
		float iay = ay - (int)ay;
		float iaz = az - (int)az;

		struct block_t block = {ix, iy, iz, btd_map[ast._selected_block].complete_id};
		if(OCT_check(block_tree, block)){
			
			selection_orientation[0] = (iax == 0.0f) ? true : false;
			selection_orientation[1] = (iay == 0.0f) ? true : false;
			selection_orientation[2] = (iaz == 0.0f) ? true : false;

			*selection = block;
			return true;
		}

		return false;
	}
	
	block_ray_actor_chunkfree (512, posx, posy, posz, dirx, diry, dirz, &select_procedure);
	
	GLL_free_rec(&ast._highlight_list); // Clear the selection list of last frame
	
	/*
		Depending on the Selected Shape, fill around the center that you found with square or circle
	 */
	
	switch(ast._click_shape){
		
		case POINT_CLICK_SHAPE:{
			GLL_add(&ast._highlight_list, selection); 
			break;
			
		}
		case SQUARE_CLICK_SHAPE:{
			
			for(int h = -ast._scroll_radius; h <= ast._scroll_radius; h++){
				for(int k = -ast._scroll_radius; k <= ast._scroll_radius; k++){
					struct block_t* b = malloc(sizeof(struct block_t));
					*b = *selection;
					if(selection_orientation[0]){
						b->_y += k;
						b->_z += h;
					}
					else if(selection_orientation[1]){
						b->_x += k;
						b->_z += h;
					}
					else if(selection_orientation[2]){
						b->_y += k;
						b->_x += h;
					}
					
					GLL_add(&ast._highlight_list, b);
					
				}
			}
			
			free(selection);
			break;
		}
		case CIRCLE_CLICK_SHAPE:{
			for(int h = -ast._scroll_radius; h <= ast._scroll_radius; h++){
				int y = (int)(sqrt((ast._scroll_radius * ast._scroll_radius) - (h * h)));
				for(int k = -y; k <= y; k++){
					struct block_t* b = malloc(sizeof(struct block_t));
					*b = *selection;
					if(selection_orientation[0]){
						b->_y += k;
						b->_z += h;
					}
					else if(selection_orientation[1]){
						b->_x += k;
						b->_z += h;
					}
					else if(selection_orientation[2]){
						b->_y += k;
						b->_x += h;
					}
					
					GLL_add(&ast._highlight_list, b);
					
				}
			}
		}
		
	}
	
	/*
		Set the GL State for drawing 
	 */
	
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	glEnable(GL_BLEND);
	glBlendFunc( GL_SRC_COLOR ,GL_DST_COLOR);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(sin(ast._camera_pos[0]) * ast._camera_rad * cos(ast._camera_pos[1]), sin(ast._camera_pos[1]) * ast._camera_rad, cos(ast._camera_pos[0]) * ast._camera_rad * cos(ast._camera_pos[1]), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(ast._fov,(GLfloat)width/(GLfloat)height,0.1,500.0);
	
	/*
		Define a function that is called for every block in the highlight-list (that just draws the corresponding block)
	 */
	
	void gll_draw_block_operator (struct GLL_element* e) {
		
		struct block_t* n = e->data;
		
		struct block_t test_block = *n;
		
		glBegin (GL_QUADS);
		
		test_block._y--;
		if(!OCT_check(block_tree, test_block)){
			glColor3f(0.0f, 0.33f, 0.44f);
			glVertex3f(0.0f + n->_x, 0.0f + n->_y, 0.0f + n->_z);
			glVertex3f(1.0f + n->_x, 0.0f + n->_y, 0.0f + n->_z);
			glVertex3f(1.0f + n->_x, 0.0f + n->_y, 1.0f + n->_z);
			glVertex3f(0.0f + n->_x, 0.0f + n->_y, 1.0f + n->_z);
		}
		
		test_block = *n;
		test_block._y++;
		if(!OCT_check(block_tree, test_block)){
			glColor3f(0.0f, 0.66f, 0.88f);
			glVertex3f(1.0f + n->_x, 1.0f + n->_y, 1.0f + n->_z);
			glVertex3f(1.0f + n->_x, 1.0f + n->_y, 0.0f + n->_z);
			glVertex3f(0.0f + n->_x, 1.0f + n->_y, 0.0f + n->_z);
			glVertex3f(0.0f + n->_x, 1.0f + n->_y, 1.0f + n->_z);
			
		}

		test_block = *n;
		test_block._x--;
		if(!OCT_check(block_tree, test_block)){
			glColor3f(0.0f, 0.55f, 0.77f);
			glVertex3f(0.0f + n->_x,1.0f  + n->_y,0.0f  + n->_z);
			glVertex3f(0.0f + n->_x,0.0f  + n->_y,0.0f  + n->_z);
			glVertex3f(0.0f + n->_x,0.0f  + n->_y,1.0f  + n->_z);
			glVertex3f(0.0f + n->_x,1.0f  + n->_y,1.0f  + n->_z);
			
		}

		test_block = *n;
		test_block._x++;
		if(!OCT_check(block_tree, test_block)){
			glColor3f(0.0f, 0.55f, 0.77f);
			glVertex3f(1.0f + n->_x,0.0f  + n->_y,0.0f  + n->_z);
			glVertex3f(1.0f + n->_x,1.0f  + n->_y,0.0f  + n->_z);
			glVertex3f(1.0f + n->_x,1.0f  + n->_y,1.0f  + n->_z);
			glVertex3f(1.0f + n->_x,0.0f  + n->_y,1.0f  + n->_z);
			
		}

		test_block = *n;
		test_block._z--;
		if(!OCT_check(block_tree, test_block)){
			glColor3f(0.0f, 0.44f, 0.55f);
			glVertex3f(1.0f + n->_x,0.0f  + n->_y,0.0f  + n->_z);
			glVertex3f(0.0f + n->_x,0.0f  + n->_y,0.0f  + n->_z);
			glVertex3f(0.0f + n->_x,1.0f  + n->_y,0.0f  + n->_z);
			glVertex3f(1.0f + n->_x,1.0f  + n->_y,0.0f  + n->_z);
		}

		test_block = *n;
		test_block._z++;
		if(!OCT_check(block_tree, test_block)){
			glColor3f(0.0f, 0.44f, 0.55f);
			glVertex3f(0.0f + n->_x,0.0f  + n->_y,1.0f  + n->_z);
			glVertex3f(1.0f + n->_x,0.0f  + n->_y,1.0f  + n->_z);
			glVertex3f(1.0f + n->_x,1.0f  + n->_y,1.0f  + n->_z);
			glVertex3f(0.0f + n->_x,1.0f  + n->_y,1.0f  + n->_z);
		}
		
		glColor3f(1.0f, 1.0f, 1.0f);
		
		glEnd();
		
	}
	
	/*
		Apply the Operator you just defined to the list (draw the highlighted blocks)
	 */
	
	GLL_operation(&ast._highlight_list, &gll_draw_block_operator);
	
	/*	
		Reset the GL_State
	 */
	
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

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
	drawstring (str, 2.0f - UI_SCALE * CHARACTER_BASE_SIZE_X * 3, UI_SCALE * CHARACTER_BASE_SIZE_Y * 2, UI_SCALE * 3);

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

		drawtextpg (bt->forg, bt->_x, bt->_y + (bt->_scale * CHARACTER_BASE_SIZE_Y * 0.5f), bt->_scale);
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

		OCT_operation (block_tree, &OCT_operator_gl_draw);

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
	io_interface_list          = GLL_init();
	ast._highlight_list        = GLL_init();
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
				UI_SCALE * CHARACTER_BASE_SIZE_Y * 7,
				UI_SCALE,
				BUTTON2_OFFSET,
				BUTTON3_OFFSET,
				"Click",
				&empty,
				&mousetool_button_callback
			));
	 
	GLL_add(&main_button_list,
			ui_create_button_fit(
				0.0f,
				UI_SCALE * CHARACTER_BASE_SIZE_Y * 4,
				UI_SCALE,
				BUTTON2_OFFSET,
				BUTTON3_OFFSET,
				" Set ",
				&empty,
				&blockset_callback
			));
	GLL_add(&main_button_list,
			ui_create_button_fit(
				0.0,
				UI_SCALE * CHARACTER_BASE_SIZE_Y,
				UI_SCALE,
				BUTTON2_OFFSET,
				BUTTON3_OFFSET,
				" ... ",
				&empty,
				&io_interface_callback
			));
	char symbol = 3; // Rectangle
	GLL_add(&main_button_list,
			ui_create_button_fit(
				UI_SCALE * CHARACTER_BASE_SIZE_X * 3 * (7.0f/6.0f),
				UI_SCALE * CHARACTER_BASE_SIZE_Y * 7.5f * (7.0f/6.0f),
				UI_SCALE * (7.0f/6.0f),
				BUTTON2_OFFSET,
				BUTTON3_OFFSET,
				&symbol,
				&empty,
				&rectangle_callback
			));
	symbol = 4; // Circle
	GLL_add(&main_button_list,
			ui_create_button_fit(
				0.0f,
				UI_SCALE * CHARACTER_BASE_SIZE_Y * 7.5f * (7.0f/6.0f),
				UI_SCALE * (7.0f/6.0f),
				BUTTON2_OFFSET,
				BUTTON3_OFFSET,
				&symbol,
				&empty,
				&circle_callback
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
				UI_SCALE * CHARACTER_BASE_SIZE_Y * 5,
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
				1.0f - UI_SCALE * CHARACTER_BASE_SIZE_Y * 4,
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
				1.0f - UI_SCALE * CHARACTER_BASE_SIZE_Y * 2,
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
				1.0f + UI_SCALE * CHARACTER_BASE_SIZE_Y * 2,
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
				1.0f + UI_SCALE * CHARACTER_BASE_SIZE_Y * 4,
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
				UI_SCALE * CHARACTER_BASE_SIZE_Y * 5,
				UI_SCALE,
				BUTTON4_OFFSET,
				BUTTON5_OFFSET,
				"-",
				&empty,
				&decrement_selected_block_callback
			));
	
	/*
		Creating Buttons for the Open/Save Menu
	 */
	
	GLL_add(&io_interface_list,
			ui_create_button_fit(
				ccaligned("           .struc", UI_SCALE),
				1.0f - UI_SCALE * CHARACTER_BASE_SIZE_Y * 2,
				UI_SCALE,
				BUTTON3_OFFSET,
				BUTTON2_OFFSET,
				"          ",
				&empty,
				&text_input_field_enable_callback
			));
	GLL_add(&io_interface_list,
			ui_create_button_fit(
				ccaligned("           .struc", UI_SCALE) + 12 * CHARACTER_BASE_SIZE_X * UI_SCALE,
				1.0f - UI_SCALE * CHARACTER_BASE_SIZE_Y * 2,
				UI_SCALE,
				BUTTON1_OFFSET,
				BUTTON1_OFFSET,
				".struc",
				&empty,
				&empty
			));
	
	symbol = 1;
	GLL_add(&io_interface_list,
			ui_create_button_fit(
				1.0f - 3.0f * CHARACTER_BASE_SIZE_X * UI_SCALE,
				1.0f,
				UI_SCALE,
				BUTTON6_OFFSET,
				BUTTON7_OFFSET,
				&symbol,
				&empty,
				&open_file_callback
			));
	symbol = 2;
	GLL_add(&io_interface_list,
			ui_create_button_fit(
				1.0f,
				1.0f,
				UI_SCALE,
				BUTTON6_OFFSET,
				BUTTON7_OFFSET,
				&symbol,
				&empty,
				&save_file_callback
			));
	GLL_add(&io_interface_list,
			ui_create_button_fit(
				ccaligned("Back", UI_SCALE),
				1.0f + UI_SCALE * CHARACTER_BASE_SIZE_Y * 2,
				UI_SCALE,
				BUTTON4_OFFSET,
				BUTTON5_OFFSET,
				"Back",
				&empty,
				&io_interface_cancel_callback
			));

	current_button_list = main_button_list;
	
	xg_set_button1_callback (&leftclick_callback);
	xg_set_button3_callback (&rightclick_callback);
	xg_set_button4_callback (&scroll_pos_zoom_callback);
	xg_set_button5_callback (&scroll_neg_zoom_callback);

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
	GLL_free_rec(&ast._highlight_list);			  GLL_destroy(&ast._highlight_list);
	GLL_free_rec_btn(&main_button_list);          GLL_destroy(&main_button_list);
	GLL_free_rec_btn(&blockset_input_button_list);GLL_destroy(&blockset_input_button_list);
	GLL_free_rec_btn(&io_interface_list);         GLL_destroy(&io_interface_list);

	OCT_operation(block_tree, &OCT_operator_free);
}
