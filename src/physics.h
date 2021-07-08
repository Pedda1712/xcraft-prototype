#ifndef PHYSICS
#define PHYSICS

#include <stdint.h>

struct pbox_t {
	float _x, _y, _z; // Coordinates
	float _w, _h, _l; // Width, Height, Length
};

void get_world_section_around (int x, int y, int z, uint8_t* sec, int* posx, int* posy, int* posz, int* ox, int* oy, int* oz);

// vel: Axis-Distance moved

/*
	Note: These functions currently ONLY work if the moving boxes width and length are SMALLER than the static ones and if the moving boxes height is BIGGER than the height of the static box
 */

float x_swept_collision ( struct pbox_t mov, struct pbox_t sta, float velx);
float z_swept_collision ( struct pbox_t mov, struct pbox_t sta, float velz);
float y_swept_collision ( struct pbox_t mov, struct pbox_t sta, float vely);

#endif
