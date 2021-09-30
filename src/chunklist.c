#include <chunklist.h>
#include <genericlist.h>
#include <stdlib.h>

struct CLL CLL_init (){
	struct CLL t;
	t.size = 0;
	t.first = NULL;
	pthread_mutex_init(&t.mutex, NULL);
	return t;
}

void lock_list (struct CLL* list){
	pthread_mutex_lock(&list->mutex);
}

void unlock_list (struct CLL* list){
	pthread_mutex_unlock(&list->mutex);
}

void CLL_copyList (struct CLL* src, struct CLL* dest){
	struct CLL_element* p;
	for(p = src->first; p != NULL; p = p->nxt){
		CLL_add(dest, p->data);
	}
}

void CLL_add  (struct CLL* list ,struct sync_chunk_t* data){
	if(list->size == 0){
		list->first = malloc(sizeof(struct CLL_element));
		list->first->nxt = NULL;
		list->first->data = data;
		list->size++;
		return;
	}
	struct CLL_element* p;
	for( p = list->first; p != NULL; p = p->nxt){
		
		if(p->data == data) { // If duplicate is found, cancel the operation
			break;
		}
		
		if(p->nxt == NULL){
			p->nxt = malloc(sizeof(struct CLL_element));
			p->nxt->nxt = NULL;
			p->nxt->data = data;
			list->size++;
			return;
		}
	}
	
}

void CLL_freeList_rec (struct CLL_element* node){
	if(node == NULL) {return;}
	CLL_freeList_rec (node->nxt);
	free (node);
}

void CLL_freeList (struct CLL* list){
	CLL_freeList_rec(list->first);
	list->first = NULL;
	list->size = 0;
}

void CLL_freeListAndData (struct CLL* list){
	struct CLL_element* p;
	for(p = list->first; p != NULL; p = p->nxt){

		for(int i = 0; i < MESH_LEVELS; ++i){
			DFA_free(&p->data->mesh_buffer[i]);
		}

		pthread_mutex_destroy(&p->data->c_mutex);
		free(p->data);
	}
	CLL_freeList(list);
	list->first = NULL;
}

void CLL_destroyList(struct CLL* list){
	pthread_mutex_destroy(&list->mutex);
}

struct sync_chunk_t* CLL_getDataAt (struct CLL* list, int32_t x, int32_t z){
	struct CLL_element* p;
	for(p = list->first; p != NULL; p = p->nxt){
		if(p->data->_x == x && p->data->_z == z){
			return p->data;
		}
	}
	return NULL;
}
