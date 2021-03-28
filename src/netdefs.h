#ifndef NETDEFS
#define NETDEFS

#include <stdint.h>

#define SERVER_PORT 8080
#define CLIENT_PORT 8090

#define MAX_PACKET_SIZE 320

#define NP_PING              255
#define NP_PONG              254

#define NP_CHUNKREQUEST      8
#define NP_CHUNKLAYER        7

#define NP_BLOCKUPDATE       16

#define NP_PLAYERSTATEUPDATE 24

#define NP_CLIENTLISTREQUEST 32
#define NP_CLIENTINFO        31
#define NP_CLIENTREMOVE      30

#define NP_CONNECTIONREQUEST 128
#define NP_CONNECTIONACCEPT  127
#define NP_CONNECTIONREFUSE  126

#define NP_DISCONNECT        112
#define NP_TERMINATE         111

#endif
