#include <struced/octree.h>
#include <blocktexturedef.h>

#include <GL/gl.h>

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

static inline int comparator (struct block_t* p1, struct block_t* p2){
	bool cond[3] = { (p1->_x > p2->_x), (p1->_y > p2->_y), (p1->_z > p2->_z) };
	int res = 0;
	for(int i = 0; i < 3; i++){res += cond[i] * pow(2, i);}
	return res;
}

static inline bool equal (struct block_t* p1, struct block_t* p2){
	return ( (p1->_x == p2->_x) && (p1->_y == p2->_y) && (p1->_z == p2->_z));
}

struct OCT_node_t* OCT_alloc  (struct block_t n_data){
	struct OCT_node_t* new = calloc(1, sizeof(struct OCT_node_t));
	new->data = malloc (sizeof(struct block_t));
	*new->data = n_data;
	return new;
}

bool OCT_check (struct OCT_node_t* start, struct block_t data){
	
	if(start == NULL) return false;
	
	struct OCT_node_t* current = start;
	while (1){
		int comparator_result = comparator(current->data, &data);

		if( equal(current->data, &data) ) return true; // no dupes

		if (current->children[comparator_result] == NULL){
			return false;
		}else {
			current = current->children[comparator_result];
		}

	}

}

void OCT_insert (struct OCT_node_t* start, struct block_t data){

	struct OCT_node_t* current = start;
	while (1){
		int comparator_result = comparator(current->data, &data);

		if( equal(current->data, &data) ) {return;} // no dupes

		if (current->children[comparator_result] == NULL){
			current->children[comparator_result] = OCT_alloc(data);
			return;
		}else {
			current = current->children[comparator_result];
		}

	}

}


struct OCT_node_t* OCT_remove (struct OCT_node_t* start, struct block_t data){

	if(start == NULL) return NULL; 
	if(equal(&data, start->data)){ // start node itself needs to get removed

		int nind = 0; // index of the node that will be chosen as the new top
		void reinsert_operation (struct OCT_node_t* node){
			OCT_insert (start->children[nind], *node->data);
		}
		
		for(int i = 0; i < 8; i++){ //reinsert all sub nodes into the main tree
			if (start->children[i] != NULL){ // choose the first child
				nind = i;
				for(int i = 0; i < 8; i++){ // transfer all remaining children to the chosen one
					if (i != nind){

						OCT_operation (start->children[i], &reinsert_operation);
						OCT_operation (start->children[i], &OCT_operator_free);
					}
				}
				
				struct OCT_node_t* return_save = start->children[nind];
				OCT_operator_free(start);
				return return_save; // return the new head
				
			}
			
		}

		OCT_operator_free(start); // (if start is only node in tree)
		return NULL; 
		
	}

	struct OCT_node_t* current = start;
	while (1){
		int comparator_result = comparator(current->data, &data);


		if (current->children[comparator_result] == NULL){
			return start;
		}else {
			if(equal(current->children[comparator_result]->data, &data)){

				struct OCT_node_t* to_be_removed = current->children[comparator_result];

				current->children[comparator_result] = NULL; // disconnect the subtree (to_be_removed as start)

				void reinsert_operation (struct OCT_node_t* node){
					OCT_insert (start, *node->data);
				}

				for(int i = 0; i < 8; i++){ //reinsert all sub nodes into the main tree
					OCT_operation (to_be_removed->children[i], &reinsert_operation);
				}

				OCT_operation (to_be_removed, &OCT_operator_free);

				return start;

			}else{
				current = current->children[comparator_result];
			}
		}

	}
	return start;

}

void OCT_operation (struct OCT_node_t* current, void (*operator)(struct OCT_node_t*)){

	if (current == NULL) return;

	for(int i = 0; i < 8; i++){
		if(current->children[i] != NULL)
			OCT_operation(current->children[i] , operator);
	}
	(*operator)(current);
}

void OCT_operator_free (struct OCT_node_t* node){
	free (node->data);
	free (node);
}

int OCT_count_levels (struct OCT_node_t* node){

	if (node == NULL) return 0;

	int maxlower = 0;
	for(int i = 0; i < 8; i++){
		int temp = OCT_count_levels(node->children[i]);
		if( temp > maxlower )
			maxlower = temp;
	}

	return 1 + maxlower;

}

