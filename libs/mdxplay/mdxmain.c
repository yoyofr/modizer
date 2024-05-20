/*
  MDX player

  Made by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Jan.16.1999


  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)

#include "mdx.h"
#include "mdxopl3.h"
#include "gettext_wrapper.h"

/* ------------------------------------------------------------------ */
#define PATH_BUF_SIZE 1024

static PDX_DATA* _get_pdx(MDX_DATA* mdx, char* mdxpath);
static int self_construct(void);
static void self_destroy(void);

static int  option_gets( int, char ** );
static void usage( void );

/* ------------------------------------------------------------------ */
static char *command_name;
static char *pdx_pathname;
static char *dsp_device;

static int   no_pdx;
static int   no_fm;
static int   no_opl3;
static int   no_ym2151;
static int   no_fm_voice;
static int   fm_waveform;
static int   pcm_volume;
static int   fm_volume;
static int   volume;
static int   max_infinite_loops;
static int   fade_out_speed;
static int   dump_voice;
static int   output_titles;

static int   is_use_fragment;
static int   is_output_to_stdout;
static int   is_output_to_stdout_in_wav;
static int   is_use_reverb;
static float reverb_predelay;
static float reverb_roomsize;
static float reverb_damp;
static float reverb_width;
static float reverb_dry;
static float reverb_wet;

//MDX_DATA *mdx = NULL;
//PDX_DATA *pdx = NULL;

/* ------------------------------------------------------------------ */

int mdx_load( char *filename, MDX_DATA **mdx, PDX_DATA **pdx, int infloop ) {
if (!self_construct()) {
    /* failed to create class instances */
    return -1;
  }

	pdx_pathname        = (char *)NULL;
	no_pdx              = FLAG_FALSE;
	no_fm               = FLAG_FALSE;
	no_opl3             = FLAG_TRUE;
	no_ym2151           = FLAG_FALSE;
	no_fm_voice         = FLAG_FALSE;
	fm_waveform         = 0;
	pcm_volume          = 127;
	fm_volume           = 127;
	volume              = 127;
	is_output_to_stdout = FLAG_FALSE;
	max_infinite_loops  = (infloop==1?32767:1); 
	fade_out_speed      = 5;
	dump_voice          = FLAG_FALSE;
	output_titles       = FLAG_FALSE;
	is_use_reverb       = FLAG_TRUE;
	reverb_predelay     = 0.05f;
	reverb_roomsize     = 0.5f;
	reverb_damp         = 0.1f;
	reverb_width        = 0.8f;
	reverb_wet          = 0.2f;
	reverb_dry          = 0.5f;
	
	dsp_device          = (char *)NULL;
	is_use_fragment     = FLAG_TRUE;
	
	is_output_to_stdout_in_wav = FLAG_TRUE;
	

    /* load mdx file */

    *mdx = mdx_open_mdx( filename );
    if ( (*mdx) == NULL ) error_end(_("file not found"));
	
	(*mdx)->is_use_pcm8         = no_pdx      == FLAG_TRUE ? FLAG_FALSE:FLAG_TRUE;
    (*mdx)->is_use_fm           = no_fm       == FLAG_TRUE ? FLAG_FALSE:FLAG_TRUE;
    (*mdx)->is_use_opl3         = no_opl3     == FLAG_TRUE ? FLAG_FALSE:FLAG_TRUE;
    (*mdx)->is_use_ym2151       = no_ym2151   == FLAG_TRUE ? FLAG_FALSE:FLAG_TRUE;
    (*mdx)->is_use_fm_voice     = no_fm_voice == FLAG_TRUE ? FLAG_FALSE:FLAG_TRUE;
    (*mdx)->fm_wave_form        = fm_waveform;
    (*mdx)->master_volume       = volume;
    (*mdx)->fm_volume           = fm_volume  * volume/127;
    (*mdx)->pcm_volume          = pcm_volume * volume/127;
    (*mdx)->max_infinite_loops  = max_infinite_loops;
    (*mdx)->fade_out_speed      = fade_out_speed;

    (*mdx)->is_output_to_stdout = is_output_to_stdout; 
    (*mdx)->is_use_fragment     = is_use_fragment;
    (*mdx)->dsp_device          = dsp_device;
    (*mdx)->dump_voice          = dump_voice;
    (*mdx)->is_output_titles    = output_titles;

    (*mdx)->is_use_reverb       = is_use_reverb;
    (*mdx)->reverb_predelay     = reverb_predelay;
    (*mdx)->reverb_roomsize     = reverb_roomsize;
    (*mdx)->reverb_damp         = reverb_damp;
    (*mdx)->reverb_width        = reverb_width;
    (*mdx)->reverb_dry          = reverb_dry;
    (*mdx)->reverb_wet          = reverb_wet;

    (*mdx)->is_output_to_stdout_in_wav = is_output_to_stdout_in_wav; 

    /* voice data load */

    if ( mdx_get_voice_parameter( *mdx ) == 0 ) {
		/* load pdx data */
		*pdx = _get_pdx(*mdx, filename);
    } else {
		self_destroy();
		return -1;
	}
	
	return 0;
}

