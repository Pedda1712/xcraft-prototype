#include <genericlist.h>
#include <stdlib.h>

struct GLL GLL_init (){
	struct GLL crt = {0,0};
	pthread_mutex_init(&crt.mutex, NULL);
	return crt;
}

void GLL_add (struct GLL* gll, void* _new){
	
	if ( gll->first == 0 ){
		gll->first = malloc ( sizeof (struct GLL_element) );
		gll->first->data = _new;
		gll->first->next = 0;
		return;
	}
	
	for ( struct GLL_element* e = gll->first; e != 0; e = e->next){
		if( e->next == 0){
			
			e->next = malloc ( sizeof ( struct GLL_element) );
			e->next->data = _new;
			e->next->next = 0;
			break;
		}
	}
	
	gll->size++;
}

void GLL_push(struct GLL* gll, void* _new){
	struct GLL_element* new = malloc (sizeof (struct GLL_element));
	new->data = _new;
	new->next = gll->first;
	gll->first = new;
}

void* GLL_pop (struct GLL* gll){
	
	void* data_ptr = gll->first->data;
	struct GLL_element* new_first = gll->first->next;
	free(gll->first);
	gll->first = new_first;
	return data_ptr;
	
}

void GLL_rem (struct GLL* gll, void* _rem) {

	if(gll->first != NULL){
		if(gll->first->data == _rem){
			struct GLL_element* tn = gll->first;
			gll->first = gll->first->next;
			free (tn);
			return;
		}
	}
	
	for ( struct GLL_element* e = gll->first; e != NULL; e = e->next){
		if( e->next != NULL)
		if( e->next->data == _rem){
			struct GLL_element* tn = e->next;
			e->next = e->next->next;
			free (tn);
			return;
		}
	}
	gll->size--;
}

void GLL_free_help (struct GLL_element* e){
	if( e != 0){
		GLL_free_help (e->next);
		free (e);
	}
}

void GLL_free (struct GLL* gll){
	GLL_free_help (gll->first);
	gll->first = 0;
	gll->size = 0;
}

void GLL_free_rec (struct GLL* gll){
	for ( struct GLL_element* e = gll->first; e != 0; e = e->next){
		free (e->data);
	}
	GLL_free(gll);
}

void GLL_destroy (struct GLL* gll){
	pthread_mutex_destroy(&gll->mutex);
}

void GLL_lock ( struct GLL* gll ){
	pthread_mutex_lock (&gll->mutex);
}

void GLL_unlock ( struct GLL* gll ){
	pthread_mutex_unlock (&gll->mutex);
}

void GLL_operation (struct GLL* gll, void (*operator)(struct GLL_element*)){
	
	for(struct GLL_element* e = gll->first; e != NULL; e = e->next){
		(*operator)(e);
	}
	
}
