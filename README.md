clocaltunnel
============

Clocaltunnel is a [LocalTunnel][lt-website] client written in C, designed for embeddability.

[lt-website]: http://progrium.com/localtunnel/

TODO
----

* Lots of error handling (in its present state it likely crashes or fails silently a lot)
* Do all SSH communication and tunneling in a separate thread so that clocaltunnel_client_start doesn't block
* Fail gracefully if there is no SSH key on disk
* Upload the public key if there is one on disk but the Localtunnel service hasn't heard of it yet
* Make JSMN JSON parsing much more robust and future-proof


Use
---

It is a work in progress, but if you want to try it out, there is a makefile, so simply running
	
	make

in the source directory will spit out a nice executable, which will open a localtunnel to whatever you happen to have running locally on port 80. The executable generated can be run as follows:

	./clocaltunneltest

This kicks off the code you see in example.c. 


Dependencies
------------

I have developed and tested clocaltunnel on OS X. It probably works on Linux as well without too much hacking. You should probably have the Xcode Command Line tools installed at the very least, if not all of Xcode. 

That being said, it depends on libcurl and pthread, which come with the OS, and libssh2, which you will have to install. I suggest using [HomeBrew][homebrew-website].

[homebrew-website]: http://mxcl.github.com/homebrew/

License
-------

Uhhh... it's a free country. I'm a mediocre programmer and an even worse lawyer. Don't come looking for me if you use this on a mission-critical system and cause millions of dollars of loss or damage, though. That's between you and your insurance company. I make no guarantee as to whether using this code will ever set your grandmother on fire.