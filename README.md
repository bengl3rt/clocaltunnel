clocaltunnel
============

Clocaltunnel is a [LocalTunnel][lt-website] client written in C, designed for embeddability.

[lt-website]: http://progrium.com/localtunnel/

TODO
----

* Rewrite for Localtunnel v2 protocol

Use
---

Simply running
	
	make

in the source directory will spit out a nice executable, which will open a localtunnel to whatever you happen to have running locally on port 80. The executable generated can be run as follows:

	./clocaltunneltest

This kicks off the code you see in example.c. I recommend doing this once to ferret out any configuration issues before diving into the code. One common problem is the lack of an ssh key on your machine. Localtunnel requires one, so create it using:

	ssh-keygen -t rsa
	ssh-add ~/.ssh/id_rsa

Once you are ready to integrate the library into your own project, I suggest building a dylib:

	make dylib

The makefile also includes targets for building a fully self-contained static library and a basic example of how to link against clocaltunnel.


Dependencies
------------

I have developed and tested clocaltunnel on OS X. It depends on the Xcode Command Line tools. 

It also depends on libcurl and pthread, which come with the OS, and libssh2, which you will have to install. I suggest using [HomeBrew][homebrew-website].

[homebrew-website]: http://mxcl.github.com/homebrew/

If you manage to statically cross-compile libssh2, you should be able to use the staticlibssh target in the makefile to build clocaltunnel for iOS.

Finally, nothing here is actually Mac-specific so a minimal amount of hacking will probably allow you to use clocaltunnel on Linux and Cygwin. 

License
-------

MIT