#ifndef BMP24
#define BMP24

#include <stdint.h>

// loads a Bitmap that is stored uncompressed with 24 Bits Per Pixel
uint8_t* loadBMP (char* fname, uint32_t* width, uint32_t* height);

#endif
