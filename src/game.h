#ifndef GAME
#define GAME

extern void (*input_state) (float);
extern void (*render_state)();
extern void (*debug_overlay_state)(float);

void init_game ();

void world_input_state  (float fTime);
void world_render_state ();
void debug_fps_pos_state(float fTime);

#endif
