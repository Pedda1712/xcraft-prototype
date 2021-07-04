#ifndef GLISTS
#define GLISTS

#include <chunklist.h>

/*
 * Contains:
 * chunk_list as defined in globallists.c:
 * [0] -> All Chunks 
 * [1] -> All Chunks whose mesh needs updating 
 * [2] -> All Chunks who need to be regenerated at a new position
 * [3] -> Mesh-Update Queue (for when the player breaks stuff)
*/
#define NUMLISTS 4
extern struct CLL chunk_list [NUMLISTS];

#endif
