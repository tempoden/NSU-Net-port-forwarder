#include "forwarder.h"

#ifndef max
int max(int a, int b) {
	return (a > b) ? a : b;
}
#endif

struct sockaddr_in get_addr(char* host, int port) {
	struct sockaddr_in sockAddr;
	struct addrinfo *addr = NULL;

	memset(&sockAddr, 0, sizeof(struct sockaddr_in));

	if (host != NULL && strlen(host) > 0) {
		struct addrinfo hints;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		if (getaddrinfo(host, NULL, &hints, &addr) != 0) {
			perror("getaddrinfo: ");
			exit(EXIT_FAILURE);
		}
		memcpy(&sockAddr, addr->ai_addr, sizeof(struct sockaddr));
		freeaddrinfo(addr);
	} else {
		sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(port);
	//printf("got: %d\n", sockAddr.sin_port);

	return sockAddr;
}

forwarder_t* init(int lport, char* rhost, int rport) {
	forwarder_t* forwarder = (forwarder_t*)malloc(sizeof(forwarder_t));
	forwarder->connections = NULL;
	forwarder->maxfd = forwarder->serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if (forwarder->serverfd == -1) {
		perror("socket: ");
		exit(EXIT_FAILURE);
	}
	if (forwarder->serverfd >= FD_SETSIZE) {
		perror("socket fd is out of select range");
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	setsockopt(forwarder->serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

	struct sockaddr_in listenaddr = get_addr(NULL, lport);

	if (bind(forwarder->serverfd, (struct sockaddr *)(&listenaddr), sizeof(struct sockaddr_in))) {
		perror("bind ");
		exit(EXIT_FAILURE);
	}
	if (listen(forwarder->serverfd, SOMAXCONN)) {
		perror("listen ");
		exit(EXIT_FAILURE);
	}
	if (fcntl(forwarder->serverfd, F_SETFL, fcntl(forwarder->serverfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
		perror("fcnt: cannot make server socket nonblock");
		exit(EXIT_FAILURE);
	}

	forwarder->redirect = get_addr(rhost, rport);

	forwarder->connections = init_list();

	return forwarder;
}

int run(forwarder_t* forwarder) {
	connection_list_t* list;
	int client_socket, redirected_socked;
	fprintf(stderr, "Forwarder is up\n");
// Just another way to write while(true). I am true fan of goto 4Head
WORKING_LOOP:
		FD_ZERO(&forwarder->readfs);
		FD_ZERO(&forwarder->writefs);
		FD_SET(forwarder->serverfd, &forwarder->readfs);
		
		list = forwarder->connections;
		while (list != NULL) {
			if (list->content != NULL) {
				set_fds(list->content, &forwarder->readfs, &forwarder->writefs);
			}
			list = list->next;
		}

		int selected = select(forwarder->maxfd + 1, &forwarder->readfs, &forwarder->writefs, NULL, NULL);
		if (selected < 0) {
			return 1;
		}
		if (selected == 0)
			goto WORKING_LOOP;
		if (FD_ISSET(forwarder->serverfd, &forwarder->readfs)) {
			struct sockaddr_in client_addr;
			socklen_t addrlen = sizeof(struct sockaddr_in);
			client_socket = accept(forwarder->serverfd, (struct sockaddr *)(&client_addr), &addrlen);
			if (client_socket < 0) {
				perror("accept ");
				return 1;
			}
			fcntl(client_socket, F_SETFL, fcntl(client_socket, F_GETFL, 0) | O_NONBLOCK);
			redirected_socked = get_redirection_socket(forwarder);
			if (redirected_socked < 0) {
				return 1;
			}
			append(forwarder->connections, create_connection(client_socket, redirected_socked));
			append(forwarder->connections, create_connection(redirected_socked, client_socket));
			forwarder->maxfd = max(forwarder->maxfd, max(client_socket, redirected_socked));
		}

		process_connections(forwarder);

	goto WORKING_LOOP;

	return 0; //LOL
}

void process_connections(forwarder_t* forwarder) {
	int removed = 0;
	connection_list_t* list = forwarder->connections;
	while (list != NULL) {
		if (list->content != NULL) {
			process_connection(list->content, &forwarder->readfs, &forwarder->writefs);
			if (list->content->eof && list->content->sendOffset != list->content->recvOffset) {
				if (list->next != NULL) {
					list = list->next;
					remove_connection(list->prev);
				} else {
					remove_connection(list);
					list = NULL;
				}
				printf("Connection removed\n");
				removed++;
			} else {
				list = list->next;
			}
			continue;
		}
		list = list->next;
	}
	if (removed) {
		list = forwarder->connections;
		while (list != NULL) {
			if (list->content != NULL) {
				forwarder->maxfd = max(forwarder->maxfd, max(list->content->connSockfd, list->content->coupledSockfd));
			}
			list = list->next;
		}
	}
}

int get_redirection_socket(forwarder_t* forwarder) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1 || connect(sock, (struct sockaddr*)&forwarder->redirect, sizeof(struct sockaddr_in))) {
		perror("redirection failed ");
		return -1;
	}
	return sock;
}