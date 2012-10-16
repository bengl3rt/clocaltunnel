#include "clocaltunnel.h"

int main(int argc, char **argv) {
	clocaltunnel_global_init();

	clocaltunnel_error err;

	struct clocaltunnel_client *my_client = clocaltunnel_client_alloc(&err);

	clocaltunnel_client_init(my_client, 80);

	clocaltunnel_client_start(my_client, &err);

	clocaltunnel_client_free(my_client);

	clocaltunnel_global_cleanup();
	return 0;
}