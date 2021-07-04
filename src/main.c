#include <xgame.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include <worlddefs.h>
#include <chunklist.h>
#include <chunkbuilder.h>
#include <dynamicarray.h>
#include <generator.h>
#include <blocktexturedef.h>
#include <windowglobals.h>
#include <bmpfont.h>
#include <globallists.h>

#include <game.h>

#include <bmp24.h>

#include <pnoise.h>

/*
	Minecraft Clone in C, using:
		- Xlib for Windowing
		- Immediate Mode OpenGL for Graphics
 */

uint16_t width = 1440;
uint16_t height = 900;

int main () {

	printf("Initializing X11 Window ... \n");
	xg_init();
	xg_window(width, height, "XCraft");
	xg_window_set_not_resizable();
	xg_init_glx();
	xg_window_show();
	xg_cursor_visible(false);
	
	printf ("Loading BTD 'blockdef.btd' ...\n");
	loadblockdef("blockdef.btd");
	
	printf("Initializing Builder Threads ...\n");
	if(!initialize_builder_thread()){
		printf("Failed to initialize Builder Threads!\n");
		exit(-1);
	}
	
	printf("Initializing Generator Thread ...\n");
	if(!initialize_generator_thread()){
		printf("Failed to initialize Generator Thread!\n");
		exit(-1);
	}
	
	printf("Initializing GST ...\n");
	init_game();
	
	// The GameState setup on startup
	input_state  = &world_input_state;
	render_state = &world_render_state;
	debug_overlay_state = &debug_fps_pos_state;
	
	while(xg_window_isopen()){
		float frameTime = xg_get_ftime();
		
		xg_window_update();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		input_state (frameTime);
		render_state();
		debug_overlay_state(frameTime);

		xg_glx_swap();

	}
	printf("Closing X11 Window ...\n");
	xg_window_close();
	
	printf("Terminating Generator Thread ...\n");
	terminate_generator_thread();
	printf("Terminating Builder Threads ...\n");
	terminate_builder_thread();

	return 0;
}
