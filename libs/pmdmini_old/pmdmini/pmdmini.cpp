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
		len = (int)( p - file );
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

	std::strcpy ( pmd_file , file );

	dir[0] = 0;
	if ( pmd_split_dir( file , dir ) > 0 )
	{
		path[0] = dir;
		path[1] = pcmdir;
		path[2] = current_dir;
		path[3] = nullptr;
	}
	else
	{
		path[0] = current_dir;
		path[1] = pcmdir;
		path[2] = nullptr;
		path[3] = nullptr;
	}

	setpcmdir( path );

	// get song length in sec
	if (!getlength( pmd_file , &pmd_length , &pmd_loop ))
	{
		pmd_length = 0;
		pmd_loop = 0;
	}

	pmd_title[0] = 0;
	pmd_compo[0] = 0;
	pps_file[0] = 0;

	music_load( pmd_file );

	pps_file_ptr = getppsfilename( pps_file );

	if ( ( nullptr != pps_file_ptr ) && ( 0 == pps_file_ptr[0] ) && ( nullptr != argv[2] ) )
	{
		if ( ( nullptr != argv[3] ) && (45 /* minus sign */ != argv[3][0] ) && (45 /* minus sign */ != argv[3][1] ) )
		{
			char *p;

#ifdef _MSC_VER
			p = strrchr( argv[3], ':' );
#else
			p = ('/' == argv[3][0] ) ? argv[3] : nullptr;
#endif
			if ( nullptr == p )
			{
				p = ('.' == argv[3][0]) ? argv[3] : nullptr;
			}

			if ( nullptr != p )
			{
				if ( PMDWIN_OK == ppc_load( argv[3] ) )
				{
					pps_file_ptr = argv[3];
				}
			}
			else
			{
				pps_file[0] = 0;
				int len = std::strlen ( dir );
				p = std::strcpy ( pps_file, dir);
				p = std::strcpy(&pps_file[len], &current_dir[1]);
				p = std::strcpy(&pps_file[len + 1], argv[3]);

				if ( PMDWIN_OK == ppc_load( pps_file ) )
				{
					pps_file_ptr = pps_file;
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

int pmd_length_msec ( void )
{
    return pmd_length;
}


int pmd_loop_sec ( void )
{
	return pmd_loop / 1000;
}

int pmd_loop_msec ( void )
{
    return pmd_loop;
}

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

