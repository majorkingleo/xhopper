/*
    xhopper - A simple portforwarding software
    Copyright (C) 2004 - 2005 by Martin Oberzalek kingleo@gmx.at

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "utils.h"
#include "xhopper2.h"
#include "proxy.h"
#include "copy.h"
#include "reverse.h"
#include "tel.h"
#include "ftp.h"

extern int errno;

struct _SETUP SETUP;

static void help( void );
static void usage( int ret );

void catch_alarm( int signal )
{
  printf( "timeout reached, killing all chields\n" );
  kill( 0, SIGTERM );
}

static void usage( int ret )
{
  printf( "\n" );
  printf( "xhopper FLAGS MODE OPTIONS\n" );
  printf( "FLAGS:\n" );
  printf( "\t-ci\tcompress in\n" );
  printf( "\t-co\tcompress out\n" );
  printf( "\t-d\tdebug\n" );
  printf( "\t-t MIN\ttimout\n" );
  printf( "\t-h\tprint advanced help screen\n" );
  printf( "\t-help-ioconv\tprint advanced help screen\n" );
  printf( "\t-help-proxy\tprint advanced help screen\n" );
  printf( "\t-help-reverse\tprint advanced help screen\n" );
  printf( "\n" );
  printf( "MODE: proxy\n" );
  printf( "\txhopper proxy IP-ADDRESS PORT PORT [SLEEP_TIME]\n" );
  printf( "\n" );
  printf( "MODE: copy (should work again)\n" );
  printf( "\txhopper copy LISTEN-PORT\n" );
  printf( "\txhopper copy IP-ADDRESS PORT FILENAME\n" );
  printf( "\n" );
  printf( "MODE: reverse\n" );
  printf( "\txhopper reverse CONTROL-PORT LISTEN-PORT\n" );
  printf( "\txhopper reverse CLIENT-IP PORT SERVER-IP PORT\n" );
  printf( "\n" );
  printf( "MODE: proxy_tel\n" );
  printf( "\txhopper proxy_tel IP-ADDRESS PORT LISTEN-PORT\n" );
  printf( "\txhopper proxy_tel IP-ADDRESS PORT LISTEN-PORT continues\n" );
  printf( "\n" );
  printf( "MODE: proxy_ftp\n" );
  printf( "\txhopper proxy_ftp IP-ADDRESS PORT PORT\n" );
  printf( "\n" );

  if( ret >= 0 )
  {
#ifdef WIN32
	sleep( 60 );
#endif
    exit( ret );
  }

}

static void help_ioconv( void )
{
  printf( "\n"
	  "HELP IOCONV:\n" );

  printf( "\n"
	  "If you require xhopper or any other stuff on a host (named Bimbo in\n"
	  "this description) where no ftp server is running, but you can open\n"	  
	  "an editor there you can get xhopper on this host using following way:\n\n"
	  "1) copy and paste the sourcecode of ioconv (ioconv.c) to the host Bimbo.\n\n"
	  "2) Compile it there: eg: cc -o ioconv ioconv.c\n\n"
	  "3) encode your file with ioconv\n"
	  "   \t ioconv in < xhopper.tar.gz > foo\n"
	  "   ioconv encodes any file in text format. ioconv is shipped with xhopper\n\n" );
  printf(
	  "4) Open now file foo with a text editor and mark all and copy it into the\n"
	  "   clipboard.\n\n"
	  "5) Open an editor on the host Bimbo (example vi) and go into insert mode.\n\n"
	  "6) Paste all in your shell window. (The one for the host Bimbo.)\n\n"
	  "7) Save the file on the host. Eg: :write foo\n\n"
	  "8) decode the file with ioconv\n"
	  "   \tioconv out < foo > xhopper.tar.gz\n\n" );

  printf( "If the host Bimbo is a Unix like machine then there should be the uudecode\n"
	  "and uuencode commands. Which are doing the same as ioconv, but more efficent.\n" );
}

static void help_proxy( void )
{
  printf( "\n"
	  "HELP PROXY:\n" );
  
  printf( "\n"
	  "If you have 3 hosts...\n"
	  "\t[A]------[B]-------[C]\n"
	  "And host C cannot be accessed directly, but you wan't to open an xterm...\n"
	  "The xterm is the client application which wan'ts to connect a X Server\n"
	  "over a TCP/IP connection. The X Server is running on your PC or what\n"
	  "the hell you have. That's the reason why you have to set the DISPLAY\n"
	  "variable on the host where you wan't to open a xterm.\n\n"
	  "\tIS THIS CLEAR NOW ?????\n\n"
	  "So you have to start a xhopper on the host B which directs the xterm requests\n"
	  "to your X Server.\n" );
  printf(
	  "So bring xhopper to the host B (using ftp, or ioconv (xhopper -help-ioconv))\n"
	  "and start it there\n"
	  "\t xhopper proxy A 6000 6000\n"
	  "This tells xhopper to direct all request to host A on port 6000.\n"
	  "Port 6000 is display 0. And xhopper should listen on port 6000.\n"
	  "If xhopper is startet you have to set the display variable on host C.\n"
	  "\t setenv DISPLAY B:0\n"
	  "or if you not using csh\n" );

  printf(
	  "\t export DISPLAY=B:0\n\n" 
	  "Now you can open the xterm.\n\n"
	  "If you wan't to access a printserver using http, then the client\n"
	  "is your webbrowser and the server is the embedded webserver on the\n"
	  "printserver. So you have to start xhopper this ways:\n"
	  "\t xhopper proxy C 80 80\n"
	  "\n" );

  printf( 
	  "If host B is an unix like machine, this will require root privileges.\n"
	  "If you don't have them just let xhopper listen on a port > 80.\n"
	  "\t xhopper proxy C 80 8000\n"
	  "Now you can access host C by using the url: http://B:8000\n\n"
	  );
}

static void help_reverse( void )
{
  printf( "\n"
	  "HELP REVERSE:\n" );

  printf( "\n"
	  "Exmaple:\n"
	  "\t [A]------[B]-----[C]\n\n"
	  "You wan't to access a webserver on host C, but the firewall between\n"
	  "A and B doesn't let you use xhopper in proxy mode on host B.\n"
	  "Any other port than port 80 is in use and the firewall doesn't allow\n"
	  "you using other port's.\n"
	  "But you can access one port from host B to host A.\n"
	  "So bring xhopper to host B (ftp, or ioconv (-help-ioconv)).\n"
	  "Start xhopper on host A in reverse mode\n"
	  "\t xhopper reverse 3000 4000\n"
	  );

  printf(
	  "This tells xhopper to wait for a control connection on port 3000 and\n"
	  "listen on port 4000 for normal incoming connections.\n"
	  "Start xhopper on host B\n"
	  "\t xhopper reverse A 3000 C 80\n"
	  "This tells xhopper that he should contact the xhopper on host A\n"
	  "on port 3000 and he sould forward all incoming connections to host\n"
	  "C on port 80.\n"
	  "Now you can use the url: http://A:4000 to access the webservice on\n"
	  "host C.\n\n"
	  );

  printf(
	  "How the magic works:\n"
	  "\t You use the url http://A:4000, the xhopper on host A is listening\n"
	  "\t on this port and tells the xhopper on host B over the control\n"
	  "\t connection that he has an incoming request.\n"
	  "\t The xhopper on host B opens a data connection to host A on port 3000\n"
	  "\t and a connection to host C on port 80.\n"
	  "\t That's the magic.\n"
	  "\t The only thing you have to be sure is, that you can access the\n"
	  "\t port 3000 of the host A from the host B.\n" 
	  "\n"
	  );
}

static void help( void )
{
  printf( "\n"
	  "HELP:\n"
	  "  compression:\n"
	  "\t If compression is turned on a gzip compression will be done\n"
	  "\t on any input or output. Only in the 'proxy' mode compression\n"
	  "\t is optional. In any other case the communication partner is\n"
	  "\t also xhopper so compression can be done.\n" 
	  "\n" );

  printf( "  proxy mode:\n"
	  "\t In proxy mode xhopper is listening on a port, and if an application\n"
	  "\t creates a connection to this port, xhopper will forward all data\n"
	  "\t to a given server and port. That's a simple port forwarding.\n"
	  "\t To use xhopper for opening a xterm, xhopper has to be started on\n"
	  "\t a host which can be reached from both sides. The host where you\n"
	  "\t wan't to run the xterm and the the host where you wan't to display\n"
	  "\t the xterm.\n" );

  printf( "\t eg: Start xhopper on the server in the middel this way:\n"
	  "\t\t xhopper YOUR-IP 6000 6001\n"
	  "\t this will let xhopper listen on port 6001 for incoming connections.\n"
	  "\t This will be the XDisplay 1. Port 6000 is the XDisplay 0.\n"
	  "\t Then start the xterm this way:\n"
	  "\t\t xterm -display MIDDLE-IP:1\n"
	  "\t -help-proxy gives more info.\n"
	  "\n" );

  printf( "  proxy_tel mode:\n"
	  "\t Works as proxy mode, but assumes that you wanna start X programms on\n"
	  "\t the connected host. So xhopper creates a back connection and forwards\n"
	  "\t all X connection back to you. It will send a message what DISPLAY is\n"
	  "\t to set. This works with telnet well, but may won't work with ssh or\n"
	  "\t any other type of connection.\n"
	  "\t If the 'continues' option is set, xhopper listen's on the next 10 ports\n"
	  "\t for incomming connections too, and forwards the back connections to a\n"
	  "\t higher X11 port.\n"
	  "\t Eg: If LISTEN-PORT is 3000 and you connect to 3001 xhopper will install\n"
	  "\t     the back connection to 6001. This is display 1.\n"
	  "\n" );

  printf( "  proxy_ftp mode:\n"
	  "\t creates a ftp proxy which is listening on a specific port and directs\n"
	  "\t all incoming ftp connections to an other port.\n"
	  "\n" );

  printf( "  copy mode:\n"
	  "\t In copy mode you can send files via xhopper. Simple start xhopper on\n"
	  "\t the server in copy mode and tell xhopper on which port it should\n"
	  "\t listen.\n"
	  "\t\t xhopper 3000\n"
	  "\t Now xhopper is listening for files on port 3000. Sending a file\n"
	  "\t to this host can be done this way:\n"
	  "\t\t xhopper SERVER-IP 3000 foo.txt\n"
	  "\t Note: The content of the file will be compressed and uncompressed\n"
	  "\t automatically.\n"
	  "\n" );

  printf( "  reverse mode:\n"
	  "\t If you have a host where you cannot get a connection outside cause\n"
	  "\t of a firewall, but incoming connections are working, the reverse mode\n"
	  "\t will be the solution.\n"
	  "\t\t xhopper reverse 3000 6001\n"
	  "\t Now xhopper is listening on host FOO on port 3000 for a control\n"
	  "\t connection of an other xhopper. The other xhopper can be startet with\n"
	  "\t this command.\n" 
	  );

  printf( "\t\t xhopper reverse FOO 3000 BAR 6000\n"
	  "\t This xhopper will open the control connection and will forward all\n"
	  "\t incoming 'normal' connections to the host BAR on port 6000.\n"
	  "\t If on host FOO a xterm will be started with the command\n"
	  "\t\t xterm -display localhost:1\n"
	  "\t the xhopper that is running on host FOO will request a new\n"
	  "\t channel over the control channel from the xhopper that is running\n"
	  "\t on host BAR.\n"
	  "\t -help-reverse gives more info.\n"
	  "\n"
	  );
}

int main( int argc, char **argv )
{
  int i = 0;

  printf( "\txhopper version " VERSION ",\n"
	  "\tCopyright (C) 2004-2005 Martin Oberzalek kingleo@gmx.at\n"
	  "\txhopper comes with ABSOLUTELY NO WARRANTY; \n"
	  "\tThis is free software, and you are welcome to redistribute it\n"
	  "\tunder certain conditions; Hove a look at COPYING for details.\n\n"
	  "\tSpecial thanks to ASI for enough beer which was really needed!\n\n"
	  "\tNOTE: 6000\t X11\n"
	  "\t        23\t telnet\n"
	  "\t      2401\t cvspserver\n"
	  "\t        22\t ssh\n"
	  "\t        80\t webservices\n"
	  "%s\n",
	  get_ad() );

  if( sizeof( int ) != 4 )
    printf( "WARNING: sizeof(int) != 4 => header size will be not correct\n" );

  memset( &SETUP, 0, sizeof( SETUP ) );

  for( i = 1; i < argc; i++ )
    {
      if( strcmp( argv[i], "proxy" ) == 0 )
	{
	  SETUP.mode |= MODE_PURE_PROXY;
	  
	  if( argc < i + 3 )
	    {
	      printf( "invalid number of arguments\n" );
	      usage( RETURN_PARSE_ERROR );
	    }

	  if( !is_int( argv[i+2] ) || !is_int( argv[i+3] ) )
	    {
	      printf( "ERROR: port number are expected as integer ( %s %s )\n", argv[i+2], argv[i+3] );
	      usage( RETURN_PARSE_ERROR );
	    }

	  strncpy( SETUP.u.proxy.server_name, argv[i+1], SERVER_BUF_LEN );

	  sscanf( argv[i+2], "%d", &SETUP.u.proxy.server_port );
	  sscanf( argv[i+3], "%d", &SETUP.u.proxy.listen_port );

	  if( argc > i + 4 && is_int( argv[i+4] ) )
	    {
	  		sscanf( argv[i+4], "%d", &SETUP.u.proxy.wait_time );
			printf( "WAIT TIME set to: %d\n", SETUP.u.proxy.wait_time );
			i++;
        }

	  i += 4;

	}
      else if( strcmp( argv[i], "proxy_tel" ) == 0 )
	{
	  SETUP.mode |= MODE_PROXY_TEL;
	  
	  if( argc < i + 3 )
	    {
	      printf( "invalid number of arguments\n" );
	      usage( RETURN_PARSE_ERROR );
	    }

	  if( !is_int( argv[i+2] ) || !is_int( argv[i+3] ) )
	    {
	      printf( "ERROR: port number are expected as integer ( %s %s )\n", argv[i+2], argv[i+3] );
	      usage( RETURN_PARSE_ERROR );
	    }

	  strncpy( SETUP.u.proxy_tel.server_name, argv[i+1], SERVER_BUF_LEN );

	  sscanf( argv[i+2], "%d", &SETUP.u.proxy_tel.server_port );
	  sscanf( argv[i+3], "%d", &SETUP.u.proxy_tel.listen_port );

	  if( argc > i + 4 )
	  {
	      if( strcmp( argv[i+4], "continues" ) == 0 )
	      {
		  SETUP.u.proxy_tel.continues = 1;
		  i++;
	      } else {
		  SETUP.u.proxy_tel.continues = 0;
		  printf( "expecting 'continues' option\n" );
		  usage( RETURN_PARSE_ERROR );
	      }
	  }

	  i += 4;
	}
      else if( strcmp( argv[i], "proxy_ftp" ) == 0 )
	{
	  SETUP.mode |= MODE_PROXY_FTP;
	  
	  if( argc < i + 3 )
	    {
	      printf( "invalid number of arguments\n" );
	      usage( RETURN_PARSE_ERROR );
	    }

	  if( !is_int( argv[i+2] ) || !is_int( argv[i+3] ) )
	    {
	      printf( "ERROR: port number are expected as integer ( %s %s )\n", argv[i+2], argv[i+3] );
	      usage( RETURN_PARSE_ERROR );
	    }

	  strncpy( SETUP.u.proxy_ftp.server_name, argv[i+1], SERVER_BUF_LEN );

	  sscanf( argv[i+2], "%d", &SETUP.u.proxy_ftp.server_port );
	  sscanf( argv[i+3], "%d", &SETUP.u.proxy_ftp.listen_port );

	  i += 4;

	} else if( strcmp( argv[i], "-h" ) == 0 ) {

	  usage( -1 );
	  help();
	  
	  return RETURN_SUCCESS;

	} else if( strcmp( argv[i], "-help-ioconv" ) == 0 ) {

	  help_ioconv();
	  
	  return RETURN_SUCCESS;

	} else if( strcmp( argv[i], "-help-reverse" ) == 0 ) {

	  help_reverse();
	  
	  return RETURN_SUCCESS;


	} else if( strcmp( argv[i], "-help-proxy" ) == 0 ) {

	  help_proxy();
	  
	  return RETURN_SUCCESS;

	} else if( strcmp( argv[i], "-ci" ) == 0 ) {

	  SETUP.mode |= MODE_COMPRESS;

	} else if( strcmp( argv[i], "-co" ) == 0 ) {

	  SETUP.mode |= MODE_UNCOMPRESS;

	} else if( strcmp( argv[i], "-d" ) == 0 ) {

	  SETUP.mode |= MODE_DEBUG;

	} else if( strcmp( argv[i], "-t" ) == 0 ) {

	  SETUP.mode |= MODE_TIMEOUT;
	  
	  if( argc < i + 1 )
	    {
	      printf( "missing timeout value\n" );
	      usage( RETURN_PARSE_ERROR );
	    }

	  if( !is_int( argv[i+1] ) )
	  {
	    printf( "timeout value '%s' not an inter number\n", argv[i+1] );
	    usage( RETURN_PARSE_ERROR );
	  }

	  sscanf( argv[i+1], "%d", &SETUP.timeout ); 

	  i += 1;

	} else if( strcmp( argv[i], "copy" ) == 0 ) {

	  if( argc <= i + 1 )
	    {
	      printf( "missing port\n" );
	      usage( RETURN_PARSE_ERROR );
	    }
 
	  if( is_int( argv[i+1] ) )
	    {
	      SETUP.mode |= MODE_FILETRANSFER;
	      SETUP.mode |= MODE_SERVER;
	      SETUP.mode |= MODE_UNCOMPRESS;
	      sscanf( argv[i+1], "%d", &SETUP.u.copy_server.listen_port );
	      i += 1;
	      continue;
	    }

	  if( argc < i + 3 )
	    {
	      printf( "missing server name and/or port\n" );
	      usage( RETURN_PARSE_ERROR );
	    }
   
	  if( !is_int( argv[i+2] ) )
	    {
	      printf( "port has to be an integer\n" );
	      usage( RETURN_PARSE_ERROR );
	    }

	  SETUP.mode |= MODE_FILETRANSFER;
	  SETUP.mode |= MODE_CLIENT;
	  SETUP.mode |= MODE_COMPRESS;
	  strcpy( SETUP.u.copy_client.server_name, argv[i+1] );
	  sscanf( argv[i+2], "%d", &SETUP.u.copy_client.server_port );
	  strcpy( SETUP.u.copy_client.file_name, argv[i+3] );	  

	  i+= 3;

	} else if( strcmp( argv[i], "reverse" ) == 0 ) {
	  
	  if( argc < i + 3 )
	    {
	      printf( "missing port\n" );
	      usage( RETURN_PARSE_ERROR );
	    }
	  
	  if( is_int( argv[i+1] ) && is_int( argv[i+2] ) )
	    {
	      SETUP.mode |= MODE_REVERSECONTACT;
	      SETUP.mode |= MODE_CLIENT;
	      SETUP.mode |= MODE_COMPRESS;
	      sscanf( argv[i+1], "%d", &SETUP.u.reverse_client.control_port );
	      sscanf( argv[i+2], "%d", &SETUP.u.reverse_client.listen_port );
	      i += 2;
	      continue;
	    }
	  
	  if( argc < i + 5 )
	    {
	      printf( "missing server name and/or port\n" );
	      usage( RETURN_PARSE_ERROR );
	    }
	  
	  if( !is_int( argv[i+2] ) || !is_int( argv[i+4] )  )
	    {
	      printf( "port has to be an integer\n" );
	      usage( RETURN_PARSE_ERROR );
	    }

	  SETUP.mode |= MODE_REVERSECONTACT;
	  SETUP.mode |= MODE_SERVER;
	  SETUP.mode |= MODE_UNCOMPRESS;

	  strcpy( SETUP.u.reverse_server.client_name, argv[i+1] );
	  sscanf( argv[i+2], "%d", &SETUP.u.reverse_server.client_port );	  

	  strcpy( SETUP.u.reverse_server.server_name, argv[i+3] );
	  sscanf( argv[i+4], "%d", &SETUP.u.reverse_server.server_port );

	  i+= 5;

	} else {
	  printf( "unknown arg number: %d => %s\n", i, argv[i] );
	  usage( RETURN_PARSE_ERROR ); 
	}
    }

  if( SETUP.mode == MODE_NONE )
    {
      printf( "ERROR: no mode selected\n" );
      usage( RETURN_PARSE_ERROR );
    }

  if( SETUP.mode & MODE_COMPRESS && SETUP.mode & MODE_UNCOMPRESS )
    {
      printf( "ERROR: invalid compression mode\n" );
      usage( RETURN_PARSE_ERROR );
    }

  if( SETUP.mode & MODE_TIMEOUT )
    {
      signal( SIGALRM, catch_alarm );
      alarm( SETUP.timeout * 60 );
      printf( "installed alarm timout, terminating in %d seconds\n",
	      SETUP.timeout * 60 );
    }

  if( SETUP.mode & MODE_PURE_PROXY )
    {
      return create_pure_proxy();
    }
  else if( SETUP.mode & MODE_PROXY_TEL )
    {
      return create_proxy_tel();
    }
  else if( SETUP.mode & MODE_PROXY_FTP )
    {
      return create_proxy_ftp();

    } else if( SETUP.mode & MODE_FILETRANSFER ) {

      if( SETUP.mode & MODE_SERVER )
	{
	  return create_file_server();
	} else {
	  return copy_file();
	}

    } else if( SETUP.mode & MODE_REVERSECONTACT ) {

      if( SETUP.mode & MODE_CLIENT )
	{
	  return create_reverse_client();
	} else {
	  return create_reverse_server();
	}

    } else {

      printf( "ERROR: no mode selected\n" );
      usage( RETURN_PARSE_ERROR );

    }

  return RETURN_SUCCESS;
}

char *get_ad( void )
{
#ifndef NO_AD
  static char buffer[1024] = {0};


  if( buffer[0] == '\0' )
    {
      /* The \r\n is required to get a nice add on putty 
	 when using the proxy_tel mode. 
      */
      strcat( buffer, 
	      "\r\nxhopper is sponsered by:\r\n"
	      "                              .------.\r\n"
	      "                            :|||\"\"\"`.`.\r\n"
	      "                            :|||     7.`.\r\n"
	      "         .===+===+===+===+===||`----L7'-`7`---.._\r\n"
	      "         []                  || ==       |       \"\"\"-.\r\n" );
      strcat( buffer, 
	      "         []...._____.........||........../ _____ ____|\r\n"
	      "        c\\____/,---.\\_       ||_________/ /,---.\\_  _/\r\n"
	      "          /_,-/ ,-. \\ `._____|__________||/ ,-. \\ \\_[\r\n"
	      "             /\\ `-' /                    /\\ `-' /\r\n"
	      "               `---'                       `---'\r\n"
	      "\thttp://www.austromobil.at\tGenieﬂen Sie die Fahrt!\r\n" );

    }

  return buffer;
#else
  return "";
#endif
}

#ifdef NEED_SETENV
int setenv(const char *name, const char *value, int overwrite)
{
    char *buffer = NULL;
    int  ret;

    if( overwrite != 1 )
    {
	char *erg = getenv( name );

	if( erg != NULL )
	    return -1;
    }

    buffer = malloc( strlen( name ) + strlen( value ) + 2 );

    sprintf( buffer, "%s=%s", name, value );

    ret = putenv( buffer );

    return ret;
}
#endif
