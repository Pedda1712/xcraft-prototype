#ifndef GAME
#define GAME

#include <stdint.h>
#include <stdbool.h>
#include <GL/gl.h>
#include <physics.h>

extern struct game_state_t {
	float _player_fov;
	float _player_x;
	float _player_y;
	float _player_z;
	float _dir_x;
	float _dir_y;
	float _dir_z;
	float _angle_x;
	float _angle_y;
	float _mouse_x;
	float _mouse_y;
	int32_t _p_chunk_x;
	int32_t _p_chunk_z;
	int32_t _player_range;
	GLuint _atlas_texture;
	GLuint _text_font;
	GLuint _gfx_font;
	GLuint _shader_prg;
	float _skycolor[4];
	int32_t _selected_block;
	struct pbox_t _player_box;
} gst;

extern struct GLL state_stack;
extern struct GLL state_update_stack;

void init_game ();
void exit_game ();

void empty ();

void world_input_state  (float fTime);
void world_render_state (float fTime);
void world_overlay_state (float fTime);
void debug_fps_pos_state(float fTime);

void menu_input_state (float fTime);
void menu_overlay_state (float fTime);


#endif
