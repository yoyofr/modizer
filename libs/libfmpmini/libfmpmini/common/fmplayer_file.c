#include "common/fmplayer_file.h"
#include <string.h>
#include <stdlib.h>

void fmplayer_file_free(const struct fmplayer_file *fmfileptr) {
  struct fmplayer_file *fmfile = (struct fmplayer_file *)fmfileptr;
  if (!fmfile) return;
  free((void *)fmfile->filename_sjis);
  free(fmfile->path);
  free(fmfile->buf);
  free(fmfile->ppzbuf[0]);
  free(fmfile->ppzbuf[1]);
  free(fmfile);
}

static void opna_writereg_dummy(struct fmdriver_work *work, unsigned addr, unsigned data) {
  (void)work;
  (void)addr;
  (void)data;
}

static unsigned opna_readreg_dummy(struct fmdriver_work *work, unsigned addr) {
  (void)work;
  (void)addr;
  return 0xff;
}

struct dummy_opna {
  uint32_t timerb_loop;
  uint8_t loopcnt;
};

static uint8_t opna_status_dummy(struct fmdriver_work *work, bool a1) {
  (void)a1;
  struct dummy_opna *opna = work->opna;
  if (!opna->timerb_loop) {
    if (work->loop_cnt >= opna->loopcnt) {
      opna->timerb_loop = work->timerb_cnt;
    } else if (work->timerb_cnt > 0xfffff) {
      opna->timerb_loop = -1;
    }
  }
  return opna->timerb_loop ? 0 : 2;
}

static void dummy_work_init(struct fmdriver_work *work, struct dummy_opna *dopna) {
  work->opna_writereg = opna_writereg_dummy;
  work->opna_readreg = opna_readreg_dummy;
  work->opna_status = opna_status_dummy;
  work->opna = dopna;
}

static struct driver_pmd *pmd_dup(const struct driver_pmd *pmd) {
  struct driver_pmd *pmddup = malloc(sizeof(*pmddup));
  if (!pmddup) return 0;
  memcpy(pmddup, pmd, sizeof(*pmd));
  size_t datalen = pmddup->datalen+1;
  const uint8_t *data = pmddup->data-1;
  uint8_t *datadup = malloc(datalen);
  if (!datadup) {
    free(pmddup);
    return 0;
  }
  memcpy(datadup, data, datalen);
  pmddup->data = datadup+1;
  pmddup->datalen = datalen-1;
  return pmddup;
}

static void pmd_free(struct driver_pmd *pmd) {
  if (pmd) {
    free(pmd->data-1);
    free(pmd);
  }
}

static struct driver_fmp *fmp_dup(const struct driver_fmp *fmp) {
  struct driver_fmp *fmpdup = malloc(sizeof(*fmpdup));
  if (!fmpdup) return 0;
  memcpy(fmpdup, fmp, sizeof(*fmp));
  fmpdup->data = malloc(fmp->datalen);
  if (!fmpdup->data) {
    free(fmpdup);
    return 0;
  }
  memcpy((void*)fmpdup->data, fmp->data, fmp->datalen);
  return fmpdup;
}

static void fmp_free(struct driver_fmp *fmp) {
  if (fmp) {
    free((void*)fmp->data);
    free(fmp);
  }
}

static void calc_loop(struct fmdriver_work *work, int loopcnt) {
  if ((loopcnt < 1) || (0xff < loopcnt)) {
    work->loop_timerb_cnt = -1;
    return;
  }
  struct dummy_opna *opna = work->opna;
  opna->loopcnt = loopcnt;
  while (!opna->timerb_loop) work->driver_opna_interrupt(work);
  work->loop_timerb_cnt = opna->timerb_loop;
}

