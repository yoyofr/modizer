#include <cstdlib> /* for atoi() */
#include <cstring> /* for memset(), memcpy() */
#include <cstdint>
#include <climits> /* for INT_MAX */
#include <csignal>
#ifdef _MSC_VER
#define SDL_MAIN_HANDLED
#include <SDL.h>
#else
#include <SDL.h>
#endif

#include "pmdmini.h"
#include "pmdwinimport.h"

int audio_on = 0;

//
// PCM definitions ( BLOCK = samples )
//

#define PCM_BLOCK 2048
#define PCM_BYTE_PER_SAMPLE 2
#define PCM_CH  2
#define PCM_NUM_BLOCKS 4

#define PHASE_OUT_TIME_SECONDS (5)

//
// buffer size definitions
//

#define PCM_BLOCK_SAMPLES_CHANNELS (PCM_BLOCK * PCM_CH)
#define PCM_BUFFER_SAMPLES_CHANNELS (PCM_BLOCK_SAMPLES_CHANNELS * PCM_NUM_BLOCKS)
#define PCM_BLOCK_BYTES (PCM_BLOCK_SAMPLES_CHANNELS * PCM_BYTE_PER_SAMPLE)


//
// audio buffering
//

int buf_wpos = 0;
int buf_ppos = 0;

short buffer[ PCM_BUFFER_SAMPLES_CHANNELS ];

static void audio_callback( void *param , Uint8 *data , int len );
static int init_audio(void);
static void free_audio(void);
static void player_screen( void );
static void write_dword(uint8_t *p, uint32_t v);
static void write_word(uint8_t *p, uint16_t v);
static void audio_write_wav_header(SDL_RWops *file, long freq, long pcm_bytesize);
static void audio_loop_file(const char *outwav, const int len);
static void player_loop( const int len );
//static int split_dir( const char *file , char *dir );

static void audio_callback( void *param , Uint8 *data , int len )
{
    int i;

    short *pcm = (short *)data;

    if ( !audio_on )
    {
        std::memset( data , 0 , len );
        len = 0; /* statement cuts off following for loop */
    }

    for( i = 0; i < len / 2; i++ )
    {
        pcm[ i ] = buffer[ buf_ppos++ ];
        if ( buf_ppos >= PCM_BUFFER_SAMPLES_CHANNELS )
        {
            buf_ppos = 0;
        }
    }
}

//
// audio hardware functions
//

static int init_audio(void)
{
    SDL_AudioSpec af;

    if ( SDL_Init( SDL_INIT_AUDIO ) )
    {
        printf("Failed to Initialize!!\n");
        return 1;
    }

    af.freq = 44100;
    af.format = AUDIO_S16;
    af.channels = PCM_CH;
    af.samples = PCM_BLOCK;
    af.callback = audio_callback;
    af.userdata = NULL;

    if (SDL_OpenAudio(&af,NULL) < 0)
    {
        printf("Audio Error!!\n");
        return 1;
    }

    memset(buffer,0,sizeof(buffer));

    SDL_PauseAudio(0);
    return 0;
}

static void free_audio(void)
{
    SDL_CloseAudio();
}

static void player_screen( void )
{
    int i,n;
    int notes[32];

    n = pmd_get_tracks ( );

    // 画面の制限
    if (n > 8)
    {
        n = 8;
    }

    pmd_get_current_notes( notes , n );

    for ( i = 0; i < n; i ++ )
    {
        printf("%02x " , ( notes[i] & 0xff ) );
    }
    printf(" ");

}

//
// audio renderers
//

//
// WAVファイル系
//

static void write_dword(uint8_t *p, uint32_t v)
{
    p[0] = v & 0xff;
    p[1] = (v>>8) & 0xff;
    p[2] = (v>>16) & 0xff;
    p[3] = (v>>24) & 0xff;
}

static void write_word(uint8_t *p, uint16_t v)
{
    p[0] = v & 0xff;
    p[1] = (v>>8) & 0xff;
}

//
// audio_write_wav_header : ヘッダを出力する
// freq : 再生周波数
// pcm_bytesize : データの長さ
//

static void audio_write_wav_header(SDL_RWops *file, long freq, long pcm_bytesize)
{
    uint8_t hdr[0x80];
    
    if ( file == NULL )
        return;
    
    std::memcpy(hdr,"RIFF", 4);
    write_dword(hdr + 4, pcm_bytesize + 44);
    std::memcpy(hdr + 8,"WAVEfmt ", 8);
    write_dword(hdr + 16, 16); // chunk length
    write_word(hdr + 20, 01); // pcm id
    write_word(hdr + 22, PCM_CH); // ch
    write_dword(hdr + 24, freq); // freq
    write_dword(hdr + 28, freq * PCM_CH * PCM_BYTE_PER_SAMPLE); // bytes per sec
    write_word(hdr + 32, PCM_CH * PCM_BYTE_PER_SAMPLE); // bytes per frame
    write_word(hdr + 34, PCM_BYTE_PER_SAMPLE * 8 ); // bits

    std::memcpy(hdr + 36, "data",4);
    write_dword(hdr + 40, pcm_bytesize); // pcm size
    
    SDL_RWseek(file, 0, RW_SEEK_SET);
    SDL_RWwrite(file, hdr, 1, 44);
    
    SDL_RWseek(file, 0, RW_SEEK_END); 
    
}

