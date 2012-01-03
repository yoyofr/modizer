/* Demonstrates using rewind to re-scan archive.
Opens "demo.zip", lists files, asks user which to print,
then prints the one indicated.

Note that you could instead build an array of fex_pos_t values
for each scanned file, then seek directly to the selected file.
This might be faster if the archive had thousands of files,
or you were extracting multiple files at arbitrary positions. */

#include "fex/fex.h"

#include <stdlib.h>
#include <stdio.h>

/* Refer to demo.c for descriptions of other functions used here */
static void error( fex_err_t );
static void process_file( fex_t* );

int main( void )
{
	fex_t* fex;
	int index;
	int selection;
	
	error( fex_open( &fex, "demo.zip" ) );
	
	/* List archive */
	index = 0;
	while ( !fex_done( fex ) )
	{
		if ( fex_has_extension( fex_name( fex ), ".txt" ) )
		{
			index++;
			printf( "%d: %s\n", index, fex_name( fex ) );
		}
		
		error( fex_next( fex ) );
	}
	
	/* Get choice from user */
	do
	{
		printf( "Which file number? " );
		fflush( stdin );
		selection = 0;
	}
	while ( scanf( "%d", &selection ) != 1 || selection <= 0 || selection > index );
	printf( "\n" );
	
	/* Go back to first file in archive */
	error( fex_rewind( fex ) );
	
	/* Re-scan archive until selected file is reached */
	index = 0;
	while ( !fex_done( fex ) )
	{
		if ( fex_has_extension( fex_name( fex ), ".txt" ) )
		{
			index++;
			if ( index == selection )
			{
				process_file( fex );
				break;
			}
		}
		
		error( fex_next( fex ) );
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
