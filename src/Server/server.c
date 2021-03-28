#include <stdio.h>
#include <stdlib.h>

#include <Server/netthread_srv.h>
#include <Server/world_srv.h>

int main () {
	
	initialize_world();
	
	if(!initialize_server()){
		printf("Server initialization failed!\n");
		exit(-1);
	}
	
	start_server_thread();

	wait_for_termination();
	
	return 0;
}
