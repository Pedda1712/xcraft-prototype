#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include <xcraft_window_module.h>
#include <chunkbuilder.h>
#include <generator.h>
#include <blocktexturedef.h>
#include <windowglobals.h>
#include <game.h>

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
	
	printf ("Loading BTD 'blockdef.btd' ...\n");
	loadblockdef("blockdef.btd");
	
	set_world_name ("default");
	//delete_current_world();
	
	printf("Initializing Builder Thread ...\n");
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
	
	xg_cursor_set (true, 132);
	
	// The GameState setup on startup
	input_state  = &menu_input_state;
	render_state = &world_render_state;
	debug_overlay_state = &debug_fps_pos_state;
	overlay_state = &menu_overlay_state;
	
	while(xg_window_isopen()){
		float frameTime = xg_get_ftime();
		
		xg_window_update();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		input_state (frameTime);
		render_state();
		debug_overlay_state(frameTime);
		overlay_state ();

		xg_glx_swap();

	}
	exit_game();
	
	printf("Closing X11 Window ...\n");
	xg_window_close();
	
	printf("Terminating Generator Thread ...\n");
	terminate_generator_thread();
	printf("Terminating Builder Threads ...\n");
	terminate_builder_thread();

	return 0;
}
