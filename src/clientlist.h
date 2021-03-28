#ifndef CLIENTLIST
#define CLIENTLIST

#include <stdint.h>
#include <stdbool.h>

struct netclient_t {
	uint64_t u_id;
	uint64_t address; /* This information is only available on the Server*/
};

struct netclientlist_t {
	struct netclient_t cli;
	struct netclientlist_t* nxt_cli;
};

struct netclientlist_t* NCL_alloc ();

void NCL_addClient (struct netclientlist_t* node ,struct netclient_t cli);

struct netclientlist_t* NCL_remClient (struct netclientlist_t* node , uint64_t address);

void NCL_freeList  (struct netclientlist_t* node);

bool NCL_isClient (struct netclientlist_t* node ,uint64_t address);

#endif