void OCT_operator_gl_draw (struct OCT_node_t* n){

			struct blocktexdef_t tex = btd_map[BLOCK_ID(n->data->_type)];

			float get_texX (uint8_t ind) {return ((ind * 16) % 256) / 256.0f;}
			float get_texY (uint8_t ind) {return (ind / 16) * (1.f/16.0f);}

			float ctx, cty;


			ctx = get_texX (tex.index[0]); cty = get_texY (tex.index[0]);
			
			if(!IS_X(n->data->_type)){
				glBegin (GL_QUADS);
				glTexCoord2f (ctx, cty);
				glVertex3f(0.0f + n->data->_x, 0.0f + n->data->_y, 0.0f + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty);
				glVertex3f(1.0f + n->data->_x, 0.0f + n->data->_y, 0.0f + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
				glVertex3f(1.0f + n->data->_x, 0.0f + n->data->_y, 1.0f + n->data->_z);
				glTexCoord2f (ctx, cty + 1.f/16.f);
				glVertex3f(0.0f + n->data->_x, 0.0f + n->data->_y, 1.0f + n->data->_z);

				ctx = get_texX (tex.index[1]); cty = get_texY (tex.index[1]);
				glTexCoord2f (ctx, cty);
				glVertex3f(0.0f + n->data->_x, 1.0f + n->data->_y, 0.0f + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty);
				glVertex3f(1.0f + n->data->_x, 1.0f + n->data->_y, 0.0f + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
				glVertex3f(1.0f + n->data->_x, 1.0f + n->data->_y, 1.0f + n->data->_z);
				glTexCoord2f (ctx, cty + 1.f/16.f);
				glVertex3f(0.0f + n->data->_x, 1.0f + n->data->_y, 1.0f + n->data->_z);

				ctx = get_texX (tex.index[2]); cty = get_texY (tex.index[2]);
				glTexCoord2f (ctx, cty + 1.f/16.f);
				glVertex3f(0.0f + n->data->_x,0.0f  + n->data->_y,0.0f  + n->data->_z);
				glTexCoord2f (ctx, cty);
				glVertex3f(0.0f + n->data->_x,1.0f  + n->data->_y,0.0f  + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty);
				glVertex3f(0.0f + n->data->_x,1.0f  + n->data->_y,1.0f  + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
				glVertex3f(0.0f + n->data->_x,0.0f  + n->data->_y,1.0f  + n->data->_z);

				ctx = get_texX (tex.index[3]); cty = get_texY (tex.index[3]);
				glTexCoord2f (ctx, cty + 1.f/16.f);
				glVertex3f(1.0f + n->data->_x,0.0f  + n->data->_y,0.0f  + n->data->_z);
				glTexCoord2f (ctx, cty);
				glVertex3f(1.0f + n->data->_x,1.0f  + n->data->_y,0.0f  + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty);
				glVertex3f(1.0f + n->data->_x,1.0f  + n->data->_y,1.0f  + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
				glVertex3f(1.0f + n->data->_x,0.0f  + n->data->_y,1.0f  + n->data->_z);

				ctx = get_texX (tex.index[4]); cty = get_texY (tex.index[4]);
				glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
				glVertex3f(0.0f + n->data->_x,0.0f  + n->data->_y,0.0f  + n->data->_z);
				glTexCoord2f (ctx, cty + 1.f/16.f);
				glVertex3f(1.0f + n->data->_x,0.0f  + n->data->_y,0.0f  + n->data->_z);
				glTexCoord2f (ctx, cty);
				glVertex3f(1.0f + n->data->_x,1.0f  + n->data->_y,0.0f  + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty);
				glVertex3f(0.0f + n->data->_x,1.0f  + n->data->_y,0.0f  + n->data->_z);

				ctx = get_texX (tex.index[5]); cty = get_texY (tex.index[5]);
				glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
				glVertex3f(0.0f + n->data->_x,0.0f  + n->data->_y,1.0f  + n->data->_z);
				glTexCoord2f (ctx, cty + 1.f/16.f);
				glVertex3f(1.0f + n->data->_x,0.0f  + n->data->_y,1.0f  + n->data->_z);
				glTexCoord2f (ctx, cty);
				glVertex3f(1.0f + n->data->_x,1.0f  + n->data->_y,1.0f  + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty);
				glVertex3f(0.0f + n->data->_x,1.0f  + n->data->_y,1.0f  + n->data->_z);
				glEnd();
			}else{
				
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_EQUAL, 1);
				
				glBegin (GL_QUADS);
				glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
				glVertex3f(0.0f + n->data->_x, 0.0f + n->data->_y, 0.0f + n->data->_z);
				glTexCoord2f (ctx, cty + 1.f/16.f);
				glVertex3f(1.0f + n->data->_x, 0.0f + n->data->_y, 1.0f + n->data->_z);
				glTexCoord2f (ctx, cty);
				glVertex3f(1.0f + n->data->_x, 1.0f + n->data->_y, 1.0f + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty);
				glVertex3f(0.0f + n->data->_x, 1.0f + n->data->_y, 0.0f + n->data->_z);

				ctx = get_texX (tex.index[1]); cty = get_texY (tex.index[1]);
				glTexCoord2f (ctx, cty);
				glVertex3f(1.0f + n->data->_x, 1.0f + n->data->_y, 0.0f + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty);
				glVertex3f(0.0f + n->data->_x, 1.0f + n->data->_y, 1.0f + n->data->_z);
				glTexCoord2f (ctx + 1.f/16.f, cty + 1.f/16.f);
				glVertex3f(0.0f + n->data->_x, 0.0f + n->data->_y, 1.0f + n->data->_z);
				glTexCoord2f (ctx, cty + 1.f/16.f);
				glVertex3f(1.0f + n->data->_x, 0.0f + n->data->_y, 0.0f + n->data->_z);
				glEnd();
				
				glDisable(GL_ALPHA_TEST);
			}

			

		}
