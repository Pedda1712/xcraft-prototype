#ifndef DYNAMICARRAY
#define DYNAMICARRAY

#include <stdint.h>

#define DFA_INITSIZE 50
#define DFA_GROWTHRATE 2

struct DFA {
	uint32_t size;
	uint32_t _alloc_size;
	float* data;
};

struct DFA DFA_init ();
void DFA_add   (struct DFA* arr, float e);
void DFA_rem   (struct DFA* arr, float e);
void DFA_fit   (struct DFA* arr);
void DFA_free  (struct DFA* arr);
void DFA_clear (struct DFA* arr);

#endif