void mdx_play(MDX_DATA *mdx,PDX_DATA *pdx) {
	/* MML data parse and play */    
	if ( mdx->is_use_ym2151 == FLAG_TRUE ) {
		mdx_parse_mml_ym2151( mdx, pdx );
	}
	
}
	    
	
void mdx_close(MDX_DATA *mdx,PDX_DATA *pdx) {	
    /* one playing finished */
	
	mdx_close_pdx( pdx );
	mdx_close_mdx( mdx );
	self_destroy();
}

void error_end( char *msg ) {

  fprintf( stderr, "%s: %s\n", command_name, msg );
  exit(1);
}

/* pdx loading */

static unsigned char*
_load_pdx_data(char* name, long* out_length) 
{
  int i;
  FILE *fp = NULL;
  struct stat stt;
  long length;
  int len = 0;
  unsigned char* buf = NULL;

  char *path_name = NULL;
  char *node_name = NULL;
  char *p = NULL;
    char *save_p;
  char *p2 = NULL;
  DIR  *pdx_path = NULL;
  struct dirent *dent = NULL;

  if ( !name ) return NULL;

  /* read PDX file in case insensitibe of file name */

  path_name = strdup( name );
  if ( !path_name ) return NULL;

  p = strrchr( path_name, '/' );
  if (!p) {
    goto error_end;
  }

  node_name = strdup(p+1);
  if ( !node_name ) {
    goto error_end;
  }
  *(p+1) = '\0';
    
    //patch: replace special chars by '_'
    for (int ii=0;node_name[ii];ii++) {
        if (node_name[ii]=='-') node_name[ii]='_';
    }
    
  
  pdx_path = opendir( path_name );
  if ( !pdx_path ) {
    goto error_end;
  }

  i=0;
  while ( (dent = readdir( pdx_path )) != NULL ) {
    p = strrchr( dent->d_name, '/' );
    if ( p == NULL ) p = dent->d_name;
    else p++;

    //patch: replace special chars by '_'
      save_p=p;
      p=strdup(p);
      for (int ii=0;p[ii];ii++) {
          if (p[ii]=='-') p[ii]='_';
      }
      
      
    if ( strcasecmp( p, node_name ) == 0 ) {
        free(p);
        p=save_p;
      i=1; break;
    }
      free(p);
  }
  if ( i == 0 ) {
    goto error_end;
  }

  len = strlen(path_name) + strlen(p) + 256;
  p2 = (char *)malloc(sizeof(char)*len);
  if (!p2) {
    goto error_end;
  }
  memset(p2, 0, len);
  snprintf( p2, len, "%s%s", path_name, p );
  fp = fopen( p2, "r" );
  if (!fp) {
    goto error_end;
  }

  fstat(fileno(fp), &stt);
  length = stt.st_size;
  buf = (unsigned char *)malloc(sizeof(unsigned char)*length);
  if ( !buf ) {
    goto error_end;
  }

  len = fread(buf, 1, length, fp );
  if (len<0) {
    goto error_end;
  }
  fclose(fp);

  closedir( pdx_path );
  free( node_name );
  free( path_name );
  free( p2 );

  if (out_length) {
    *out_length = length;
  }
  return buf;

error_end:
  if (fp) {
    fclose(fp);
  }
  if (p2) {
    free(p2);
  }
  if (path_name) {
    free(path_name);
  }
  if (node_name) {
    free(node_name);
  }
  if (pdx_path) {
    closedir(pdx_path);
  }
  return NULL;
}

