#ifndef _FORWARDER_
#define _FORWARDER_

#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "connection_list.h"

typedef struct _forwarder {
	int serverfd;
	struct sockaddr_in redirect;
	fd_set readfs, writefs;
	int maxfd;
	connection_list_t* connections;
} forwarder_t;

struct sockaddr_in get_addr(char* host, int port);

forwarder_t* init(int lport, char* rhost, int rport);

int run(forwarder_t* forwarder);
void process_connections(forwarder_t* forwarder);
int get_redirection_socket(forwarder_t* forwarder);
#endif // !_FORWARDER_
