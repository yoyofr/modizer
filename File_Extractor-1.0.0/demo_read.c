/* Demonstrates reading file into your own block of memory.

Opens "demo.zip", reads the first ".txt" file into our structure,
reading the header separately from the rest of the data.
It then closes the archive and prints the header and contents. */

#include "fex/fex.h"

#include <stdlib.h>
#include <stdio.h>

/* Refer to demo.c for descriptions of other functions used here */
static void error( fex_err_t );

/* Example structure to read into */
enum { header_size = 16 };
struct myfile_t
{
	char header [header_size];  /* First 16 bytes of file */
	char* data;                 /* Pointer to remaining data */
	int data_size;              /* Size of remaining data */
};

int main( void )
{
	fex_t* fex;
	struct myfile_t myfile;
	myfile.data = NULL;
	
	error( fex_open( &fex, "demo.zip" ) );
	
	/* Scan archive headers */
	while ( !fex_done( fex ) )
	{
		if ( fex_has_extension( fex_name( fex ), ".txt" ) )
		{
			/* Must be called before using fex_size() */
			error( fex_stat( fex ) );
			
			/* Get file size and allocate space for it */
			myfile.data_size = fex_size( fex ) - header_size;
			myfile.data = (char*) malloc( myfile.data_size );
			if ( myfile.data == NULL )
				error( "Out of memory" );
			
			/* Read header */
			error( fex_read( fex, myfile.header, header_size ) );
			
			/* Read rest of data */
			error( fex_read( fex, myfile.data, myfile.data_size ) );
			
			break;
		}
		
		error( fex_next( fex ) );
	}
	
	fex_close( fex );
	fex = NULL;

	/* If file was read, print header and first 100 characters of data */
	if ( myfile.data != NULL )
	{
		int i;
		
		printf( "Header: " );
		for ( i = 0; i < header_size; ++i )
			putchar( myfile.header [i] );
		
		printf( "\nData: " );
		for ( i = 0; i < myfile.data_size && i < 100; ++i )
			putchar( myfile.data [i] );
		printf( "\n" );
		
		free( myfile.data );
	}
	
	return 0;
}

static void error( fex_err_t err )
{
	if ( err != NULL )
	{
		const char* str = fex_err_str( err );
		fprintf( stderr, "Error: %s\n", str );
		exit( EXIT_FAILURE );
	}
}
