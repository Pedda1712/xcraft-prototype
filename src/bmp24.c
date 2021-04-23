#include <bmp24.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

uint8_t* loadBMP (char* fname, uint32_t* w, uint32_t* h){
	
	uint32_t pixel_offset;
	int32_t width;
	int32_t height;
	int16_t bpp;
	
	FILE* m_file = fopen(fname, "rb");
	fseek(m_file, 10, SEEK_SET);
	fread(&pixel_offset, 4, 1, m_file);
	fseek(m_file, 18, SEEK_SET);
	fread(&width, 4, 1, m_file);
	fread(&height, 4, 1, m_file);
	fseek(m_file, 28, SEEK_SET);
	fread (&bpp, 2, 1, m_file);
	
	*w = width;
	*h = height;
	
	if(bpp != 24){
		printf("%s is not a 24bit BMP file, returning NULL, this will result in a segfault!\n",fname);
		return NULL;
	}
	
	uint32_t row_size = width * (bpp / 8);
	
	uint8_t* current_row = malloc(row_size);
	uint8_t* pixel_data = malloc(row_size * height);
	
	fseek(m_file, pixel_offset, SEEK_SET);
	for(int i = height-1;i >= 0;--i){
		fread(current_row, row_size, 1, m_file);
		memcpy(pixel_data + i*row_size, current_row, row_size);
	}
	free(current_row);
	
	fclose(m_file);
	return pixel_data;
}
