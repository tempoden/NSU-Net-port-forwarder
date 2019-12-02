#include <stdio.h>
#include <stdlib.h>
#include "forwarder.h"


int main(int argc, char **argv) {
	int lport, rport = 80;

	if (argc < 3) {
		fprintf(stderr, "Usage: <lport> <rhost> <rport>\n");
		return 0;
	}

	lport = atoi(argv[1]);
	rport = (argc > 3 ? atoi(argv[3]) : 80);

	if (lport == 0 || rport == 0) {
		fprintf(stderr, "Wrong port number\n");
		return 1;
	}

	// C-style OOP
	forwarder_t* forwarder = init(lport, argv[2], rport);
	// Handle errors and do cleanup
	if (run(forwarder)) {
		destroy(forwarder->connections);
		close(forwarder->serverfd);
	}

	return 0;
}