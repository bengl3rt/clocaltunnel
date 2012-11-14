#include "clocaltunnel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <curl/curl.h>

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <pwd.h>

#include "jsmn/jsmn.h"
#include <libssh2.h>

struct clocaltunnel_client {
	unsigned int local_port;
	char *external_url;
	pthread_t recv_thread;
	clocaltunnel_error last_err;
	clocaltunnel_client_state state;
};

CURL *curl_inst;

struct MemoryStruct {
  char *memory;
  size_t size;
};

struct open_localtunnel {
	char *host;
	char *port;
	char *user;
};
		 
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		exit(EXIT_FAILURE);
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

char *get_ssh_key() {
	struct passwd *pw = getpwuid(getuid());

	const char *homedir = pw->pw_dir;

	const char *ssh_key_path = "/.ssh/id_rsa.pub";

	char full_ssh_key_path[strlen(homedir)+strlen(ssh_key_path)+1];

	sprintf(full_ssh_key_path, "%s%s", homedir, ssh_key_path);

	long f_size;
	char* key;
	size_t key_size, result;
	FILE* fp = fopen(full_ssh_key_path, "r");

	if (fp == NULL) {
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	f_size = ftell(fp); /* This returns 29696, but file is 85 bytes */
	fseek(fp, 0, SEEK_SET);
	key_size = sizeof(char) * f_size;
	key = malloc(key_size);
	result = fread(key, 1, f_size, fp);

	fclose(fp);

	return key;
}

int contact_localtunnel_service(struct open_localtunnel *tunnelinfo) {
	int rc = 0;
	char *tunnel_open_url = "http://open.localtunnel.com";

	struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;

	struct MemoryStruct chunk;

	chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */

	char *ssh_key = get_ssh_key();

	if (ssh_key == NULL) {
		return CLOCALTUNNEL_ERROR_SSH_KEY;
	}

	curl_easy_setopt(curl_inst, CURLOPT_URL, tunnel_open_url);

	/* send all data to this function  */ 
	curl_easy_setopt(curl_inst, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */ 
	curl_easy_setopt(curl_inst, CURLOPT_WRITEDATA, (void *)&chunk);

	/* some servers don't like requests that are made without a user-agent
	 field, so we provide one */ 
	curl_easy_setopt(curl_inst, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "key",
                 CURLFORM_COPYCONTENTS, ssh_key,
                 CURLFORM_END);

	curl_easy_setopt(curl_inst, CURLOPT_HTTPPOST, formpost);

	/* get it! */ 
	CURLcode res = curl_easy_perform(curl_inst);

	curl_formfree(formpost);

	if (res != CURLE_OK) {
#if CLOCALTUNNEL_DEBUG
		   fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
#endif
		   return CLOCALTUNNEL_ERROR_CURL;

	}

	jsmn_parser p;
	jsmntok_t tok[10];
	
	jsmn_init(&p);

	rc = jsmn_parse(&p, chunk.memory, tok, 10);

	if (rc) {
		goto cleanup;
	}

	char *host = malloc(tok[2].end - tok[2].start + 1);
	memcpy(host, chunk.memory+tok[2].start, tok[2].end - tok[2].start);
	host[tok[2].end - tok[2].start] = '\0';

	char *port = malloc(tok[4].end - tok[4].start + 1);
	memcpy(port, chunk.memory+tok[4].start, tok[4].end - tok[4].start);
	port[tok[4].end - tok[4].start] = '\0';

	char *user = malloc(tok[8].end - tok[8].start + 1);
	memcpy(user, chunk.memory+tok[8].start, tok[8].end - tok[8].start);
	user[tok[8].end - tok[8].start] = '\0';
	
	tunnelinfo->port = port;
	tunnelinfo->host = host;
	tunnelinfo->user = user;

	//TODO catch errors and return err code
cleanup:
	free(chunk.memory);

	return CLOCALTUNNEL_OK;
}

int get_socket_to_localtunnel(int *sock, char *host) {
	struct addrinfo hints, *res, *res0;
	int error;
	const char *cause = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(host, "22", &hints, &res0);
	if (error) {
		printf("getaddrinfo error\n");
		return CLOCALTUNNEL_ERROR_SOCKET;
	}
	*sock = -1;
	for (res = res0; res; res = res->ai_next) {
	   *sock = socket(res->ai_family, res->ai_socktype,
	       res->ai_protocol);
	   if (*sock < 0) {
	           cause = "socket";
	           continue;
	   }
	  
	   if (connect(*sock, res->ai_addr, res->ai_addrlen) < 0) {
	           cause = "connect";
	           close(*sock);
	           *sock = -1;
	           continue;
	   }

	   break;  /* okay we got one */
	}
	
	freeaddrinfo(res0);

	if (*sock == -1) {
		printf("Socket error in %s\n", cause);
		return CLOCALTUNNEL_ERROR_SOCKET;
	}

	return CLOCALTUNNEL_OK;
}

int do_tunnel_listen(char *local_destip, unsigned int local_destport, LIBSSH2_SESSION *session, LIBSSH2_CHANNEL *channel) {
	int rc = 0;
	int i;
	struct sockaddr_in sin;
    socklen_t sinlen = sizeof(sin);	
    fd_set fds;
    struct timeval tv;
    ssize_t len, wr;
    char buf[16384];

	int forwardsock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(local_destport);
    if (INADDR_NONE == (sin.sin_addr.s_addr = inet_addr(local_destip))) {
        perror("inet_addr");
        goto shutdown;
    }
    if (-1 == connect(forwardsock, (struct sockaddr *)&sin, sinlen)) {
        perror("connect");
        goto shutdown;
    }
 
    /* Must use non-blocking IO hereafter due to the current libssh2 API */ 
    libssh2_session_set_blocking(session, 0);

    while (1) {
        FD_ZERO(&fds);
        FD_SET(forwardsock, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        rc = select(forwardsock + 1, &fds, NULL, NULL, &tv);
        if (-1 == rc) {
            perror("select");
            goto shutdown;
        }
        if (rc && FD_ISSET(forwardsock, &fds)) {
            len = recv(forwardsock, buf, sizeof(buf), 0);
            if (len < 0) {
                perror("read");
                goto shutdown;
            } else if (0 == len) {
                goto shutdown;
            }
            wr = 0;
            do {
                i = libssh2_channel_write(channel, buf, len);

                if (i < 0) {
                    fprintf(stderr, "libssh2_channel_write: %d\n", i);
                    goto shutdown;
                }
                wr += i;
            } while(i > 0 && wr < len);
        }
        while (1) {
            len = libssh2_channel_read(channel, buf, sizeof(buf));

            if (LIBSSH2_ERROR_EAGAIN == len)
                break;
            else if (len < 0) {
                fprintf(stderr, "libssh2_channel_read: %d", (int)len);
                goto shutdown;
            }
            wr = 0;
            while (wr < len) {
                i = send(forwardsock, buf + wr, len - wr, 0);
                if (i <= 0) {
                    perror("write");
                    goto shutdown;
                }
                wr += i;
            }
            if (libssh2_channel_eof(channel)) {
                goto shutdown;
            }
        }
    }

shutdown:
	libssh2_session_set_blocking(session, 1);
	close(forwardsock);
	return rc;
}


int open_ssh_tunnel(struct open_localtunnel tunnelinfo, struct clocaltunnel_client *c) {
	const char *remote_listenhost = "localhost"; /* resolved by the server */
	char *local_destip = "127.0.0.1";

	int rc = CLOCALTUNNEL_OK;
	LIBSSH2_SESSION *session;
	LIBSSH2_AGENT *agent = NULL;
	LIBSSH2_LISTENER *listener = NULL;
    struct libssh2_agent_publickey *identity, *prev_identity = NULL;

#ifdef CLOCALTUNNEL_DEBUG
    printf("Host: %s Port: %s User: %s\n", tunnelinfo.host, tunnelinfo.port, tunnelinfo.user);
#endif

	int sock;  

	if (get_socket_to_localtunnel(&sock, tunnelinfo.host) != CLOCALTUNNEL_OK) {
		return CLOCALTUNNEL_ERROR_SOCKET;
	}

	session = libssh2_session_init();

    if (libssh2_session_handshake(session, sock)) {

        fprintf(stderr, "Failure establishing SSH session\n");
        return CLOCALTUNNEL_ERROR_SSH;
    }

	 /* Connect to the ssh-agent */ 
    agent = libssh2_agent_init(session);

    if (!agent) {
        fprintf(stderr, "Failure initializing ssh-agent support\n");
        rc = CLOCALTUNNEL_ERROR_SSH;
        goto shutdown;
    }
    if (libssh2_agent_connect(agent)) {

        fprintf(stderr, "Failure connecting to ssh-agent\n");
        rc = CLOCALTUNNEL_ERROR_SSH;
        goto shutdown;
    }
    if (libssh2_agent_list_identities(agent)) {

        fprintf(stderr, "Failure requesting identities to ssh-agent\n");
        rc = CLOCALTUNNEL_ERROR_SSH;
        goto shutdown;
    }
    while (1) {
        rc = libssh2_agent_get_identity(agent, &identity, prev_identity);

        if (rc == 1) {
        	if (prev_identity == NULL) {
        		rc = CLOCALTUNNEL_ERROR_SSH_AGENT;
        	}
        	
            break;	
        }

        if (rc < 0) {
#ifdef CLOCALTUNNEL_DEBUG
            fprintf(stderr,
                    "Failure obtaining identity from ssh-agent support\n");
#endif
            rc = CLOCALTUNNEL_ERROR_SSH_AGENT;
            goto shutdown;
        }
        if (libssh2_agent_userauth(agent, tunnelinfo.user, identity)) {
#ifdef CLOCALTUNNEL_DEBUG
            printf("Authentication with username %s and "
                   "public key %s failed!\n",
                   tunnelinfo.user, identity->comment);
#endif
        } else {
#ifdef CLOCALTUNNEL_DEBUG
            printf("Authentication with username %s and "
                   "public key %s succeeded!\n",
                   tunnelinfo.user, identity->comment);
#endif
            break;
        }
        prev_identity = identity;
    }
    if (rc) {
        fprintf(stderr, "Couldn't continue authentication\n");
        goto shutdown;
    }

    unsigned int remote_wantport = atoi(tunnelinfo.port);
    int remote_listenport;

#ifdef CLOCALTUNNEL_DEBUG
    printf("Asking server to listen on remote %s:%d\n", remote_listenhost, remote_wantport);
#endif

    listener = libssh2_channel_forward_listen_ex(session, remote_listenhost, remote_wantport, &remote_listenport, 10);
    if (!listener) {
        fprintf(stderr, "Could not start the tcpip-forward listener!\n"
                "(Note that this can be a problem at the server!"
                " Please review the server logs.)\n");
        rc = CLOCALTUNNEL_ERROR_SSH;
        goto shutdown;
    }

#ifdef CLOCALTUNNEL_DEBUG
    printf("Server is listening on %s:%d\n", remote_listenhost,
        remote_listenport);
 
    printf("Waiting for remote connection\n");
#endif

    c->state = CLOCALTUNNEL_CLIENT_TUNNEL_OPENED;

    while (1) {
    	LIBSSH2_CHANNEL *channel = libssh2_channel_forward_accept(listener);

	    if (!channel) {
	        fprintf(stderr, "Could not accept connection!\n"
	                "(Note that this can be a problem at the server!"
	                " Please review the server logs.)\n");
	        rc = CLOCALTUNNEL_ERROR_SSH;
	        goto shutdown;
	    }
    
	    do_tunnel_listen(local_destip, c->local_port, session, channel);
		
	    libssh2_channel_free(channel);
    }

shutdown:
 
    if (listener)
        libssh2_channel_forward_cancel(listener);

    libssh2_session_disconnect(session, "Client disconnecting normally");

    libssh2_session_free(session);

    close(sock);
 
    libssh2_exit();
 
    return rc;
}


void tunnel_local_port(struct clocaltunnel_client *c) {
	struct open_localtunnel tunnelinfo;

	memset(&tunnelinfo, 0, sizeof(struct open_localtunnel));
	
	c->last_err = contact_localtunnel_service(&tunnelinfo);

	if (c->last_err != CLOCALTUNNEL_OK) {
		c->state = CLOCALTUNNEL_CLIENT_ERROR;
		goto cleanup;
	}

	c->state = CLOCALTUNNEL_CLIENT_TUNNEL_REGISTERED;

	c->external_url = tunnelinfo.host;

	c->last_err = open_ssh_tunnel(tunnelinfo, c);

	if (c->last_err != CLOCALTUNNEL_OK) {
		printf("open_ssh_tunnel error: %d\n", c->last_err);
		c->state = CLOCALTUNNEL_CLIENT_ERROR;
		goto cleanup;
	}

cleanup:
	if (tunnelinfo.host)
		free(tunnelinfo.host);

	if (tunnelinfo.user)
		free(tunnelinfo.user);

	if (tunnelinfo.port)
		free(tunnelinfo.port);
}

void *client_thread_start(void *client_data) {
	tunnel_local_port((struct clocaltunnel_client *)client_data);	
	pthread_exit(NULL);
}

int clocaltunnel_client_start(struct clocaltunnel_client *c) {
	if (pthread_create(&c->recv_thread, NULL, client_thread_start, (void*)c)) {
		printf("Pthread error\n");
		c->last_err = CLOCALTUNNEL_ERROR_PTHREAD;
		c->state = CLOCALTUNNEL_CLIENT_ERROR;
		return (-1);
	} else {
		c->state = CLOCALTUNNEL_CLIENT_STARTING;
		return CLOCALTUNNEL_OK;
	}
}

void clocaltunnel_global_init() {
	curl_global_init(CURL_GLOBAL_ALL);

	curl_inst = curl_easy_init();
}

void clocaltunnel_global_cleanup() {
	curl_easy_cleanup(curl_inst);
	curl_global_cleanup();
}

struct clocaltunnel_client *clocaltunnel_client_alloc() {
	struct clocaltunnel_client *new_client = calloc(1, sizeof(struct clocaltunnel_client));

	return new_client;
}

void clocaltunnel_client_stop(struct clocaltunnel_client *c) {
	if(pthread_cancel(c->recv_thread)) {
		printf("Error stopping thread\n");
	} else {
		c->state = CLOCALTUNNEL_CLIENT_INITIALIZED;
	}
}


void clocaltunnel_client_free(struct clocaltunnel_client *client) {
	free(client);
}

void clocaltunnel_client_init(struct clocaltunnel_client *client, unsigned int local_port) {
	client->local_port = local_port;
	client->state = CLOCALTUNNEL_CLIENT_INITIALIZED;
	client->last_err = CLOCALTUNNEL_OK;
}

clocaltunnel_client_state clocaltunnel_client_get_state(struct clocaltunnel_client *c) {
	return c->state;
}

char *clocaltunnel_client_get_external_url(struct clocaltunnel_client *c) {
	if (c->state < CLOCALTUNNEL_CLIENT_TUNNEL_REGISTERED) {
		return NULL;
	}

	return c->external_url;
}

clocaltunnel_error clocaltunnel_client_get_last_error(struct clocaltunnel_client *c) {
	return c->last_err;
}