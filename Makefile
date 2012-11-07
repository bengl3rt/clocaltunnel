#ARCHS=-arch i386
CFLAGS=-O3 -Wall $(ARCHS)
LIB_DEPS=-lcurl -pthread -lz -lcrypto
LIB_SOURCES=jsmn/jsmn.c clocaltunnel.c

#Build the example code with the clocaltunnel source emebdded directly
example:
	gcc $(CFLAGS) -o clocaltunneltest $(LIB_DEPS) -lssh2 $(LIB_SOURCES) example.c

clean:
	rm -f clocaltunneltest*
	rm -f *.o
	rm -f *.dylib
	rm -f *.a

#Build clocaltunnel as a shared library
dylib:
	gcc $(ARCHS) $(DEFINES) -dynamiclib $(CFLAGS) -o libclocaltunnel.dylib $(LIB_DEPS) -lssh2 $(LIB_SOURCES)

#Point at the static library of libssh2. Homebrew installs one or you can build your own
LIBSSH2_STATIC_PATH=/Users/ben/Code/libssh2-1.4.2/src/.libs/libssh2.a

#Build clocaltunnel as a shared library with libssh2 linked statically
staticlibssh:
	gcc $(LIBSSH2_STATIC_PATH) -dynamiclib -o libclocaltunnel.dylib -lz -lcurl -pthread -lcrypto $(LIB_SOURCES)

#Build a statical library of clocaltunnel containing a statically linked libssh2
staticlib:
	rm -f *.o
	gcc $(ARCHS) $(DEFINES) -c $(LIB_SOURCES)
	ar -x $(LIBSSH2_STATIC_PATH)
	libtool -o libclocaltunnel.a *.o
	rm __.SYMDEF\ SORTED
	rm -f *.o

staticlibdebug: DEFINES = -DCLOCALTUNNEL_DEBUG
staticlibdebug: staticlib

#Build the example code linking the shared or static clocaltunnel libraru
libexample:
	gcc $(CFLAGS) $(LIB_DEPS) -L. -lclocaltunnel -o clocaltunneltest example.c

static: staticlib libexample
dynamic: dylib libexample

all: example