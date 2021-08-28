#ifndef PNOISE
#define PNOISE

#include <stdint.h>

extern uint8_t p [512];

float noise (float x, float y, float z);
void seeded_noise_shuffle (int seed);

#endif
