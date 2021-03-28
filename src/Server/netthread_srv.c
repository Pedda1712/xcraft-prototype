#include <Server/netthread_srv.h>
#include <Server/world_srv.h>

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
struct sockaddr_in cliaddr;

uint8_t rec_buf [MAX_PACKET_SIZE];

uint64_t next_client_uid = 1;
struct netclientlist_t* client_list;

/*Threading*/
pthread_t net_thread;

void* network_thread_server (void* arg){

	memset (rec_buf, 0, MAX_PACKET_SIZE);
	int len = sizeof(cliaddr);
	for(;;){
		int n = recvfrom(socket_fd, rec_buf, MAX_PACKET_SIZE, MSG_WAITALL, &cliaddr, &len);
		
		if(n > 0){
			uint64_t rec_address = cliaddr.sin_addr.s_addr;
			switch (rec_buf[0]){
				case NP_PING:{
					char* client_ip = inet_ntoa(cliaddr.sin_addr);
					printf("Received NP_PING from: %s:%i\n", client_ip, ntohs(cliaddr.sin_port));
					
					uint8_t back_msg = NP_PONG;
					sendto(socket_fd, &back_msg, sizeof(back_msg), MSG_CONFIRM, &cliaddr, sizeof(cliaddr));
					break;}
				case NP_CHUNKREQUEST:{
					if(NCL_isClient(client_list, rec_address)){
						uint64_t client_id;
						int32_t req_pos_x;
						int32_t req_pos_y;
						memcpy(&client_id, &rec_buf[1], sizeof(uint64_t));
						memcpy(&req_pos_x, &rec_buf[9], sizeof(int32_t));
						memcpy(&req_pos_y, &rec_buf[13], sizeof(int32_t));
						printf("Client with UID %lu requested chunk data at X:%i Y:%i\n", client_id, req_pos_x, req_pos_y);
						
						struct chunk_t* req_chunk = chunk_data(req_pos_x, req_pos_y);
						
						uint8_t ret_pack [MAX_PACKET_SIZE];
						for(int32_t l = 0; l < CHUNK_SIZE_Y;++l){
							ret_pack[0] = NP_CHUNKLAYER;
							memcpy(&ret_pack[1], &req_pos_x, sizeof(int32_t));
							memcpy(&ret_pack[5], &req_pos_y, sizeof(int32_t));
							memcpy(&ret_pack[9], &l, sizeof(int32_t));
							memcpy(&ret_pack[13],&req_chunk->block_data[l * CHUNK_LAYER], CHUNK_LAYER);
							sendto(socket_fd, ret_pack, MAX_PACKET_SIZE, MSG_CONFIRM, &cliaddr, sizeof(cliaddr));
						}
					}else{
						printf("Non-Connected Client %lu sent Chunk Request, sending termination ...\n",rec_address);
						uint8_t ret_pack [1];
						ret_pack[0] = NP_TERMINATE;
						sendto(socket_fd, ret_pack, 1, MSG_CONFIRM, &cliaddr, sizeof(cliaddr));
					}
					break;}
				case NP_CONNECTIONREQUEST:{
					if(!NCL_isClient (client_list, rec_address)){
						printf("Client connected: %lu:%lu\n", rec_address, next_client_uid);
						struct netclient_t new_client = {next_client_uid, rec_address};
						NCL_addClient(client_list, new_client);
						
						uint8_t ret_pack [9];
						ret_pack[0] = NP_CONNECTIONACCEPT;
						memcpy(&ret_pack[1], &new_client.u_id, sizeof(uint64_t));
						sendto(socket_fd, ret_pack, 9, MSG_CONFIRM, &cliaddr, sizeof(cliaddr));
						next_client_uid++;
					}else{
						printf("Already connected Client tried to connect, terminating connection ... \n");
						uint8_t ret_pack [1];
						ret_pack[0] = NP_TERMINATE;
						sendto(socket_fd, ret_pack, 1, MSG_CONFIRM, &cliaddr, sizeof(cliaddr));
						NCL_remClient(client_list, rec_address);
					}
				break;}
				case NP_DISCONNECT:{
					if(NCL_isClient(client_list, rec_address)){
						uint64_t client_id;
						memcpy(&client_id, &rec_buf[1], sizeof(uint64_t));

						NCL_remClient(client_list, rec_address);
						
						printf("Client %lu:%lu disconnected from Server!\n", rec_address, client_id);
					}
				break;}
				default:{
					printf("Received Message: %i\n",rec_buf[0]);
				break;}
			}
		}
	}
	
	return 0;
}

bool initialize_server (){
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	
	if(socket_fd < 0){
		printf("Socket Creation failed!\n");
		return false;
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr,  0, sizeof(cliaddr));
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(SERVER_PORT);
	
	if( bind(socket_fd, &servaddr, sizeof(servaddr)) < 0 ){
		printf("Socket Binding failed!\n");
		return false;
	}
	
	client_list = NCL_alloc();
	client_list->cli.u_id = 0;
	client_list->cli.address = INADDR_ANY;
	
	return true;
}

void start_server_thread (){
	pthread_create(&net_thread, NULL, network_thread_server, NULL);
}
void terminate_server_thread (){
	pthread_cancel(net_thread);
	pthread_join(net_thread, NULL);
	NCL_freeList(client_list);
}

void wait_for_termination (){
	pthread_join(net_thread, NULL);
	NCL_freeList(client_list);
}
