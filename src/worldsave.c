#include <worldsave.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pnoise.h>

#include <errno.h>

#include <ftw.h>

#include <game.h>

char w_path [MAX_WORLDNAME_LENGTH];

void delete_current_world (){
	int fntor (const char* fpath, const struct stat* sb, int typeflag){
		remove (fpath);
		return 0;
	}
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
