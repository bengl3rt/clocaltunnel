#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "clocaltunnel.h"

static struct clocaltunnel_client *my_client;

void cleanup() {
	printf("\nShutting down clocaltunnel client...\n");
	clocaltunnel_client_stop(my_client);
	clocaltunnel_client_free(my_client);

	clocaltunnel_global_cleanup();
}

void int_handler(int signal) {
	if (my_client) {
		cleanup();
	}

	exit(0);
}

int main(int argc, char **argv) {
	signal(SIGINT, int_handler);

	clocaltunnel_global_init();

	clocaltunnel_error err;

	my_client = clocaltunnel_client_alloc(&err);

	clocaltunnel_client_init(my_client, 80);

	printf("Starting clocaltunnel client...\n");

	clocaltunnel_client_start(my_client);

	while (clocaltunnel_client_get_state(my_client) < CLOCALTUNNEL_CLIENT_TUNNEL_OPENED) {
		usleep(1000000);

		if (clocaltunnel_client_get_state(my_client) == CLOCALTUNNEL_CLIENT_ERROR)  {
			printf("Error! %d\n", clocaltunnel_client_get_last_error(my_client));
		}
	}

	char external_url[50];

	strcpy(external_url, clocaltunnel_client_get_external_url(my_client));

	printf("Client running. Use Ctrl+C to exit. URL: %s\n", external_url);

	while(1) {}

	return 0;
}