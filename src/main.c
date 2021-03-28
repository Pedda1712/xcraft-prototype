#include "xgame.h"
#include <stdio.h>
#include <math.h>

const uint16_t width =  800;
const uint16_t height = 600;

int main () {

	xg_init();
	xg_window(width, height, "Window");
	xg_window_set_not_resizable();
	xg_init_glx();

	xg_window_show();

	glClearColor(0.0f,0.0f,0.0f,1.0f);

	float rotation = 0.0f;

	int16_t offset_x, offset_y;

	float _player_x,_player_y,_player_z;
	_player_x = 0.0f;
	_player_y = 0.0f;
	_player_z = 0.0f;
	float _dir_x,_dir_y,_dir_z;
	_dir_x = 0.0f;
	_dir_y = 0.0f;
	_dir_z = 0.0f;
	float angle_x, angle_y;
	angle_x = 3.14159f/2;
	angle_y = 0.0f;

	xg_cursor_visible(false);

	GLfloat d_verts [] = {
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
	 	 0.0f,  0.5f, 0.0f	 
	};

	GLubyte d_colors [] = {
		255,0,0,
		0,0,255,
		0,255,0
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glMatrixMode(GL_PROJECTION);  
	glLoadIdentity();
	gluPerspective(40.0,(GLdouble)width/(GLdouble)height,0.1,20.0);

	while(xg_window_isopen()){

		float frameTime = xg_get_ftime();
		rotation += frameTime * 20.0f;

		if(rotation > 360.0f){
			rotation = 0.0f;
		}

		xg_window_update();

		glClear(GL_COLOR_BUFFER_BIT);

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
			_player_x += _dir_x * frameTime;
			_player_y += _dir_y * frameTime;
			_player_z += _dir_z * frameTime;
		}
		if(xg_keyboard_ascii(115)){
			_player_x -= _dir_x * frameTime;
			_player_y -= _dir_y * frameTime;
			_player_z -= _dir_z * frameTime;
		}

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(_player_x,_player_y,_player_z,_player_x + _dir_x,_player_y + _dir_y, _player_z + _dir_z,0,1,0);
		glTranslatef(0.0,0.0,4.0);
		glRotatef(rotation, 0.0f, 1.0f, 0.0f);

		glVertexPointer(3, GL_FLOAT, 0, d_verts);
		glColorPointer(3, GL_UNSIGNED_BYTE, 0, d_colors);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		xg_glx_swap();

	}
	xg_window_close();
	return 0;
}