//
// audio_loop_file : 音声をデータ化する
// outwav : 出力WAVファイル名
// len : 長さ(秒)
//

static void audio_loop_file(const char *outwav, const int len)
{
    SDL_RWops *file = NULL;

    int total;
    int sec,old_sec;
    int sec_sample;
    const int freq  = 44100;
    
    // ファイル
    if ( ( outwav != NULL ) && ( outwav[0] != 0 ) )
    {
        file = SDL_RWFromFile(outwav, "wb");
        if (!file)
        {       
            printf("Can't write a WAV/PCM file!\n");
            return;
        }
    }
    
    audio_write_wav_header(file, freq, 0);

    old_sec = total = sec = sec_sample = 0;

    do
    {
        pmd_renderer ( buffer + 0, ( freq / 10 ) );
        SDL_RWwrite(file, buffer, PCM_CH * PCM_BYTE_PER_SAMPLE, ( freq / 10 ));

        total += ( freq / 10 );
        sec_sample += ( freq / 10 );

        while ( sec_sample >= freq )
        {
            sec_sample -= freq;
            sec++;
        }

        if ( sec != old_sec )
        {
            old_sec = sec;
            if ( sec > len )
            {
                fadeout(10);
            }
        }

    } while(sec < (len + 5));

    audio_write_wav_header(file, freq, freq * (len + 5) * PCM_CH * PCM_BYTE_PER_SAMPLE);

    if (file)
    {
        SDL_RWclose(file);
    }

}

volatile std::sig_atomic_t signal_raised = 0;

void set_signal_raised(int signal) {
    signal_raised = 1;
}

//
// player_loop
//

static void player_loop( const int len )
{
    int total;
    int sec,old_sec;
    int sec_sample;
    const int freq  = 44100;

    old_sec = total = sec = sec_sample = 0;

    std::signal(SIGINT, set_signal_raised);
    do
    {

        if ( ( ( buf_ppos < buf_wpos ) && ( ( PCM_BUFFER_SAMPLES_CHANNELS - buf_wpos + buf_ppos ) >= PCM_BLOCK_SAMPLES_CHANNELS ) )
            ||
            ( ( buf_ppos > buf_wpos ) && ( ( buf_ppos - buf_wpos ) >= PCM_BLOCK_SAMPLES_CHANNELS ) ) )
        {
            if ( buf_wpos >= PCM_BUFFER_SAMPLES_CHANNELS )
            {
                buf_wpos = 0;
            }

            pmd_renderer ( buffer + buf_wpos, PCM_BLOCK );

            buf_wpos += PCM_BLOCK_SAMPLES_CHANNELS;

            total += PCM_BLOCK;
            sec_sample += PCM_BLOCK;

            while ( sec_sample >= freq )
            {
                sec_sample -= freq;
                sec++;
            }

            if ( sec != old_sec )
            {
                old_sec = sec;
                if ( sec > len )
                {
                    fadeout(10);
                }
            }

            player_screen();
            printf("Time : %02d:%02d / %02d:%02d\r\r", sec / 60, sec % 60, (len % 3600) / 60, (len % 3600) % 60);
            fflush(stdout);
        }
        else
        {
            SDL_Delay(1);
        }

        if (1 == signal_raised)
        {
            signal_raised = 0;
            sec = len + 1;
        }

    } while (sec < (len + PHASE_OUT_TIME_SECONDS));
    std::signal(SIGINT, SIG_DFL);

}

//
// path splitter
//

//static int split_dir( const char *file , char *dir )
//{
//    char *p;
//    int len = 0;

//    p = strrchr( (char *)file , '/' );

//    if ( p )
//    {
//        len = (int)( p - file );
//        std::strncpy ( dir , file , len );
//    }
//    dir[ len ] = 0;

//    return len;
//}

//
// entry point
//