static PDX_DATA*
_open_pdx(char* name)
{
  unsigned char* buf = NULL;
  long length = 0;
  PDX_DATA* pdx = NULL;

  buf = _load_pdx_data(name, &length);
  if (!buf) {
    return NULL;
  }
  pdx = mdx_open_pdx(buf, length);
  free(buf);
  return pdx;
}

static PDX_DATA*
_get_pdx(MDX_DATA* mdx, char* mdxpath)
{
  char *a = NULL;
  char buf[PATH_BUF_SIZE];
  PDX_DATA* pdx = NULL;

  mdx->pdx_enable = FLAG_FALSE;
  if ( mdx->haspdx == FLAG_FALSE ) goto no_pdx_file;

  /* current directory */
  memset(buf, 0, PATH_BUF_SIZE);
  if (getcwd(buf, PATH_BUF_SIZE-1-strlen(mdx->pdx_name)-1)!=NULL) {
    strcat(buf, "/");
    strcat( buf, mdx->pdx_name );
    if ( (pdx=_open_pdx( buf )) != NULL ) goto get_pdx_file;
  }
  
  /* mdx file path directory */
  if ( strlen(mdxpath)+strlen(mdx->pdx_name) > PATH_BUF_SIZE ) goto next0;
  memset(buf, 0, PATH_BUF_SIZE);
  strncpy( buf, mdxpath, PATH_BUF_SIZE-1 );
  if ( (a=strrchr( buf, '/' )) != NULL ) {
    *(a+1)='\0';
    strcat( buf, mdx->pdx_name );
    if ( (pdx=_open_pdx( buf )) != NULL ) goto get_pdx_file;
  }

next0:
  /* specified directory : option "--pdx_path" */
  memset(buf, 0, PATH_BUF_SIZE);
  if ( pdx_pathname == NULL ) goto next;
  if ( strlen(pdx_pathname)+strlen(mdx->pdx_name) > PATH_BUF_SIZE ) goto next;
  snprintf( buf, PATH_BUF_SIZE, "%s/%s", pdx_pathname, mdx->pdx_name );
  if ( (pdx=_open_pdx( buf )) != NULL ) goto get_pdx_file;
    
next:
  /* specified directory : env "PDX_PATH" */
  memset(buf, 0, PATH_BUF_SIZE);
  a = getenv( MDX_PDX_PATH_ENV_NAME );
  if ( a==NULL ) goto next2;
  if ( strlen(a)+strlen(mdx->pdx_name) > PATH_BUF_SIZE ) goto next2;
  snprintf( buf, PATH_BUF_SIZE, "%s/%s", a, mdx->pdx_name );
  if ( (pdx=_open_pdx( buf )) != NULL ) goto get_pdx_file;
    
next2:
  //fprintf(stderr,_("%s: No pdx file : %s\n"), command_name, mdx->pdx_name );
no_pdx_file:
  goto unget_pdx_file;
    
unget_pdx_file:
  mdx->haspdx = FLAG_FALSE;
  mdx->tracks = 9;
  mdx->pdx_enable = FLAG_TRUE;
  return NULL;

get_pdx_file:
  mdx->pdx_enable = FLAG_TRUE;
  return pdx;
}

/* class initializations */
extern void* _mdxmml_ym2151_initialize(void);
extern void* _mdx2151_initialize(void);
extern void* _pcm8_initialize(void);
extern void* _ym2151_c_initialize(void);
extern void  _mdxmml_ym2151_finalize(void* self);
extern void  _mdx2151_finalize(void* in_self);
extern void  _pcm8_finalize(void* in_self);
extern void  _ym2151_c_finalize(void* in_self);

static void* self_mdx2151 = NULL;
static void* self_mdxmml_ym2151 = NULL;
static void* self_pcm8 = NULL;
static void* self_ym2151_c = NULL;

void*
_get_mdx2151(void)
{
  return self_mdx2151;
}

void*
_get_mdxmml_ym2151(void)
{
  return self_mdxmml_ym2151;
}

void*
_get_pcm8(void)
{
  return self_pcm8;
}

void*
_get_ym2151_c(void)
{
  return self_ym2151_c;
}

