#include <Client/netthread_cli.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <pthread.h>

#include <netdefs.h>
#include <clientlist.h>

#include <string.h>

/*Networking*/
int32_t socket_fd;
struct sockaddr_in servaddr;
struct sockaddr_in myaddr;

uint8_t rec_buf [MAX_PACKET_SIZE];

uint64_t my_id = 0;

/*Threading*/
pthread_t net_thread;

void* network_thread_client (void* arg){
	int len = sizeof(servaddr);
	for(;;){
		int n = recvfrom(socket_fd, rec_buf, MAX_PACKET_SIZE, MSG_WAITALL, &servaddr, &len);
		
		if(n > 0){
			switch (rec_buf[0]){
				case NP_CHUNKLAYER:{
					
				break;}
				default:{
					printf("Received Message: %i\n",rec_buf[0]);
				break;}
			}
		}
	}
	
	return 0;
}

bool initialize_client (){
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	
	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	if(socket_fd < 0){
		printf("Socket Creation failed!\n");
		return false;
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&myaddr,  0, sizeof(myaddr));
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");;
	servaddr.sin_port = htons(SERVER_PORT);
	
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons(CLIENT_PORT);
	
	if( bind(socket_fd, &myaddr, sizeof(myaddr)) < 0 ){
		printf("Socket Binding failed!\n");
		return false;
	}
	
	uint8_t msgbuf [MAX_PACKET_SIZE];
	*msgbuf = NP_CONNECTIONREQUEST;
	
	printf("Sending Connection Request to Server ... \n");
	sendto(socket_fd, msgbuf, MAX_PACKET_SIZE, MSG_CONFIRM ,&servaddr, sizeof(servaddr));
	
	int addrlen = sizeof(servaddr);
	
	int n = recvfrom(socket_fd, msgbuf, MAX_PACKET_SIZE, MSG_WAITALL, &servaddr, &addrlen);
	if(n > 0){
		if(msgbuf[0] == NP_CONNECTIONACCEPT){
			printf("Server accepted connection!\n");
			memcpy(&my_id, &msgbuf[1] , sizeof(uint64_t));
		}else{
			printf("Connection failed : %i \n",msgbuf[0]);
			return false;
		}
	}else{
		printf("Connection timed out!\n");
		return false;
	}
	
	return true;
}

void start_client_thread (){
	pthread_create(&net_thread, NULL, network_thread_client, NULL);
}

void terminate_client_thread (){
	MSG_disconnect();
	
	pthread_cancel(net_thread);
	pthread_join (net_thread, NULL);
}

void wait_for_termination (){
	pthread_join (net_thread, NULL);
}

// Packet Shortcut Functions
void MSG_disconnect (){
	uint8_t disc_msg[9];
	disc_msg[0] = NP_DISCONNECT;
	memcpy(&disc_msg[1], &my_id, sizeof(uint64_t));
	sendto(socket_fd, disc_msg, 9, MSG_CONFIRM, &servaddr, sizeof(servaddr));
}

void MSG_chunkrequest (int32_t x, int32_t z){
	uint8_t chunk_msg [17];
	chunk_msg[0] = NP_CHUNKREQUEST;
	memcpy(&chunk_msg[1], &my_id, sizeof(uint64_t));
	memcpy(&chunk_msg[9], &x    , sizeof(int32_t ));
	memcpy(&chunk_msg[13],&z    , sizeof(int32_t ));
	sendto(socket_fd, chunk_msg, 17, MSG_CONFIRM, &servaddr, sizeof(servaddr));
}
