//
//  fmpmini.c
//  libfmpmini
//
//  Created by Yohann Magnien David on 08/05/2024.
//

#include "fmpmini.h"

#include "libopna/opna.h"
#include "libopna/opnatimer.h"
#include "fmdriver/fmdriver.h"
#include "common/fmplayer_file.h"
#include "common/fmplayer_common.h"
#include "common/fmplayer_fontrom.h"

static struct {
    struct opna opna;
    struct opna_timer timer;
    struct fmdriver_work work;
    struct fmplayer_file *fmfile;
    const char *lastopenpath;
    struct ppz8 ppz8;
      char adpcmram[OPNA_ADPCM_RAM_SIZE];
    char str_error[256];
    int max_loop;
    int mutemask;
} g;

void fmpmini_init(void) {
    g.lastopenpath=NULL;
    g.max_loop=2;
    g.mutemask=0;
}

void fmpmini_setMaxLoop(int loop) {
    g.max_loop=loop;
}

void fmpmini_close(void) {
    fmplayer_file_free(g.fmfile);
}

void fmpmini_mute(int mask) {
    g.mutemask=mask;
    opna_set_mask(&g.opna, mask&((1<<16)-1));
    ppz8_set_mask(&g.ppz8, (mask>>16)&((1<<16)-1));
}

const char *fmpmini_getName(void) {
    return g.fmfile->filename_sjis;
}

const char *fmpmini_getComment(int line) {
    return g.work.get_comment(&(g.work),line);
}


int fmp_ppz8_maxChannel;

int64_t fmpmini_getLength(void) {
    if (!(g.lastopenpath)) return 0;
    
    fmp_ppz8_maxChannel=0;
    
    int64_t total_smpl=0;
    while (g.work.playing) {
        opna_timer_nomix(&g.timer, 512);
        total_smpl+=512;
        if (g.work.loop_cnt>g.max_loop) break;
    }
    int64_t length=total_smpl*1000/44100;
    
    printf("Max ppz8: %d\n",fmp_ppz8_maxChannel);
    
    fmpmini_reset();
    return length;
}

void fmpmini_reset(void) {
    fmpmini_close();
    fmpmini_loadFile(g.lastopenpath);
    fmpmini_mute(g.mutemask);
}

int fmpmini_channelsNb(void){
    if (!(g.lastopenpath)) return 0;
    
    if (g.fmfile->driver.fmp.pvi_name[0] || g.fmfile->driver.fmp.ppz_name[0]) return 16+fmp_ppz8_maxChannel;
    else return 16;
}
  
int fmpmini_loadFile(const char *path) {
    enum fmplayer_file_error error;
      struct fmplayer_file *file = fmplayer_file_alloc(path, &error);
      if (!file) {
          snprintf(g.str_error,sizeof(g.str_error),"cannot open file - %s",fmplayer_file_strerror(error));
        goto err;
      }
      
      g.fmfile = file;
      fmplayer_init_work_opna(&g.work, &g.ppz8, &g.opna, &g.timer, &g.adpcmram);
      fmplayer_file_load(&g.work, g.fmfile, 1);
      // path might be the same as g.lastopenpath
      const char *tmp = g.lastopenpath;
      g.lastopenpath = strdup(path);
      if (tmp) free((void*)tmp);
      return 1;
    err:
      fmplayer_file_free(file);
    return 0;
}

int fmpmini_render(int16_t *buffer,int samples) {
    memset(buffer, 0, samples*4);
    opna_timer_mix(&g.timer, buffer, samples);
    return 0;
}