static int
self_construct(void)
{
  self_mdx2151 = _mdx2151_initialize();
  if (!self_mdx2151) {
    goto error_end;
  }
  self_mdxmml_ym2151 = _mdxmml_ym2151_initialize();
  if (!self_mdxmml_ym2151) {
    goto error_end;
  }
  self_pcm8 = _pcm8_initialize();
  if (!self_pcm8) {
    goto error_end;
  }
#if 0
  self_ym2151_c = _ym2151_c_initialize();
  if (!self_ym2151_c) {
    goto error_end;
  }
#endif

  return FLAG_TRUE;

error_end:
#if 0
  if (self_ym2151_c) {
    _ym2151_c_finalize(self_ym2151_c);
    self_ym2151_c = NULL;
  }
#endif
  if (self_pcm8) {
    _pcm8_finalize(self_pcm8);
    self_pcm8 = NULL;
  }
  if (self_mdxmml_ym2151) {
    _mdxmml_ym2151_finalize(self_mdxmml_ym2151);
    self_mdxmml_ym2151 = NULL;
  }
  if (self_mdx2151) {
    _mdx2151_finalize(self_mdx2151);
    self_mdx2151 = NULL;
  }
  return FLAG_FALSE;
}

static void
self_destroy(void)
{
#if 0
  if (self_ym2151_c) {
    _ym2151_c_finalize(self_ym2151_c);
    self_ym2151_c = NULL;
  }
#endif
  if (self_pcm8) {
    _pcm8_finalize(self_pcm8);
    self_pcm8 = NULL;
  }
  if (self_mdxmml_ym2151) {
    _mdxmml_ym2151_finalize(self_mdxmml_ym2151);
    self_mdxmml_ym2151 = NULL;
  }
  if (self_mdx2151) {
    _mdx2151_finalize(self_mdx2151);
    self_mdx2151 = NULL;
  }
}


#if 0
/* command line handling */

