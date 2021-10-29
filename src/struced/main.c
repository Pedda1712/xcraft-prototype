#include <GL/glew.h>
#include <xcraft_window_module.h>

#include <bmp24.h>
#include <bmpfont.h>

#include <windowglobals.h>

#include <struced/appstate.h>

uint16_t width = 1280;
uint16_t height = 960;

int main () {
	
	xg_init();
	xg_window(width, height, "XCraft Structure Editor");
	xg_window_set_not_resizable ();
	xg_init_glx();
	
	xg_window_show();
	
	glewInit();
	
	xg_cursor_set (true, 132);
	
	glClearColor(0.2f,0.2f,0.2f,1.0f);
	
	glEnable(GL_TEXTURE_2D);
	
	init_app_state_system ();
	
	GLL_add(&app_state_stack, &app_editor_draw_state);
	GLL_add(&app_state_stack, &app_overlay_draw_state);
	GLL_add(&app_state_stack, &app_input_state);

	float fTime = 1.0f;
	
	while (xg_window_isopen()){
		
		xg_window_update();
		
		fTime = xg_get_ftime();
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		for(struct GLL_element* e = app_state_stack.first; e != NULL; e = e->next){
			void (*f)(float) = e->data;
			(*f)(fTime);
		}
		for(struct GLL_element* e = app_state_modifier_stack.first; e != NULL; e = e->next){
			void (*f)() = e->data;
			(*f)();
		}
		GLL_free(&app_state_modifier_stack);
		
		
		xg_glx_swap();
		
	}
	
	exit_app_state_system ();
	
	xg_window_close();
	
	return 0;
}
