#include "connection_list.h"

void set_fds(connection_t* connection, fd_set* readfds, fd_set* writefds) {
	if (!connection->eof) {
		FD_SET(connection->connSockfd, readfds);
	}
	FD_SET(connection->coupledSockfd, writefds);
}

connection_t* create_connection(int conn, int coupled) {
	connection_t* connection = (connection_t*)malloc(sizeof(connection_t));
	connection->connSockfd = conn;
	connection->coupledSockfd = coupled;
	connection->eof = connection->recvOffset = connection->sendOffset = 0;
	return connection;
}

void process_connection(connection_t* connection, fd_set *readfds, fd_set *writefds) {
	if (FD_ISSET(connection->connSockfd, readfds) && CONNECTION_BUFFER_SIZE != connection->recvOffset) {
		int bytesRead = recv(connection->connSockfd, connection->recvBuffer + connection->recvOffset,
							 CONNECTION_BUFFER_SIZE - connection->recvOffset, 0);
		if (0 == bytesRead) {
			connection->eof = 1;
		}
		connection->recvOffset += bytesRead;
	}
	if (FD_ISSET(connection->coupledSockfd, writefds) && connection->sendOffset != connection->recvOffset) {
		int bytesWrote = send(connection->coupledSockfd, connection->recvBuffer + connection->sendOffset,
							  connection->recvOffset - connection->sendOffset, 0);
		connection->sendOffset += bytesWrote;
		if (CONNECTION_BUFFER_SIZE == connection->recvOffset && CONNECTION_BUFFER_SIZE == connection->sendOffset) {
			connection->recvOffset = 0;
			connection->sendOffset = 0;
		}
	}
}

connection_list_t* init_list() {
	connection_list_t* head = (connection_list_t*) malloc(sizeof(connection_list_t));
	head->prev = head->next = NULL;
	head->content = NULL;
	return head;
}

void append(connection_list_t* head, connection_t* to_add) {
	if (head->content == NULL) {
		head->content = to_add;
		return;
	}
	while (head->next != NULL) {
		head = head->next;
	}
	head->next = (connection_list_t*)malloc(sizeof(connection_list_t));
	head->next->prev = head;
	head->next->content = to_add;
	head->next->next = NULL;
}

void remove_connection(connection_list_t* to_remove) {
	if (to_remove == NULL) {
		return;
	}
	if (to_remove->prev == NULL && to_remove->next == NULL) {
		close(to_remove->content->connSockfd);
		free(to_remove->content);
		return;
	}
	connection_list_t* prev = to_remove->prev;
	connection_list_t* next = to_remove->next;
	if (prev != NULL && next != NULL) {
		prev->next = next;
		next->prev = prev;
	}
	else if (prev != NULL) {
		prev->next = NULL;
	}
	else {
		next->prev = NULL;
	}
	if (to_remove->content != NULL) {
		close(to_remove->content->connSockfd);
		free(to_remove->content);
	}
	free(to_remove);
}

void destroy(connection_list_t* head) {
	printf("DESTUCTION\n");
	if (head == NULL) {
		return;
	}
	while (head->next != NULL) {
		head = head->next;
		remove_connection(head->prev);
	}
	remove_connection(head);
	free(head);
}