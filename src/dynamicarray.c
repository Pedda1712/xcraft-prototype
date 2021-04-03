#include <dynamicarray.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct DFA DFA_init (){
	struct DFA t;
	t.size = 0;
	t._alloc_size = DFA_INITSIZE;
	t.data = malloc(sizeof(float) * DFA_INITSIZE);
	return t;
}

void DFA_free (struct DFA* arr){
	free(arr->data);
}

void DFA_clear (struct DFA* arr){
	arr->size = 0;
}

void DFA_add   (struct DFA* arr, float e){
	if(arr->size < arr->_alloc_size){
		arr->data[arr->size] = e;
		arr->size++;
	}else{
		arr->data = realloc(arr->data, sizeof(float) * arr->_alloc_size * DFA_GROWTHRATE);
		arr->_alloc_size = arr->_alloc_size * DFA_GROWTHRATE;
		arr->data[arr->size] = e;
		arr->size++;
	}

}

void DFA_rem   (struct DFA* arr, float e){
	for(uint32_t i = 0; i < arr->size; ++i){
		if(arr->data[i] == e){
			memmove(&arr->data[i], &arr->data[i+1], sizeof(float) * (arr->size - (i + 1)));
			arr->size--;
			return;
		}
	}
}

void DFA_fit   (struct DFA* arr){
	if(arr->_alloc_size > arr->size){
		arr->data = realloc (arr->data, arr->size * sizeof(float));
		arr->_alloc_size = arr->size;
	}
}
