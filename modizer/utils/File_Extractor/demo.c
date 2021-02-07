/* Demonstrates basic archive scanning and extraction.
Opens "demo.zip" and prints first 100 characters of text files. */

#include "fex/fex.h"

#include <stdlib.h>
#include <stdio.h>

/* If fex error occurred, prints it and exits program, otherwise just returns */
static void error( fex_err_t );

static void process_file( fex_t* );

int main( void )
{
	fex_t* fex;
	
	error( fex_open( &fex, "demo.zip" ) );
	
	/* Scan archive and process any text files */
	while ( !fex_done( fex ) )
	{
		if ( fex_has_extension( fex_name( fex ), ".txt" ) )
			process_file( fex );
		
		error( fex_next( fex ) );
	}
	
	/* Close archive and null pointer to avoid accidental use */
	fex_close( fex );
	fex = NULL;

	return 0;
}

static void process_file( fex_t* fex )
{
	const void* data;
	int size;
	int i;
	
	printf( "Processing %s\n", fex_name( fex ) );
	
	/* Get pointer to extracted data. Fex will automatically free this later. */
	error( fex_data( fex, &data ) );
	size = fex_size( fex );
	
	/* Print first 100 characters */
	for ( i = 0; i < size && i < 100; ++i )
		putchar( ((const char*) data) [i] );
	putchar( '\n' );
}

static void error( fex_err_t err )
{
	if ( err != NULL )
	{
		/* Get error string for err */
		const char* str = fex_err_str( err );
		
		fprintf( stderr, "Error: %s\n", str );
		exit( EXIT_FAILURE );
	}
}
