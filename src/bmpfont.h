#ifndef BMPFONT
#define BMPFONT

#define CHARACTER_BASE_SIZE_X 0.1f
#define CHARACTER_BASE_SIZE_Y (CHARACTER_BASE_SIZE_X * ((float)width/(float)height))

#include <GL/gl.h>

GLuint loadfont (char* filename, GLint font_filter);
void drawstring (char* str, float x, float y, float scale);
void setfont    (GLuint font_tex);
void setupfont  ();
void revertfont ();

#endif
