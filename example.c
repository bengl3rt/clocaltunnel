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
			clocaltunnel_error err = clocaltunnel_client_get_last_error(my_client);

			switch (err) {
                case CLOCALTUNNEL_ERROR_MALLOC:
                {
                    printf("Unable to allocate memory for localtunnel client\n");
                    break;
                }
                case CLOCALTUNNEL_ERROR_MISC:
                {
                    printf("Misc error in localtunnel client\n");
                    break;
                }
                    
                case CLOCALTUNNEL_ERROR_PTHREAD:
                {
                    printf("Error starting receive thread in localtunnel client\n");
                    break;
                }
                case CLOCALTUNNEL_ERROR_CURL:
                {
                    printf("Error communicating with localtunnel web service\n");
                    break;
                }
                case CLOCALTUNNEL_ERROR_SOCKET:
                {
                    printf("Unable to open a socket to localtunnel server\n");
                    break;
                }
                case CLOCALTUNNEL_ERROR_SSH:
                {
                    printf("Error establishing SSH communication with localtunnel server\n");
                    break;
                }
                case CLOCALTUNNEL_ERROR_SSH_AGENT:
                {
                	printf("SSH agent could not authenticate. Try adding a key using ssh-add\n");
                	break;
                }
                case CLOCALTUNNEL_ERROR_SSH_KEY:
                {
                    printf("No SSH key found on disk. Try creating one using ssh-keygen\n");
                    break;
                }
                case CLOCALTUNNEL_ERROR_JSON:
                {
                    printf("JSON parse error\n");
                    break;
                }
                default:
                    break;
            }
		}
	}

	char external_url[50];

	strcpy(external_url, clocaltunnel_client_get_external_url(my_client));

	printf("Client running. Use Ctrl+C to exit. URL: %s\n", external_url);

	while(1) {}

	return 0;
}