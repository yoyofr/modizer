/* Demonstrates scanning a directory of archives and files.
Requires dirent.h support.

Scans any supported files or archives in current directory and prints
beginnings of any ".txt" files. */

#include "fex/fex.h"

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>

/* Refer to demo.c for descriptions of other functions used here */
static void error( fex_err_t );
static void process_file( fex_t* );

static void scan_dir_or_archive( const char path [] );

int main( void )
{
	scan_dir_or_archive( "." );
	return 0;
}

static void scan_archive( const char path [] );

static void scan_dir_or_archive( const char path [] )
{
	/* For clarity this function doesn't check directory-scanning
	calls for errors. */
	
	/* Try to open as dir. If it fails, assume it's a file */
	DIR* dir = opendir( path );
	if ( dir == NULL )
	{
		scan_archive( path );
	}
	else
	{
		struct dirent* entry;
		while ( (entry = readdir( dir )) != NULL )
		{
			/* Don't scan "." and ".." entries */
			if ( strcmp( entry->d_name, "." ) != 0 &&
					strcmp( entry->d_name, ".." ) != 0 )
			{
				/* epath = path + "/" + entry->d_name */
				size_t len = strlen( path ) + 1 + strlen( entry->d_name );
				char* epath = (char*) malloc( len + 1 );
				if ( epath == NULL )
					error( "Out of memory" );
				strcpy( epath, path );
				strcat( epath, "/" );
				strcat( epath, entry->d_name );
				
				scan_dir_or_archive( epath );
				
				free( epath );
			}
		}
		closedir( dir );
	}
}

static void scan_archive( const char path [] )
{
	fex_type_t type;
	fex_t* fex;
	
	/* Determine file's type */
	error( fex_identify_file( &type, path ) );
	
	/* Only open files that fex can handle */
	if ( type != NULL )
	{
		error( fex_open_type( &fex, path, type ) );
		while ( !fex_done( fex ) )
		{
			if ( fex_has_extension( fex_name( fex ), ".txt" ) )
				process_file( fex );
			
			error( fex_next( fex ) );
		}
		fex_close( fex );
		fex = NULL;
	}
	else
	{
		printf( "Skipping unsupported archive: %s\n", path );
	}
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
