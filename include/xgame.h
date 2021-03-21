#ifndef LINWIN
#define LINWIN

#include <stdbool.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

// Windowing and GLX Functions
bool lw_init ();
void lw_window (uint32_t width, uint32_t height, char* title);
void lw_window_show ();
void lw_init_glx ();
void lw_window_close ();
void lw_window_stop ();
void lw_window_set_not_resizable ();
bool lw_window_isopen ();
void lw_window_update ();
void lw_glx_swap ();

//GameDev functions
float lw_get_ftime ();

// Keyboard Input Functions
bool lw_keyboard_ascii (uint32_t key);
void lw_mouse_position (int32_t* x,int32_t* y);
void lw_set_mouse_position (int32_t x, int32_t y);
void lw_cursor_visible (bool vis);

#endif
