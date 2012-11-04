struct clocaltunnel_client;

typedef enum {
	CLOCALTUNNEL_OK = 0,
	CLOCALTUNNEL_ERROR_MALLOC,
	CLOCALTUNNEL_ERROR_MISC,
	CLOCALTUNNEL_ERROR_PTHREAD,
	CLOCALTUNNEL_ERROR_CURL,
	CLOCALTUNNEL_ERROR_SOCKET,
	CLOCALTUNNEL_ERROR_SSH,
} clocaltunnel_error;

typedef enum {
	CLOCALTUNNEL_CLIENT_INITIALIZED = 1,
	CLOCALTUNNEL_CLIENT_ERROR,
	CLOCALTUNNEL_CLIENT_STARTING,
	CLOCALTUNNEL_CLIENT_TUNNEL_REGISTERED,
	CLOCALTUNNEL_CLIENT_TUNNEL_OPENED,
} clocaltunnel_client_state;

void clocaltunnel_global_init();
void clocaltunnel_global_cleanup();

void clocaltunnel_client_free(struct clocaltunnel_client *client);
struct clocaltunnel_client *clocaltunnel_client_alloc();

void clocaltunnel_client_init(struct clocaltunnel_client *client, unsigned int local_port);
int clocaltunnel_client_start(struct clocaltunnel_client *c); 
void clocaltunnel_client_stop(struct clocaltunnel_client *c);

clocaltunnel_client_state clocaltunnel_client_get_state(struct clocaltunnel_client *c);
clocaltunnel_error clocaltunnel_client_get_last_error(struct clocaltunnel_client *c);
char *clocaltunnel_client_get_external_url(struct clocaltunnel_client *c);