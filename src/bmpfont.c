#include <bmpfont.h>

#include <string.h>
#include <stdlib.h>

#include <bmp24.h>
#include <windowglobals.h>

uint32_t font_atlas_w, font_atlas_h;
uint16_t font_pixel_w, font_pixel_h;


#define TEXELMAPX(x) ((float)(x) / (float)font_atlas_w)
#define TEXELMAPY(y) ((float)(y) / (float)font_atlas_h)

GLuint font_texture;
GLuint loadfont (char* fn, GLint font_filter){
	uint8_t* font_image_data = loadBMP (fn, &font_atlas_w, &font_atlas_h);
	
	font_pixel_w = font_atlas_w / 16;
	font_pixel_h = font_atlas_h / 16;
	
	uint8_t* font_image_rgba_data = malloc(font_atlas_w * font_atlas_h * 4);
	
	for(uint32_t x = 0; x < font_atlas_w; x++){
		for(uint32_t y = 0; y < font_atlas_h; y++){
			uint32_t rgb_index  = (x + y * font_atlas_w) * 3;
			uint32_t rgba_index = (x + y * font_atlas_w) * 4;
			
			if (font_image_data[rgb_index] == 255 && font_image_data[rgb_index + 2] == 255 && font_image_data[rgb_index + 1] == 0){
				font_image_rgba_data[rgba_index + 3] = 0;
				font_image_rgba_data[rgba_index + 0] = 0;
				font_image_rgba_data[rgba_index + 1] = 0;
				font_image_rgba_data[rgba_index + 2] = 0;
			}else{
				font_image_rgba_data[rgba_index + 3] = 255;
				font_image_rgba_data[rgba_index + 0] = font_image_data[rgb_index];
				font_image_rgba_data[rgba_index + 1] = font_image_data[rgb_index + 1];
				font_image_rgba_data[rgba_index + 2] = font_image_data[rgb_index + 2];
			}
		}
	}
	
	glGenTextures(1, &font_texture);
	glBindTexture(GL_TEXTURE_2D, font_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font_atlas_w, font_atlas_h, 0, GL_BGRA, GL_UNSIGNED_BYTE, font_image_rgba_data);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, font_filter );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, font_filter );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	free(font_image_data);
	free(font_image_rgba_data);
	
	return font_texture;
}

static void drawchar (char c, float x, float y, float scale){
	x = (x * 2) - 1.0f;
	y = ((1.0f - y) * 2) - 1.0f;
	
	uint32_t tex_coord_start_x = ((uint8_t)c * font_pixel_w) % font_atlas_w;
	uint32_t tex_coord_start_y = (((uint8_t)c * font_pixel_w) / font_atlas_w) * font_pixel_h;
	
	glBegin (GL_QUADS);
	
	glTexCoord2f (TEXELMAPX(tex_coord_start_x), TEXELMAPY(tex_coord_start_y));
	glVertex2f (x, y);
	glTexCoord2f (TEXELMAPX(tex_coord_start_x + font_pixel_w), TEXELMAPY(tex_coord_start_y));
	glVertex2f (x + CHARACTER_BASE_SIZE_X * scale, y);
	glTexCoord2f (TEXELMAPX(tex_coord_start_x + font_pixel_w), TEXELMAPY(tex_coord_start_y + font_pixel_h));
	glVertex2f (x + CHARACTER_BASE_SIZE_X * scale, y - CHARACTER_BASE_SIZE_Y * scale);
	glTexCoord2f (TEXELMAPX(tex_coord_start_x), TEXELMAPY(tex_coord_start_y + font_pixel_h));
	glVertex2f (x, y - CHARACTER_BASE_SIZE_Y * scale);
	
	glEnd();
}

void drawstring (char* str, float x, float y, float scale){
	uint32_t str_len = strlen (str);
	
	for(uint32_t i = 0; i < str_len; i++){
		drawchar(str[i], x * 0.5f + i * CHARACTER_BASE_SIZE_X * scale * 0.5f, y * 0.5f, scale);
	}
}

void drawtextpg (struct paragraph_t* t, float x, float y, float scale){
	for(uint16_t i = 0; i < t->width; ++i){
		for (uint16_t j = 0; j < t->height; ++j){
			drawchar(t->chars[j * t->width + i], x * 0.5f + i * CHARACTER_BASE_SIZE_X * scale * 0.5f, (y * 0.5f + j * CHARACTER_BASE_SIZE_Y * scale * 0.5f), scale);
		}
	}
}

void setupfont (){
	glBindTexture(GL_TEXTURE_2D, font_texture);
	
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glEnable  (GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL, 0.0f);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
}

void revertfont (){
	glDisable (GL_ALPHA_TEST);
}

void setfont(GLuint tex){
	font_texture = tex;
	glBindTexture(GL_TEXTURE_2D, font_texture);
}

struct paragraph_t* PG_create (uint16_t width, uint16_t height){
	struct paragraph_t* pg = malloc(sizeof(struct paragraph_t));
	pg->width = width;pg->height = height;
	
	pg->chars = malloc(sizeof(char) * height * width);
	memset (pg->chars, 0, height * width);
	return pg;
}

void PG_free (struct paragraph_t* pg){
	free(pg->chars);
	free(pg);
}

void PG_button_fill (struct paragraph_t* pg, uint8_t b_base){
	
	for (int x = 0; x < pg->width; x++){
		for(int y = 0; y < pg->height;y++){
			uint32_t ind = y * pg->width + x;
			
			uint8_t tile = b_base;
			if (y == 0){tile -= 16;}
			if (y == pg->height - 1){tile += 16;}
			if (x == 0){tile--;}
			if (x == pg->width - 1) {tile++;}
			
			pg->chars[ind] = tile;
			
		}
	}
	
}
