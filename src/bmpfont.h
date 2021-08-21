#ifndef BMPFONT
#define BMPFONT

#define CHARACTER_BASE_SIZE_X 0.1f
#define CHARACTER_BASE_SIZE_Y (CHARACTER_BASE_SIZE_X * ((float)width/(float)height))

#include <GL/gl.h>

struct paragraph_t {
	uint16_t width;
	uint16_t height;
	char* chars;
};

struct paragraph_t* PG_create (uint16_t width, uint16_t height);
void PG_button_fill (struct paragraph_t* pg, uint8_t b_base);
void PG_free (struct paragraph_t* pg);

GLuint loadfont (char* filename, GLint font_filter);
void drawstring (char* str, float x, float y, float scale);
void drawtextpg (struct paragraph_t* t, float x, float y, float scale);
void setfont    (GLuint font_tex);
void setupfont  ();
void revertfont ();

#endif
