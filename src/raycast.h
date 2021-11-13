#ifndef RAYCAST
#define RAYCAST

#include <stdbool.h>

/*
 get_next_block_in_direction:
 Overly complicated function (because integer rounding is a bitch) for determining the next block in a given direction
 sx,sy,sz -> these values are the actual block position (when cast to an int)
 ax,ay,az -> these values are the intersection point (used for chaining get_next_block_in_direction calls)
 */

void get_next_block_in_direction (float posx, float posy, float posz, float dirx, float diry, float dirz, float* sx, float* sy, float* sz, float* actx, float* acty, float* actz);

void block_ray_actor_chunkfree (int bmax, float ax, float ay, float az, float dx, float dy, float dz,  bool (*action_func)(int, int, int, float, float ,float));

#endif
