LIB_DEPS=-lcurl -lssh2
LIB_SOURCES=jsmn/jsmn.c clocaltunnel.c

example:
	gcc -O3 -Wall -o clocaltunneltest $(LIB_DEPS) $(LIB_SOURCES) example.c

clean:
	rm -f clocaltunneltest

all: example