static int option_gets( int argc, char **argv ) {

  extern char *optarg;
  extern int optind;

  int c;
  int option_index=0;

  pdx_pathname        = (char *)NULL;
  no_pdx              = FLAG_FALSE;
  no_fm               = FLAG_FALSE;
  no_opl3             = FLAG_TRUE;
  no_ym2151           = FLAG_FALSE;
  no_fm_voice         = FLAG_FALSE;
  fm_waveform         = 0;
  pcm_volume          = 127;
  fm_volume           = 127;
  volume              = 127;
  is_output_to_stdout = FLAG_FALSE;
  max_infinite_loops  = 1;
  fade_out_speed      = 5;
  dump_voice          = FLAG_FALSE;
  output_titles       = FLAG_FALSE;
  is_use_reverb       = FLAG_FALSE;
  reverb_predelay     = 0.05f;
  reverb_roomsize     = 0.5f;
  reverb_damp         = 0.1f;
  reverb_width        = 0.8f;
  reverb_wet          = 0.2f;
  reverb_dry          = 0.5f;

  dsp_device          = (char *)NULL;
  is_use_fragment     = FLAG_TRUE;

  is_output_to_stdout_in_wav = FLAG_TRUE;

  command_name =
    (strrchr(argv[0],'/')==NULL)?argv[0]:(strrchr(argv[0],'/')+1);

  while(1) {
    static struct option long_options[] = {
      {"pdxpath",          1, 0, 'p'},
      {"no-pcm",           0, 0, 'P'},
      {"no-fm",            0, 0, 'F'},
#if defined(CAN_HANDLE_OPL3) || defined (HAVE_SUPPORT_DMFM)
      {"no-fm-voice",      0, 0, 'I'},
      {"opl2",             0, 0, '2'},
      {"opl3",             0, 0, '4'},
      {"psg",              0, 0, 'G'},
      {"fm-waveform",      1, 0, 'W'},
#endif /* CAN_HANDLE_OPL3 */
      {"ym2151",           0, 0, 'X'},
      {"opm",              0, 0, 'X'},
      {"pcm-volume",       1, 0, 'C'},
      {"fm-volume",        1, 0, 'M'},
      {"volume",           1, 0, 'v'},
      {"output-to-stdout", 0, 0, 's'},
      {"output-as-raw",    0, 0, 'R'},
#ifdef HAVE_OSS_AUDIO
      {"dsp",              1, 0, 'd'},
      {"fragment",         1, 0, 'A'},
#endif /* HAVE_OSS_AUDIO */
      {"loops",            1, 0, 'l'},
      {"fade-out",         1, 0, 'f'},
      {"dump-voice",       0, 0, '/'},
      {"output-titles",    0, 0, 't'},

      {"use-reverb",       0, 0, 'b'},
      {"reverbparam-pre",  1, 0, '0'},
      {"reverbparam-room", 1, 0, '1'},
      {"reverbparam-damp", 1, 0, '>'},
      {"reverbparam-width",1, 0, '3'},
      {"reverbparam-dry",  1, 0, '<'},
      {"reverbparam-wet",  1, 0, '5'},

      {"version",          0, 0, 'V'},
      {"help",             0, 0, 'h'},
      {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv, "VAbtsxl:f:p:v:d:h", long_options, &option_index );
    if ( c == EOF ) break;

    switch(c) {
    case 'l':
      max_infinite_loops = atoi(optarg);
      if ( max_infinite_loops < 0 ) max_infinite_loops = 0;
      break;

    case 'f':
      fade_out_speed = atoi(optarg);
      if ( fade_out_speed < 0 ) fade_out_speed = 0;
      break;

    case 'p': /* pdx pathname */
      if ( pdx_pathname != NULL ) free(pdx_pathname);
      pdx_pathname = (char *)malloc(sizeof(char)*strlen(optarg)+16);
      strcpy( pdx_pathname, optarg );
      break;

    case 'v': /* master volume */
      volume = atoi(optarg);
      if ( volume < 0 ) volume = 0;
      else if ( volume > 127 ) volume = 127;
      break;

    case 'P': /* no pcm pronouncing */
      no_pdx = FLAG_TRUE;
      break;

    case 'F': /* no fm */
      no_fm = FLAG_TRUE;
      no_opl3 = FLAG_TRUE;
      no_ym2151 = FLAG_TRUE;
      break;

    case 'x':
    case 'X': /* ym2151,opm */
      no_fm = FLAG_FALSE;
      no_ym2151 = FLAG_FALSE;
      no_opl3 = FLAG_TRUE;
      break;

#if defined(CAN_HANDLE_OPL3) || defined(HAVE_SUPPORT_DMFM)

    case '2': /* opl2 */
      no_fm = FLAG_FALSE;
      no_ym2151 = FLAG_TRUE;
      no_opl3 = FLAG_TRUE;
      break;

    case '4': /* opl3 */
      no_fm = FLAG_FALSE;
      no_ym2151 = FLAG_TRUE;
      no_opl3 = FLAG_FALSE;
      break;

    case 'G': /* PSG sound */
      fm_waveform = 6;
      no_fm_voice =FLAG_TRUE;
      no_fm = FLAG_FALSE;
      no_ym2151 = FLAG_TRUE;
      no_opl3 = FLAG_FALSE;
      break;

    case 'I': /* no fm voice */
      no_fm_voice = FLAG_TRUE;
      break;

    case 'W': /* fm-waveform */
      fm_waveform = atoi(optarg);
      if ( fm_waveform < 0 ) fm_waveform = 0;
      else if ( fm_waveform > 7 ) fm_waveform = 5;
      break;

#endif /* CAN_HANDLE_OPL3 */

    case 'C': /* pcm volume */
      pcm_volume = atoi(optarg);
      if ( pcm_volume < 0 ) pcm_volume = 0;
      else if ( pcm_volume > 127 ) pcm_volume = 127;
      break;

    case 'M': /* fm volume */
      fm_volume = atoi(optarg);
      if ( fm_volume < 0 ) fm_volume = 0;
      else if ( fm_volume > 127 ) fm_volume = 127;
      break;

    case 'h': /* help */
      usage();
      break;


    case 's':
      is_output_to_stdout = FLAG_TRUE;
      break;

    case 'R':
      is_output_to_stdout = FLAG_TRUE;
      is_output_to_stdout_in_wav = FLAG_FALSE;
      break;

#ifdef HAVE_OSS_AUDIO
    case 'd': /* specify dsp device */
      dsp_device = strdup(optarg);
      break;

    case 'A': /* is use SETFRAGMENT */
      is_use_fragment = FLAG_FALSE;
      break;
#endif /* HAVE_OSS_AUDIO */

    case '/':
      dump_voice = FLAG_TRUE;
      break;

    case 't':
      output_titles = FLAG_TRUE;
      break;

    case 'b':
      is_use_reverb = FLAG_TRUE;
      break;
    case '0':
      reverb_predelay = atof(optarg);
      break;
    case '1':
      reverb_roomsize = atof(optarg);
      break;
    case '>':
      reverb_damp = atof(optarg);
      break;
    case '3':
      reverb_width = atof(optarg);
      break;
    case '<':
      reverb_dry = atof(optarg);
      break;
    case '5':
      reverb_wet = atof(optarg);
      break;

    case '?':
      exit(1);
      break;
    default:
      exit(1);
      break;
    }
  }

  if ( optind >= argc ) {
    fprintf(stderr, _("%s: No mdx file is specified.\n"), command_name);
    exit(1);
  } 

  return optind;
}

static void usage( void ) {

  fprintf(stderr, "usage: %s [options] [mdx-filenames]\n", command_name );
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "-p DIR, --pdxpath=DIR      ");
  fprintf(stderr, _("Search DIR for PDX file.\n"));
  fprintf(stderr, "-v val, --volume val       ");
  fprintf(stderr, _("Set master volume to val.\n"));
  fprintf(stderr, "-l val, --loops val        ");
  fprintf(stderr, _("Set max infinite loops number to val.\n"));
  fprintf(stderr, "-f val, --fade-out val     ");
  fprintf(stderr, _("Set fade-out speed as val.\n"));
#ifdef HAVE_OSS_AUDIO
  fprintf(stderr, "-d dev, --dsp dev          ");
  fprintf(stderr, _("Set DSP device name.\n"));
#endif /* HAVE_OSS_AUDIO */
  fprintf(stderr, "        --fm-volume val    ");
  fprintf(stderr, _("Set FM volume to val.\n"));
  fprintf(stderr, "        --pcm-volume val   ");
  fprintf(stderr, _("Set PCM volume to val.\n"));
  fprintf(stderr, "        --no-pcm           ");
  fprintf(stderr, _("Ignore all PCM part.\n"));
  fprintf(stderr, "        --no-fm            ");
  fprintf(stderr, _("Ignore all FM part.\n"));
#if defined(CAN_HANDLE_OPL3) || defined(HAVE_SUPPORT_DMFM)
  fprintf(stderr, "        --no-fm-voice      ");
  fprintf(stderr, _("Ignore all FM voice setting.\n"));
  fprintf(stderr, "        --fm-waveform val  ");
  fprintf(stderr, _("Set wave form for FM part.\n"));
  fprintf(stderr, "        --opl2             ");
  fprintf(stderr, _("Use 2 operator for FM part.\n"));
  fprintf(stderr, "        --opl3             ");
  fprintf(stderr, _("Use 4 operator for FM part.\n"));
  fprintf(stderr, "        --psg              ");
  fprintf(stderr, _("Same as --no-fm-voice --fm-waveform 6\n"));
#endif /* CAN_HANDLE_OPL3 */
  fprintf(stderr, "-x,     --opm              ");
  fprintf(stderr, _("Use YM2151 emulator (default).\n"));
  fprintf(stderr, "-s,     --output-to-stdout ");
  fprintf(stderr, _("Output PCM data to stdout.\n"));
  fprintf(stderr, "        --output-as-raw    ");
  fprintf(stderr, _("Output PCM data as raw data.\n"));
  fprintf(stderr, "-b,     --reverb           ");
  fprintf(stderr, _("Add reverb.\n"));
  fprintf(stderr, "        --dump-voice       ");
  fprintf(stderr, _("Dump all voice parameters.\n"));
  fprintf(stderr, "--reverbparams-{pre,room,damp,width,dry,wet}\n");
  fprintf(stderr, "                           ");
  fprintf(stderr, _("Reverb parameters.\n"));
  fprintf(stderr, "-V,     --version          ");
  fprintf(stderr, _("Show version information.\n"));
  fprintf(stderr, "-h,     --help             ");
  fprintf(stderr, _("Show this help message.\n"));

  exit(0);
}
#endif