struct fmplayer_file *fmplayer_file_alloc(const void *path, enum fmplayer_file_error *error) {
  struct fmplayer_file *fmfile = calloc(1, sizeof(*fmfile));
  if (!fmfile) {
    if (error) *error = FMPLAYER_FILE_ERR_NOMEM;
    goto err;
  }
  fmfile->path = fmplayer_path_dup(path);
  if (!fmfile->path) {
    if (error) *error = FMPLAYER_FILE_ERR_NOMEM;
    goto err;
  }
  fmfile->filename_sjis = fmplayer_path_filename_sjis(path);
  size_t filesize;
  fmfile->buf = fmplayer_fileread(path, 0, 0, 0xffff, &filesize, error);
  if (!fmfile->buf) goto err;
  if (pmd_load(&fmfile->driver.pmd, fmfile->buf, filesize)) {
    fmfile->type = FMPLAYER_FILE_TYPE_PMD;
    return fmfile;
  }
  memset(&fmfile->driver, 0, sizeof(fmfile->driver));
  if (fmp_load(&fmfile->driver.fmp, fmfile->buf, filesize)) {
    fmfile->type = FMPLAYER_FILE_TYPE_FMP;
    return fmfile;
  }
  if (error) *error = FMPLAYER_FILE_ERR_BADFILE;
err:
  fmplayer_file_free(fmfile);
  return 0;
}

static void loadppc(struct fmdriver_work *work, struct fmplayer_file *fmfile) {
  if (!strlen(fmfile->driver.pmd.ppcfile)) return;
  size_t filesize;
  void *ppcbuf = fmplayer_fileread(fmfile->path, fmfile->driver.pmd.ppcfile, ".PPC", 0, &filesize, 0);
  if (ppcbuf) {
    fmfile->pmd_ppc_err = !pmd_ppc_load(work, ppcbuf, filesize);
    free(ppcbuf);
  } else {
    fmfile->pmd_ppc_err = true;
  }
}

static bool loadppzpvi(struct fmdriver_work *work, struct fmplayer_file *fmfile, int bnum, const char *name) {
  size_t filesize;
  void *pvibuf = 0, *ppzbuf = 0;
  pvibuf = fmplayer_fileread(fmfile->path, name, ".PVI", 0, &filesize, 0);
  if (!pvibuf) goto err;
  ppzbuf = calloc(ppz8_pvi_decodebuf_samples(filesize), 2);
  if (!ppzbuf) goto err;
  if (!ppz8_pvi_load(work->ppz8, bnum, pvibuf, filesize, ppzbuf)) goto err;
  free(pvibuf);
  free(fmfile->ppzbuf[bnum]);
  fmfile->ppzbuf[bnum] = ppzbuf;
  return true;
err:
  free(ppzbuf);
  free(pvibuf);
  return false;
}

static bool loadppzpzi(struct fmdriver_work *work, struct fmplayer_file *fmfile, int bnum, const char *name) {
  size_t filesize;
  void *pzibuf = 0, *ppzbuf = 0;
  pzibuf = fmplayer_fileread(fmfile->path, name, ".PZI", 0, &filesize, 0);
  if (!pzibuf) goto err;
  ppzbuf = calloc(ppz8_pzi_decodebuf_samples(filesize), 2);
  if (!ppzbuf) goto err;
  if (!ppz8_pzi_load(work->ppz8, bnum, pzibuf, filesize, ppzbuf)) goto err;
  free(pzibuf);
  free(fmfile->ppzbuf[bnum]);
  fmfile->ppzbuf[bnum] = ppzbuf;
  return true;
err:
  free(ppzbuf);
  free(pzibuf);
  return false;
}

// returns true if error
static bool loadpmdppz(struct fmdriver_work *work, struct fmplayer_file *fmfile,int bnum, const char *ppzfile) {
  if (!strlen(ppzfile)) false;
  if (!loadppzpvi(work, fmfile, bnum, ppzfile) && !loadppzpzi(work, fmfile, bnum, ppzfile)) {
    return true;
  }
  return false;
}

static void loadpvi(struct fmdriver_work *work, struct fmplayer_file *fmfile) {
  const char *pvifile = fmfile->driver.fmp.pvi_name;
  if (!strlen(pvifile)) return;
  size_t filesize;
  void *pvibuf = fmplayer_fileread(fmfile->path, pvifile, ".PVI", 0, &filesize, 0);
  if (pvibuf) {
    fmfile->fmp_pvi_err = !fmp_adpcm_load(work, pvibuf, filesize);
    free(pvibuf);
  } else {
    fmfile->fmp_pvi_err = true;
  }
}

