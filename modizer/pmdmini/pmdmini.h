#ifdef __cplusplus
extern "C" {
#endif

#ifndef __PMDMINI_H__
#define __PMDMINI_H__


void pmd_init(void);
void pmd_setrate( int freq );
int pmd_is_pmd( const char *file );
int pmd_play ( const char *file , char *pcmdir );
int pmd_length_sec ( void );
int pmd_loop_sec ( void );
void pmd_renderer ( short *buf , int len );
void pmd_stop ( void );
void pmd_get_title( char *dest );
void pmd_get_compo( char *dest );
void pmd_setrate( int freq );

int pmd_get_tracks( void );
void pmd_get_current_notes ( int *notes , int len );


#endif

#ifdef __cplusplus
}
#endif
