#include <worldsave.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pnoise.h>

#include <errno.h>

#include <ftw.h>

#include <struced/octree.h>
#include <game.h>

struct GLL structure_cache [STRUCTURE_CACHE_SIZE];
char w_path [MAX_WORLDNAME_LENGTH];

void init_structure_cache (){
	
	for(int i = 0; i < STRUCTURE_CACHE_SIZE; i++){
		structure_cache[i] = GLL_init();
	}
	read_structure_file_into_gll("struc/totem.struc", &structure_cache[0]);
}

void clean_structure_cache(){
	for(int i = 0; i < STRUCTURE_CACHE_SIZE; i++){
		GLL_free_rec(&structure_cache[i]);
		GLL_destroy(&structure_cache[i]);
	}
}

void read_structure_file_into_gll (char* fname, struct GLL* gll){
	FILE* f = fopen(fname, "rb");
	
	if (f != NULL){
		int size;
		fread(&size, sizeof(int), 1, f);
		
		for(int i = 0; i < size; i++){
			struct block_t* current = malloc (sizeof(struct block_t));
			fread(current, sizeof(struct block_t), 1, f);
			GLL_add(gll, current);
		}
		
	}else{
		printf("Error opening file %s\n", fname);
	}
	
	fclose(f);
}

bool read_structure_log_into_gll (int x, int z, struct GLL* gll){
	char ff [MAX_WORLDNAME_LENGTH];
	char fl [MAX_WORLDNAME_LENGTH];
	char fname  [512];
	
	(x >= 0) ? sprintf(ff, "p%i", abs(x)) : sprintf(ff, "n%i", abs(x));
	(z >= 0) ? sprintf(fl, "p%i", abs(z)) : sprintf(fl, "n%i", abs(z));
	sprintf(fname, "%s/struclog/%s%s.log", w_path, ff, fl);
	
	FILE* f = fopen(fname, "rb");
	
	if (f != NULL){
		fseek(f, 0, SEEK_END);
		uint32_t len = ftell(f); // how much data is there? -(how much do we need to read)
		fseek(f, 0, SEEK_SET);
		for(int i = 0; i < len/sizeof(struct block_t); i++){
			struct block_t* current = malloc (sizeof(struct block_t));
			fread(current, sizeof(struct block_t), 1, f);
			GLL_add(gll, current);
		}
		
		fclose(f);
		
		// remove the file
		if(remove(fname) != 0){
			printf("Error deleting struclog file %s\n", fname);
		}
		
		return true;
	}else{
		return false;
	}
}

void save_structure_log_into_file (struct structure_log_t* log){
	char ff [MAX_WORLDNAME_LENGTH];
	char fl [MAX_WORLDNAME_LENGTH];
	char f  [512];
	
	(log->_x >= 0) ? sprintf(ff, "p%i", abs(log->_x)) : sprintf(ff, "n%i", abs(log->_x));
	(log->_z >= 0) ? sprintf(fl, "p%i", abs(log->_z)) : sprintf(fl, "n%i", abs(log->_z));
	sprintf(f, "%s/struclog/%s%s.log", w_path, ff, fl);
		
	FILE* file = fopen(f, "ab+"); // Open File for appending
	
	if(file != NULL){

		fseek(file, 0, SEEK_END); // Append new blocks
		for(struct GLL_element* e = log->blocks.first; e != NULL; e = e->next){
			struct block_t* current = e->data;
			fwrite(current, sizeof(struct block_t), 1, file);
		}
		
		fclose(file);
		
	}else{
		printf("Error opening file %s\n", f);
	}
	
}

void delete_current_world (){
	int fntor (const char* fpath, const struct stat* sb, int typeflag){
		remove (fpath);
		return 0;
	}
	
	char f [512];
	sprintf(f, "%s/struclog", w_path);
	ftw(f, &fntor, 64);
	rmdir(f);
	
	ftw(w_path, &fntor, 64);
	rmdir(w_path);
}

void set_world_name (char* w_name){
	struct stat sb;
	
	if (!(stat("save", &sb) == 0 && S_ISDIR(sb.st_mode))){
		mkdir("save", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	
	sprintf(w_path ,"save/%s", w_name);
	
	if (!(stat(w_path, &sb) == 0 && S_ISDIR(sb.st_mode))){
		mkdir(w_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		
		char structure_path [MAX_WORLDNAME_LENGTH+9];
		sprintf(structure_path, "%s/struclog", w_path);
		mkdir(structure_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		
		char seedf [100];
		sprintf(seedf, "%s/seed.dat", w_path);
		FILE* f = fopen (seedf, "wb");
		fwrite(p, 1, 512, f);
		fclose(f);
		
		dump_player_data();
		
	}else {
		char seedf [100];
		sprintf(seedf, "%s/seed.dat", w_path);
		FILE* f = fopen (seedf, "rb");
		fread(p, 1, 512, f);
		fclose(f);
		
		read_player_data();
	}
}

void dump_player_data () {
	char f [512];
	
	sprintf(f, "%s/player.bin", w_path);
	
	FILE* p_file = fopen(f, "wb");
	
	fwrite(&gst._player_x, 1, sizeof(float), p_file);
	fwrite(&gst._player_y, 1, sizeof(float), p_file);
	fwrite(&gst._player_z, 1, sizeof(float), p_file);
	
	fclose(p_file);
}

void read_player_data () {
	char f [512];
	
	sprintf(f, "%s/player.bin", w_path);
	
	FILE* p_file = fopen(f, "rb");
	
	fread(&gst._player_x, 1, sizeof(float), p_file);
	fread(&gst._player_y, 1, sizeof(float), p_file);
	fread(&gst._player_z, 1, sizeof(float), p_file);
	
	fclose(p_file);
}

void dump_chunk (struct sync_chunk_t* c){
	
	char ff [MAX_WORLDNAME_LENGTH];
	char fl [MAX_WORLDNAME_LENGTH];
	char f  [512];
	
	(c->_x >= 0) ? sprintf(ff, "p%i", abs(c->_x)) : sprintf(ff, "n%i", abs(c->_x));
	(c->_z >= 0) ? sprintf(fl, "p%i", abs(c->_z)) : sprintf(fl, "n%i", abs(c->_z));
	sprintf(f, "%s/%s%s.dat", w_path, ff, fl);
	
	FILE* file = fopen(f, "wb");
		
	if (file == 0){
		perror("Error: ");
	}
	
	fwrite (c->data_unique.block_data, 1, CHUNK_MEM, file);
	
	fclose (file);
	
}

bool read_chunk (struct sync_chunk_t* c){
	char ff [MAX_WORLDNAME_LENGTH];
	char fl [MAX_WORLDNAME_LENGTH];
	char f  [512];
	
	(c->_x >= 0) ? sprintf(ff, "p%i", abs(c->_x)) : sprintf(ff, "n%i", abs(c->_x));
	(c->_z >= 0) ? sprintf(fl, "p%i", abs(c->_z)) : sprintf(fl, "n%i", abs(c->_z));
	sprintf(f, "%s/%s%s.dat", w_path, ff, fl);
	
	if (access (f, F_OK) == 0){
		FILE* file = fopen(f, "rb");
		
		fread (c->data_unique.block_data, 1, CHUNK_MEM, file);
		
		fclose (file);
		return true;
	}
	return false;
}
