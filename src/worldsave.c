#include <worldsave.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <errno.h>

char w_path [MAX_WORLDNAME_LENGTH];

void set_world_name (char* w_name){
	struct stat sb;
	
	if (!(stat("save", &sb) == 0 && S_ISDIR(sb.st_mode))){
		mkdir("save", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	
	sprintf(w_path ,"save/%s", w_name);
	
	if (!(stat(w_path, &sb) == 0 && S_ISDIR(sb.st_mode))){
		mkdir(w_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
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
	
	fwrite (c->water.block_data, 1, CHUNK_MEM, file);
	
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
		
		fread (c->water.block_data, 1, CHUNK_MEM, file);
		
		fclose (file);
		return true;
	}
	return false;
}
