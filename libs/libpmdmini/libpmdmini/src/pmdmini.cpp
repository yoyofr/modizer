#include <iostream>
#include <cstring>
#include <cstdio>

#include "pmdwin/pmdwinimport.h"
#include "pmdmini.h"

int pmd_length = 0;
int pmd_loop = 0;

char pmd_title[1024];
char pmd_compo[1024];
char pmd_file[2048];

OPEN_WORK *pmdwork = NULL;

//
// path splitter
//

static int pmd_split_dir( const char *file , char *dir )
{
	char *p;
	int len = 0;

#ifdef _MSC_VER
	p = std::strrchr ((char*)file, '\\');
#else
	p = std::strrchr ((char*)file, '/');
#endif

	if ( p )
	{
		len = (int)( p + 1 - file );
		std::strncpy ( dir , file , len );
	}
	dir[ len ] = 0;

	return len;
}

//
// 初期化
//

void pmd_init( char *pcmdir )
{
#ifdef _MSC_VER
	char* current_dir = (char*)(".\\");
#else
	char* current_dir = (char*)("./");
#endif
	if (0 != pcmdir[0])
	{
		current_dir = pcmdir;
	}
	pmdwininit( current_dir );
	setpcmrate( SOUND_55K );

	pmdwork = NULL;

	pmd_length = 0;
	pmd_loop = 0;
}

//
//　周波数設定
//

void pmd_setrate( int freq )
{
	setpcmrate( freq );
}

//
// ファイルチェック
//

int pmd_is_pmd( const char *file )
{
	int  size;
	unsigned char header[3];

	FILE *fp;

	fp = std::fopen(file,"rb");

	if (!fp)
		return 0;

	size = (int)std::fread(header,1,3,fp);

	std::fclose(fp);

	if (size != 3)
		return 0;

	if (header[0] > 0x0f)
		return 0;

	if (header[1] != 0x18 && header[1] !=0x1a )
		return 0;

	if (header[2] && header[2] != 0xe6)
		return 0;

	return 1;
}

//
// エラーであれば0以外を返す
//

int pmd_play ( char *argv[] , char *pcmdir )
{
	char dir[2048];
	TCHAR pps_file[1024];
	TCHAR *pps_file_ptr;

	char *path[4];
#ifdef _MSC_VER
	char *current_dir = (char *)(".\\");
#else
	char *current_dir = (char *)"./";
#endif

	char *file = argv[1];
	if ( ! pmd_is_pmd ( file ) )
		return 1;

	std::strcpy( pmd_file , file );

	dir[0] = 0;
	if ( pmd_split_dir( file , dir ) > 0 )
	{
		path[0] = dir;
		path[1] = pcmdir;
		path[2] = (0 != std::strcmp( current_dir, dir )) ? current_dir : nullptr;
		path[3] = nullptr;
	}
	else
	{
		path[0] = current_dir;
		path[1] = pcmdir;
		path[2] = nullptr;
		path[3] = nullptr;
	}
	if (nullptr != path[0]) { std::cout << "path[0] " << path[0] << '\n'; }
	if (nullptr != path[1]) { std::cout << "path[1] " << path[1] << '\n'; }
	if (nullptr != path[2]) { std::cout << "path[2] " << path[2] << '\n'; }
	if (nullptr != path[3]) { std::cout << "path[3] " << path[3] << '\n'; }

	setpcmdir( path );

	// get song length in sec
	if ( !getlength( pmd_file , &pmd_length , &pmd_loop ) )
	{
		pmd_length = 0;
		pmd_loop = 0;
	}

	pmd_title[0] = 0;
	pmd_compo[0] = 0;
	pps_file[0] = 0;

	music_load( pmd_file );

	pps_file_ptr = getppsfilename( pps_file );

	if (nullptr != pps_file_ptr)
	{
		if ( (nullptr != argv[2]) && (0 == pps_file_ptr[0]) )
		{
			if (nullptr != argv[3])
			{
				if ( (45 /* minus sign */ != argv[3][0]) && (45 /* minus sign */ != argv[3][1]) )
				{
					char *p = nullptr;

#ifdef _MSC_VER
					p = strrchr( argv[3], ':' );
					if ( nullptr == p )
					{
						p = strrchr( argv[3], '\\' );
					}
#else
					p = strrchr( argv[3], '/' );
#endif

					if ( nullptr != p )
					{
						pps_file_ptr = (TCHAR *)argv[3];
						std::cout << "pps_file_ptr " << pps_file_ptr << '\n';
						if ( PMDWIN_OK == ppc_load( pps_file_ptr ) )
						{
							std::cout << "PMDWIN_OK == ppc_load( " << pps_file_ptr << " )\n";
						}
						else
						{
							std::cout << "PMDWIN_OK != ppc_load( " << pps_file_ptr << " )\n";
						}
					}
					else
					{
						if (0 != std::strcmp( current_dir, path[0] ))
						{
							p = std::strcat( pps_file_ptr, path[0] );
						}
						else
						{
							p = std::strcat( pps_file_ptr, current_dir );
						}
						p = std::strcat( pps_file_ptr, argv[3] );
						std::cout << "pps_file_ptr " << pps_file_ptr << '\n';

						if ( PMDWIN_OK == ppc_load( pps_file_ptr ) )
						{
							std::cout << "PMDWIN_OK == ppc_load( " << pps_file_ptr << " )\n";
						}
						else
						{
							std::cout << "PMDWIN_OK != ppc_load( " << pps_file_ptr << " )\n";
						}
					}
				}
			}
		}
	}

	fgetmemo3( pmd_title, pmd_file, 1 );
	fgetmemo3( pmd_compo, pmd_file, 2 );

	setrhythmwithssgeffect( true ); // true == SSG+RHY, false == SSG

	if ( nullptr != pps_file_ptr )
	{
		if ( 0 != pps_file_ptr[0] )
		{
			setppsuse( true ); // PSSDRV FLAG set false at init. true == use PPS, false == do not use PPS
		}
	}

	music_start();

	pmdwork = getopenwork();

	return 0;
}

//
// トラック数
//
int pmd_get_tracks( void )
{
	return NumOfAllPart;
}

//
// 現在のノート
//
void pmd_get_current_notes ( int *notes , int len )
{
	int i = 0;

	for ( i = 0; i < len; i++ ) notes[i] = -1;

	if ( ! pmdwork )
		return;

	for ( i = 0; i < len ; i++ )
	{
		int data = pmdwork->MusPart[i]->onkai;

		if (data == 0xff)
			notes[i] = -1;
		else
			notes[i] = data;
	}
}

int pmd_length_sec ( void )
{
	return pmd_length / 1000;
}

int pmd_loop_sec ( void )
{
	return pmd_loop / 1000;
}
//YOYOFR
int pmd_loop_msec ( void )
{
    return pmd_loop;
}
int pmd_length_msec ( void )
{
    return pmd_length;
}
//YOYOFR
void pmd_renderer ( short *buf , int len )
{
	getpcmdata ( buf , len );
}

void pmd_stop ( void )
{
	music_stop();
	pmdwork = NULL;
}

void pmd_get_title( char *dest )
{
	std::strcpy( dest , pmd_title );
}

void pmd_get_compo( char *dest )
{
	std::strcpy( dest , pmd_compo );
}

