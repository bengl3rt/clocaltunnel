LIB_DEPS=-lcurl -pthread -lz -lcrypto
LIB_SOURCES=jsmn/jsmn.c clocaltunnel.c

#Build the example code with the clocaltunnel source emebdded directly
example:
	gcc -O3 -Wall -o clocaltunneltest $(LIB_DEPS) -lssh2 $(LIB_SOURCES) example.c

clean:
	rm -f clocaltunneltest*
	rm -f *.o
	rm -f *.dylib
	rm -f *.a

#Build clocaltunnel as a shared library
dylib:
	gcc -dynamiclib -O3 -Wall -o libclocaltunnel.dylib $(LIB_DEPS) -lssh2 $(LIB_SOURCES)

#This path works if your Homebrew install is in the default location
#Change it if yours is not, or if you built libssh2 from source someplace else
LIBSSH2_STATIC_PATH=/usr/local/Cellar/libssh2/1.4.2/lib/libssh2.a

#Build clocaltunnel as a shared library with libssh2 linked statically
staticlibssh:
	gcc $(LIBSSH2_STATIC_PATH) -dynamiclib -o libclocaltunnel.dylib -lz -lcurl -pthread -lcrypto $(LIB_SOURCES)

#Build a statical library of clocaltunnel containing a statically linked libssh2
staticlib:
	gcc -c $(LIB_SOURCES)
	ar -x $(LIBSSH2_STATIC_PATH)
	ar rcs libclocaltunnel.a *.o
	rm __.SYMDEF\ SORTED

#Build the example code linking the shared or static clocaltunnel libraru
libexample:
	gcc $(LIB_DEPS) -L. -lclocaltunnel -o clocaltunneltest example.c

static: staticlib libexample
dynamic: dylib libexample

all: example