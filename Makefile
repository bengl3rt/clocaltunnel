LIB_DEPS=-lcurl -pthread
LIB_SOURCES=jsmn/jsmn.c clocaltunnel.c

example:
	gcc -O3 -Wall -o clocaltunneltest $(LIB_DEPS) -lssh2 $(LIB_SOURCES) example.c

clean:
	rm -f clocaltunneltest*
	rm -f *.o
	rm -f *.dylib
	rm -f *.a

dylib:
	gcc -dynamiclib -O3 -Wall -o libclocaltunnel.dylib $(LIB_DEPS) $(LIB_SOURCES)

#libexample: 
#	gcc -O3 -Wall $(LIB_DEPS) -o clocaltunneltestdylib -L. -lclocaltunnel example.c


#This path works if your Homebrew install is in the default location
#Change it if yours is not, or if you built libssh2 from source someplace else
LIBSSH2_STATIC_PATH=/usr/local/Cellar/libssh2/1.4.2/lib/libssh2.a

staticlibssh:
	gcc $(LIBSSH2_STATIC_PATH) -dynamiclib -o libclocaltunnel.dylib -lz -lcurl -pthread -lcrypto $(LIB_SOURCES)

libexample:
	gcc $(LIB_DEPS) -L. -lclocaltunnel -o clocaltunneltest example.c

static: staticlibssh libexample
dynamic: dylib libexample

all: example