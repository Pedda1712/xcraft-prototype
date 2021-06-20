#ifndef GLISTS
#define GLISTS

#include <chunklist.h>

/*
 * Contains:
 * chunk_list as defined in globallists.c:
 * [0] -> All Chunks 
 * [1] -> All Chunks whose mesh needs updating 
 * [2] -> All Chunks who need to be regenerated at a new position
*/

extern struct CLL chunk_list [3];

#endif
