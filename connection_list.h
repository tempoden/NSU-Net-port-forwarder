#ifndef _CONNECTION_LIST_
#define _CONNECTION_LIST_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifndef CONNECTION_BUFFER_SIZE
#define CONNECTION_BUFFER_SIZE 1024
#endif

typedef struct _connection{
	char sendBuffer[CONNECTION_BUFFER_SIZE],
		recvBuffer[CONNECTION_BUFFER_SIZE];
	int connSockfd, coupledSockfd,
		sendOffset, recvOffset;
	int eof ;
} connection_t;


typedef struct _list{
	struct _list* prev;
	struct _list* next;
	connection_t* content;
} connection_list_t;


void set_fds(connection_t* connection, fd_set* readfds, fd_set* writefds);

connection_t* create_connection(int conn, int coupled);
void process_connection(connection_t* connection, fd_set* readfds, fd_set* writefds);

connection_list_t* init_list();
void append(connection_list_t* head, connection_t* to_add);
void remove_connection(connection_list_t* to_remove);
void destroy(connection_list_t* head);

#endif // !_CONNECTION_LIST_