int main ( int argc, char *argv[] )
{
    printf( "PMDPLAY on Simple Directmedia Layer 2.x (SDL2)\n" );
    printf( "PMDPLAY dated %s\n", __DATE__ );

    if ( argc < 2 )
    {
        printf("Usage pmdplay <PMDfile>\n");
        printf("or    pmdplay <PMDfile> <output.wav>\n");
        printf("or    pmdplay <PMDfile> <output.wav> <ADPCMfile>\n");
        printf("or    pmdplay <PMDfile> --           <ADPCMfile>\n");
        printf("or    pmdplay <PMDfile> --           --\n");
        printf("or    pmdplay <PMDfile> --           --          <repetitions::default=1::max=99::forever=0>\n");
        printf("or    pmdplay <PMDfile> <output.wav> --          <repetitions::default=1::max=99::forever=0>\n");
        printf("or    pmdplay <PMDfile> --           <ADPCMfile> <repetitions::default=1::max=99::forever=0>\n");
        printf("or    pmdplay <PMDfile> <output.wav> <ADPCMfile> <repetitions::default=1::max=99::forever=0>\n");
        return 1;
    }

    if (nullptr != argv[2])
    {
        if ( (45 /* minus sign */ != argv[2][0]) && (45 /* minus sign */ != argv[2][1]) )
        {
            printf("pmdplay ouput WAV file format %s\n", argv[2]);

            memset(buffer, 0, sizeof(buffer));
        }
        else
        {
            if (init_audio())
            {
                return 1;
            }
        }
    }
    else
    {
        if ( init_audio() )
        {
            return 1;
        }
    }

    char buf[1024];
#ifdef _MSC_VER
    char *pcmdir = std::getenv("USERPROFILE");
#else
    char *pcmdir = std::getenv("HOME");
#endif

    if (pcmdir)
    {
        std::strcpy(buf,pcmdir);
#ifdef _MSC_VER
        strcat(buf, "\\.pmdplay\\");
#else
        strcat(buf,"/.pmdplay/");
#endif
    }
    else
    {
        buf[0] = 0;
    }

    pmd_init( buf );

    if ( pmd_play( argv , buf ) )
    {
        printf("File open error\n");
        free_audio();
        return 1;
    }
    audio_on = 1;

    int song_length_sec = pmd_length_sec();
    int song_loop_length_sec = pmd_loop_sec();
    int sum_length_sec = song_length_sec;
    int loop_count = 1;

    if (nullptr != argv[2])
    {
        if (nullptr != argv[3])
        {
            if (nullptr != argv[4])
            {
                if ((45 /* minus sign */ != argv[4][0]) && (45 /* minus sign */ != argv[4][1]))
                {
                    loop_count = atoi(argv[4]);
                    if (loop_count > 99)
                    {
                        loop_count = 99;
                    }

                    if ((0 != loop_count) && (loop_count > 0))
                    {
                        if (0 != song_loop_length_sec)
                        {
                            sum_length_sec = song_length_sec + ((loop_count - 1) * song_loop_length_sec);
                        }
                        else
                        {
                            sum_length_sec = song_length_sec * loop_count;
                        }
                    }
                    else if (0 == loop_count)
                    {
                        sum_length_sec = INT_MAX - PHASE_OUT_TIME_SECONDS;
                    }
                }
            }
        }
    }

    if ( 0 != song_length_sec )
    {
        if ( 0 == loop_count )
        {
            if ( 0 != song_loop_length_sec )
            {
                printf(" %s plays %d sec, loop plays %d sec, however pmdplay is going to play forever\n", argv[1], song_length_sec, song_loop_length_sec );
            }
            else
            {
                printf(" %s plays %d sec, however pmdplay is going to play forever\n", argv[1], song_length_sec );
            }
        }
        else if ( 1 != loop_count )
        {
            if ( 0 != song_loop_length_sec )
            {
                printf(" %s plays %d sec, loop plays %d sec, pmdplay is going to play %d additional loop%s\n", argv[1], song_length_sec, song_loop_length_sec, ( loop_count - 1 ), ( loop_count > 2 ) ? "s" : "" );
            }
            else
            {
                printf(" %s plays %d sec, pmdplay is going to play it %d times\n", argv[1], song_length_sec, loop_count );
            }
        }
        else
        {
            if ( 0 != song_loop_length_sec )
            {
                printf(" %s plays %d sec, loop plays %d sec\n", argv[1], song_length_sec, song_loop_length_sec);
            }
            else
            {
                printf(" %s plays %d sec\n", argv[1], song_length_sec);
            }
        }
    }
    else
    {
        if ( 0 != song_loop_length_sec )
        {
            printf(" %s plays %d sec, loop plays %d sec, however pmdplay is going to play forever\n", argv[1], song_length_sec, song_loop_length_sec );
        }
        else
        {
            printf(" %s plays %d sec, however pmdplay is going to play forever\n", argv[1], song_length_sec );
        }
        sum_length_sec = INT_MAX - PHASE_OUT_TIME_SECONDS;
    }

    if (nullptr != argv[2])
    {
        if ( (45 /* minus sign */ == argv[2][0]) && (45 /* minus sign */ == argv[2][1]) )
        {
            player_loop(sum_length_sec);

            free_audio();
        }
        else
        {
            audio_loop_file( argv[2], ( sum_length_sec < 3600 ) ? sum_length_sec : 3600 );
        }
    }
    else
    {
        player_loop(sum_length_sec);

        free_audio();
    }

    pmd_stop();

    return 0;
}

