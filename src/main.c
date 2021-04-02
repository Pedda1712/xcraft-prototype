#include "xgame.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <worlddefs.h>
#include <chunklist.h>
#include <chunkbuilder.h>
#include <dynamicarray.h>
#include <generator.h>

#include <bmp24.h>

#include <pnoise.h>

const uint16_t width = 1600;
const uint16_t height = 900;

int main () {

	//Setting up Chunk-Building Thread
	
	printf("Initializing Chunk Thread ...\n");
	if(!initialize_chunk_thread()){
		printf("Failed to initialize Chunk Thread!\n");
		exit(-1);
	}
	
	printf("Initializing Generator Thread ...\n");
	if(!initialize_generator_thread()){
		printf("Failed to initialize Generator Thread!\n");
		exit(-1);
	}
	
	struct CLL* chunk_list = get_chunk_list(0); // Get a list with all of the chunks

	struct chunkspace_position pos = {0,0};
	trigger_generator_update(&pos);

	xg_init();
	xg_window(width, height, "Window");
	xg_window_set_not_resizable();
	xg_init_glx();

	xg_window_show();

	int32_t offset_x, offset_y;

	int32_t p_chunk_x = 0;
	int32_t p_chunk_z = 0;
	
	float p_speed = 10.0f;
	float _player_x,_player_y,_player_z;
	_player_x = 0.0f;
	_player_y = 64.0f;
	_player_z = 0.0f;
	float _dir_x,_dir_y,_dir_z;
	_dir_x = 0.0f;
	_dir_y = 0.0f;
	_dir_z = 0.0f;
	float angle_x, angle_y;
	angle_x = 3.14159f/2;
	angle_y = 0.0f;

	xg_cursor_visible(false);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	
	glMatrixMode(GL_PROJECTION);  
	glLoadIdentity();
	gluPerspective(70.0,(GLdouble)width/(GLdouble)height,0.1,500.0);
	
	//Loading a Texture
	uint32_t iw, ih;
	uint8_t* image = loadBMP("atlas.bmp",&iw,&ih);
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, iw, ih, 0, GL_BGR, GL_UNSIGNED_BYTE, image);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	free (image);

	glClearColor(0.0f,0.5f,1.0f,1.0f);
		
	while(xg_window_isopen()){
		float frameTime = xg_get_ftime();
		
		xg_window_update();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if(xg_keyboard_ascii(27)) {
			xg_window_stop ();
		}

		xg_mouse_position(&offset_x, &offset_y);
		offset_x -= width/2;
		offset_y -= height/2;
		angle_x += offset_x * 0.001f;
		angle_y += offset_y * -0.001f;
		if(angle_y > 3.14159f/2.01f) angle_y = 3.14159f/2.01f;
		if(angle_y < -3.14159f/2.01f) angle_y = -3.14159f/2.01f;
		xg_set_mouse_position( width/2, height/2 );

		_dir_x = cos(angle_y) * cos(angle_x);
		_dir_y = sin(angle_y);
		_dir_z = cos(angle_y) * sin(angle_x);

		if(xg_keyboard_ascii(119)){
			_player_x += _dir_x * frameTime * p_speed;
			_player_y += _dir_y * frameTime * p_speed;
			_player_z += _dir_z * frameTime * p_speed;
		}
		if(xg_keyboard_ascii(115)){
			_player_x -= _dir_x * frameTime * p_speed;
			_player_y -= _dir_y * frameTime * p_speed;
			_player_z -= _dir_z * frameTime * p_speed;
		}
		
		//Check if Player crossed a chunk border
		
		float _add_x = (_player_x > 0) ? CHUNK_SIZE_X / 2.0f : -CHUNK_SIZE_X / 2.0f;
		float _add_z = (_player_z > 0) ? CHUNK_SIZE_Z / 2.0f : -CHUNK_SIZE_Z / 2.0f;
		
		int32_t n_p_chunk_x = (_player_x + _add_x) / CHUNK_SIZE_X;
		int32_t n_p_chunk_z = (_player_z + _add_z) / CHUNK_SIZE_Z;
		
		if(n_p_chunk_x != p_chunk_x || n_p_chunk_z != p_chunk_z){
			p_chunk_x = n_p_chunk_x;
			p_chunk_z = n_p_chunk_z;
			pos._x = p_chunk_x;
			pos._z = p_chunk_z;
			trigger_generator_update(&pos);
		}
		
		struct CLL_element* p;
		for(p = chunk_list->first; p!= NULL; p = p->nxt){
			if(!chunk_updating(p->data)){
				if(p->data->render){
					struct sync_chunk_t* ch = p->data;
					
					glMatrixMode(GL_MODELVIEW);
					glLoadIdentity();
					gluLookAt(_player_x,_player_y,_player_z,_player_x + _dir_x,_player_y + _dir_y, _player_z + _dir_z,0,1,0);

					glVertexPointer(3, GL_FLOAT, 0, ch->vertex_array.data);
					glTexCoordPointer (2, GL_FLOAT, 0, ch->texcrd_array.data);
					glColorPointer (3, GL_FLOAT, 0, ch->lightl_array.data);
					glDrawArrays(GL_QUADS, 0, ch->vertex_array.size/3 );
					
				}
				chunk_data_unsync(p->data);
			}
		}

		xg_glx_swap();

	}
	xg_window_close();
	
	printf("Terminating Generator Thread ...\n");
	terminate_generator_thread();
	printf("Terminating Chunk Thread ...\n");
	terminate_chunk_thread();

	return 0;
}