static void loadfmpppz(struct fmdriver_work *work, struct fmplayer_file *fmfile) {
  const char *pvifile = fmfile->driver.fmp.ppz_name;
  if (!strlen(pvifile)) return;
  fmfile->fmp_ppz_err = !loadppzpvi(work, fmfile, 0, pvifile);
}

void fmplayer_file_load(struct fmdriver_work *work, struct fmplayer_file *fmfile, int loopcnt) {
  struct dummy_opna dopna = {0};
  struct fmdriver_work dwork = {0};
  switch (fmfile->type) {
  case FMPLAYER_FILE_TYPE_PMD:
    {
      struct driver_pmd *pmddup = pmd_dup(&fmfile->driver.pmd);
      if (pmddup) {
        dummy_work_init(&dwork, &dopna);
        pmd_init(&dwork, pmddup);
        calc_loop(&dwork, loopcnt);
        pmd_free(pmddup);
        work->loop_timerb_cnt = dwork.loop_timerb_cnt;
      }
    }
    pmd_init(work, &fmfile->driver.pmd);
    loadppc(work, fmfile);
    work->pcmerror[1] = loadpmdppz(work, fmfile, 0, fmfile->driver.pmd.ppzfile);
    work->pcmerror[2] = loadpmdppz(work, fmfile, 1, fmfile->driver.pmd.ppzfile2);
    work->pcmerror[0] = fmfile->pmd_ppc_err;
    break;
  case FMPLAYER_FILE_TYPE_FMP:
    {
      struct driver_fmp *fmpdup = fmp_dup(&fmfile->driver.fmp);
      if (fmpdup) {
        dummy_work_init(&dwork, &dopna);
        fmp_init(&dwork, fmpdup);
        calc_loop(&dwork, loopcnt);
        fmp_free(fmpdup);
        work->loop_timerb_cnt = dwork.loop_timerb_cnt;
      }
    }
    fmp_init(work, &fmfile->driver.fmp);
    loadpvi(work, fmfile);
    loadfmpppz(work, fmfile);
    work->pcmerror[0] = fmfile->fmp_pvi_err;
    work->pcmerror[1] = fmfile->fmp_ppz_err;
    break;
  }
}
#define MSG_FILE_ERR_UNKNOWN "Unknown error"
#define MSG_FILE_ERR_NOMEM "Memory allocation error"
#define MSG_FILE_ERR_FILEIO "File I/O error"
#define MSG_FILE_ERR_BADFILE_SIZE "Invalid file size"
#define MSG_FILE_ERR_BADFILE "Invalid file format"
#define MSG_FILE_ERR_NOTFOUND "File not found"

#define XWIDE(x) L ## x
#define WIDE(x) XWIDE(x)

const char *fmplayer_file_strerror(enum fmplayer_file_error error) {
  if (error >= FMPLAYER_FILE_ERR_COUNT) return MSG_FILE_ERR_UNKNOWN;
  static const char *errtable[] = {
    "",
    MSG_FILE_ERR_NOMEM,
    MSG_FILE_ERR_FILEIO,
    MSG_FILE_ERR_BADFILE_SIZE,
    MSG_FILE_ERR_BADFILE,
    MSG_FILE_ERR_NOTFOUND,
  };
  return errtable[error];
}

const wchar_t *fmplayer_file_strerror_w(enum fmplayer_file_error error) {
  if (error >= FMPLAYER_FILE_ERR_COUNT) return WIDE(MSG_FILE_ERR_UNKNOWN);
  static const wchar_t *errtable[] = {
    L"",
    WIDE(MSG_FILE_ERR_NOMEM),
    WIDE(MSG_FILE_ERR_FILEIO),
    WIDE(MSG_FILE_ERR_BADFILE_SIZE),
    WIDE(MSG_FILE_ERR_BADFILE),
    WIDE(MSG_FILE_ERR_NOTFOUND),
  };
  return errtable[error];
}
