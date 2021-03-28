#ifndef NTHRD_SRV
#define NTHRD_SRV

#include <stdbool.h>

bool initialize_server ();
void start_server_thread ();
void wait_for_termination ();
void terminate_server_thread ();


#endif
