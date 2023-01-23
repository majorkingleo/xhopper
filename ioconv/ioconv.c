#include <stdio.h>
#include <string.h>

static void conv_in( void ) {
  int c, count = 0; while( ( c = getc( stdin ) ) != EOF ) {
    printf( "%02X", (unsigned)c ); 
    if( count++ > 75 / 3 ) { printf( "\n" ); count = 0; }        
  } printf( "\n" );  
}

static int x_getline( char *buf ) {
  int count = 0, c; while( ( c = getc( stdin ) ) != EOF ) {  
    if( c == '\n' ) { *buf = '\0'; return 1; }  *buf = c; buf++; count++; }
  *buf = '\0'; return count > 0;
}

static void conv_out( void ) {
  char buffer[85], buf[4]; while( x_getline( buffer ) ) {
  int i; char *b = buffer; while( *b != '\0' ) {
  int c; char cc; for( i = 0; i < 2; i++, b++ ) buf[i] = *b;
  buf[i] = '\0'; sscanf( buf, "%X", &c ); cc = (char) c;  
  fwrite( &cc, sizeof( cc ), 1, stdout ); } }
}

int main( int argc, char **argv ) {
  if( argc < 2 ) { printf( "usage: ioconv [in|out]\n" ); return -1; }
  if( strcmp( argv[1], "in" ) == 0 ) conv_in(); else conv_out();
  return 0;
}
