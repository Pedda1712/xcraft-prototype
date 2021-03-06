#include <ui.h>

#include <stdlib.h>
#include <string.h>

struct button_t* ui_create_button (int width, int height, float x, float y, float scale, uint8_t BACKGROUND, uint8_t BACKGROUND_HIGHLIGHTED, char* b_str, void (*h)(struct button_t*), void (*c)(struct button_t*)){
	struct button_t* crt = malloc ( sizeof (struct button_t) );
	crt->_x = x; crt->_y = y; crt->highlighted_func = h; crt->clicked_func = c;
	crt->_scale = scale;
	
	crt->backg = PG_create(width, height); PG_button_fill (crt->backg, BACKGROUND);
	crt->backh = PG_create(width, height); PG_button_fill (crt->backh, BACKGROUND_HIGHLIGHTED);
	crt->forg  = PG_create(width, height);
	
	//strcpy ( crt->forg->chars + ((height / 2) * width) + ( (width/2) - strlen(b_str) / 2), b_str);
	strcpy ( crt->forg->chars + 1, b_str);
	
	return crt;
}

struct button_t* ui_create_button_fit (float x, float y, float scale, uint8_t BACKGROUND, uint8_t BACKGROUND_HIGHLIGHTED, char* b_str, void (*h)(struct button_t*), void (*c)(struct button_t*)){
	return ui_create_button ( strlen (b_str)+2, 2, x, y, scale, BACKGROUND, BACKGROUND_HIGHLIGHTED, b_str, h, c);
}

void ui_destroy_button (struct button_t* b){
	PG_free (b->backg);
	PG_free (b->backh);
	PG_free (b->forg);
	free (b);
}

void GLL_free_rec_btn (struct GLL* gll){
	for ( struct GLL_element* e = gll->first; e != 0; e = e->next){
		ui_destroy_button((struct button_t*)e->data);
	}
	GLL_free(gll);
}
