#include <clientlist.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct netclientlist_t* NCL_alloc (){
	struct netclientlist_t* new = malloc(sizeof(struct netclientlist_t));
	memset(&new->cli, 0, sizeof(struct netclient_t));
	new->nxt_cli = NULL;
	return new;
}

void NCL_addClient (struct netclientlist_t* node ,struct netclient_t cli){
	
	if (node->nxt_cli == NULL){
		node->nxt_cli = NCL_alloc();
		node->nxt_cli->cli = cli;
	}else{
		NCL_addClient(node->nxt_cli, cli);
	}
	return;
}

struct netclientlist_t* NCL_remClient (struct netclientlist_t* node ,uint64_t address){
	if(node == NULL){
		printf("Address %lu not part of client list!\n",address);
		return node;
	}else{
		if((node->cli.address == address)){
			struct netclientlist_t* ret = node->nxt_cli;
			free(node);
			return ret;
		}else{
			node->nxt_cli = NCL_remClient (node->nxt_cli, address);
			return node;
		}
	}
	
}

void NCL_freeList  (struct netclientlist_t* node){
	if (node == NULL){
		return;
	}else{
		NCL_freeList(node->nxt_cli);
		free(node);
		return;
	}
}

bool NCL_isClient (struct netclientlist_t* node, uint64_t address){
	if (node == NULL){
		return false;
	}else{
		if((node->cli.address == address) ){
			return true;
		}else{
			return NCL_isClient(node->nxt_cli, address);
		}
	}
}
