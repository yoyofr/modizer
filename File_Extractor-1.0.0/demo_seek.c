/* Demonstrates processing files in a certain order, using seeking.
Opens "demo.zip" and first processes file of type ".1st", then ".txt",
without scanning archive twice. */

#include "fex/fex.h"

#include <stdlib.h>
#include <stdio.h>

/* Refer to demo.c for descriptions of other functions used here */
static void error( fex_err_t );
static void process_file( fex_t* );

int main( void )
{
	fex_t* fex;
	
	/* Positions of first and second files in archive. Zero isn't a
	valid position, so we can conveniently store zero as a flag that
	we haven't found the files. */
	fex_pos_t first  = 0;
	fex_pos_t second = 0;
	
	error( fex_open( &fex, "demo.zip" ) );
	
	/* Scan archive and note positions of first and second files */
	while ( !fex_done( fex ) )
	{
		if ( fex_has_extension( fex_name( fex ), ".1st" ) )
			first = fex_tell_arc( fex );
		
		if ( fex_has_extension( fex_name( fex ), ".txt" ) )
			second = fex_tell_arc( fex );
		
		error( fex_next( fex ) );
	}
	
	/* Now, process the two files in order */
	if ( first != 0 && second != 0 )
	{
		error( fex_seek_arc( fex, first ) );
		process_file( fex );
		
		error( fex_seek_arc( fex, second ) );
		process_file( fex );
	}
	
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
	
	error( fex_data( fex, &data ) );
	size = fex_size( fex );
	
	for ( i = 0; i < size && i < 100; ++i )
		putchar( ((const char*) data) [i] );
	putchar( '\n' );
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
