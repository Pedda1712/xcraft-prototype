#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>

#include <netdefs.h>
#include <worlddefs.h>

#include <Client/netthread_cli.h>
#include <Client/worldthread_cli.h>

int main () {
	
	if(!initialize_client()){
		exit(-1);
	}
	
	start_client_thread ();
	
	//Send a Chunk Request from the Main Thread
	for(int x = 0; x < 16;++x){
		for(int y = 0; y < 16;++y){
			MSG_chunkrequest(x,y);
		}
	}
	
	sleep(10);
	
	terminate_client_thread ();

	return 0;
}
