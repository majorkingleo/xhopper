# xhopper 
a portforwarding tool

You are searching for some docu? The best docu you will get about
xhopper is the help screen:

	./xhopper -h

Compression is supported by zlib, which is shipped with this
packages. I know this is not the newest release, but I'm trying
getting the stuff compiled on every type of unix machine and even win32.

Ftp proxy is supported by a stripped down version of ftpproxy.

What works, and what not:

1. File copying seems to be broken on HP-UX or every where.
2. FTP-Proxy does automatically forward all to port 21 on the
   target machine and cannot by changed from command line. This
   will be fixed in future releases.

**Why C?**

Cause I'm trying getting the stuff compiled on every type of unix
machine.

**Why not automake/autoconf?**

Cause I'm trying getting the stuff compiled on every type of unix
machine.

**Sponsors:**

Don't wanna see it? Just compile with:

	CFLAGS="-DNO_AD" ./configure

**ioconv:**

ioconv is a small tool that codes binary data into ascii
format and vice versa. It's a simple replacement for uuencode
and uudecode. 

The source code itself is so small that you can copy and paste
it easy to the target machine.

**Scenario:**

You wanna open an xterm on a target machine which is
located in a network which does not allow you opening
a connection back to your machine. So that's a case
for xhopper and the reverse mode (./xhopper -h for
more information about the reverse mode).
On this target machine is no xhopper installed,	but
you can access this machine via ssh, or telnet or
something else.

1. Open an vi on the target machine.
2. Open the source code of ioconv on your machine.
3. Copy & paste the sourcecode of ioconv to the target machine.
4. compile ioconv on the target machine:

		   cc -o ioconv ioconv.c
		   
5. convert xhopper to ascii data:

		   ioconv in < xhopper.tar.gz > xhopper.dat
		   
6. open a vi on the target machine.
7. open a vi on our machine.      
8. Copy & paste the data to the target machine.
9. decode xhopper.dat on the target machine:

		ioconv out < xhopper.dat > xhopper.tar.gz
		
10. compile xhopper on the target machine.   
	
