#ifndef CLICKTOOL
#define CLICKTOOL

#include <stdbool.h>

void rotate_direction_by_ssoffset (float, float, float, float, float, float*, float*, float*);

void place_clicktool (bool, float,float);
void break_clicktool (bool, float,float);

#endif
