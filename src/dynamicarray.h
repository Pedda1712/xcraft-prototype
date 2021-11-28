#ifndef DYNAMICARRAY
#define DYNAMICARRAY

#include <stdint.h>

#define DFA_INITSIZE 50
#define DFA_GROWTHRATE 1.25f

struct DFA {
	uint32_t size;
	uint32_t _alloc_size;
	float* data;
};

struct DFA DFA_init ();
void DFA_add   (struct DFA* arr, float e);
void DFA_rem   (struct DFA* arr, float e); //remove an element
void DFA_fit   (struct DFA* arr); // shrink allocated size to fit perfectly
void DFA_free  (struct DFA* arr); // free the data
void DFA_clear (struct DFA* arr); // empties the array (size to 0), but doesn't deallocate or resize memory

#endif
