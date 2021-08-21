#ifndef PLAYER
#define PLAYER

#include <stdbool.h>

/*
 get_next_block_in_direction:
 Overly complicated function (because integer rounding is a bitch) for determining the next block in a given direction
 sx,sy,sz -> these values are the actual block position (when cast to an int)
 ax,ay,az -> these values are the intersection point (used for chaining get_next_block_in_direction calls)
 */

void get_next_block_in_direction (float posx, float posy, float posz, float dirx, float diry, float dirz, float* sx, float* sy, float* sz, float* actx, float* acty, float* actz);

bool break_block_action (int chunk_x, int chunk_z, int ccx, int ccy, int ccz, float ax, float ay, float az);
bool place_block_action (int chunk_x, int chunk_z, int ccx, int ccy, int ccz, float ax, float ay, float az);

#define RAY_ACTION_LENGTH 10
void block_ray_actor (bool (*action_func)(int, int, int, int, int, float, float, float));


#endif
