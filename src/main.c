#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include <GL/glew.h>

#include <xcraft_window_module.h>
#include <chunkbuilder.h>
#include <generator.h>
#include <blocktexturedef.h>
#include <windowglobals.h>
#include <game.h>
#include <genericlist.h>


#include <shader.h>

/*
	Minecraft Clone in C, using:
		- Xlib for Windowing
		- OpenGL 2.0 for Graphics
 */

uint16_t width = 1600;
uint16_t height = 900;

int main () {

	printf("Initializing X11 Window ... \n");
	xg_init();
	xg_window(width, height, "XCraft");
	xg_window_set_not_resizable();
	xg_init_glx();
	
	glewInit();
	
	xg_window_show();
	
	printf ("Loading BTD 'blockdef.btd' ...\n");
	loadblockdef("blockdef.btd");
	
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
	
	while(xg_window_isopen()){
		float frameTime = xg_get_ftime();
		
		xg_window_update();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		for (struct GLL_element* e = state_stack.first; e != NULL; e = e->next){
			void (*f) (float) = e->data;
			(*f)(frameTime);
		}
		for (struct GLL_element* e = state_update_stack.first; e != NULL; e = e->next){
			void (*f) (void) = e->data;
			(*f)();
		}
		GLL_free(&state_update_stack);

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
