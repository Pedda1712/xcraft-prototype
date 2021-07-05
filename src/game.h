#ifndef GAME
#define GAME

#include <stdint.h>
#include <GL/gl.h>
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
	int32_t _p_chunk_x;
	int32_t _p_chunk_z;
	int32_t _player_range;
	GLuint _atlas_texture;
	float _skycolor[4];
	int32_t _selected_block;
} gst;

extern void (*input_state) (float);
extern void (*render_state)();
extern void (*debug_overlay_state)(float);

void init_game ();

void world_input_state  (float fTime);
void world_render_state ();
void debug_fps_pos_state(float fTime);

#endif
