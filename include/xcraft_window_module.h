#ifndef XCRAFTWM
#define XCRAFTWM

#include <stdbool.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysymdef.h>
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
bool xg_keyboard_modif (uint32_t keysym);
void xg_mouse_position (int32_t* x,int32_t* y);
void xg_set_mouse_position (int32_t x, int32_t y);
void xg_cursor_set (bool vis, uint8_t shape);

void emptyfnc (bool b);
void xg_set_button1_callback (void (*fncptr)(bool));
void xg_set_button2_callback (void (*fncptr)(bool));
void xg_set_button3_callback (void (*fncptr)(bool));
void xg_set_button4_callback (void (*fncptr)(bool));
void xg_set_button5_callback (void (*fncptr)(bool));

#endif
