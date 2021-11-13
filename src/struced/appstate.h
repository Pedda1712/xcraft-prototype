#ifndef APPSTATE
#define APPSTATE

#include <genericlist.h>
#include <blocktexturedef.h>

#include <stdbool.h>

extern struct appstate_info_t {
	unsigned int _text_font;
	unsigned int _atlas_font;
	int32_t _mouse_position [2];
	bool _current_mouse_status;
	float _camera_pos [2];
	float _camera_rad;
	float _fov;
	uint8_t _selected_block;
	int32_t _placed_blocks;
	void (*_left_click_callback)(bool, float, float);
	void (*_right_click_callback)(bool, float, float);
} ast;

extern struct OCT_node_t* block_tree;

extern struct GLL app_state_stack;
extern struct GLL app_state_modifier_stack;

void app_overlay_draw_state     (float);
void app_editor_draw_state      (float);
void app_input_state            (float);
void app_text_input_state       (float);

void init_app_state_system ();
void exit_app_state_system ();

#endif
