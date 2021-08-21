#include <genericlist.h>
#include <stdlib.h>

struct GLL GLL_init (){
	struct GLL crt = {0,0};
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

void GLL_rem (struct GLL* gll, void* _rem) {
	for ( struct GLL_element* e = gll->first; e != 0; e = e->next){
		if( e->next->data == _rem){
			
			struct GLL_element* tn = e->next;
			e->next = e->next->next;
			free (tn);
			
			break;
		}
	}
}

void GLL_free_help (struct GLL_element* e){
	if( e != 0){
		GLL_free_help (e->next);
		free (e);
	}
}

void GLL_free (struct GLL* gll){
	GLL_free_help (gll->first);
}

void GLL_free_rec (struct GLL* gll){
	for ( struct GLL_element* e = gll->first; e != 0; e = e->next){
		free (e->data);
	}
	GLL_free(gll);
}
