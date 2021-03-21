#ifndef LINWIN
#define LINWIN

#include <stdbool.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

// Windowing and GLX Functions
bool xg_init ();
void xg_window (uint32_t width, uint32_t height, char* title);
void xg_window_show ();
void xg_init_glx ();
void xg_window_close ();
void xg_window_stop ();
void xg_window_set_not_resizable ();
bool xg_window_isopen ();
void xg_window_update ();
void xg_glx_swap ();

//GameDev functions
float xg_get_ftime ();

// Keyboard Input Functions
bool xg_keyboard_ascii (uint32_t key);
void xg_mouse_position (int32_t* x,int32_t* y);
void xg_set_mouse_position (int32_t x, int32_t y);
void xg_cursor_visible (bool vis);

#endif
