#ifndef NTTHRD_CLI
#define NTTHRD_CLI

#include <stdint.h>
#include <stdbool.h>

bool initialize_client ();
void start_client_thread ();
void terminate_client_thread ();
void wait_for_termination ();

// Shortcut Functions to send specific Packets 
void MSG_disconnect ();
void MSG_chunkrequest (int32_t x, int32_t z);

#endif
