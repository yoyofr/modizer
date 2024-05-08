#ifndef MYON_FMPLAYER_FILE_H_INCLUDED
#define MYON_FMPLAYER_FILE_H_INCLUDED

#include <stddef.h>
#include "fmdriver/fmdriver.h"
#include "fmdriver/fmdriver_pmd.h"
#include "fmdriver/fmdriver_fmp.h"
#include "libopna/opnadrum.h"

enum fmplayer_file_type {
  FMPLAYER_FILE_TYPE_PMD,
  FMPLAYER_FILE_TYPE_FMP
};

enum fmplayer_file_error {
  FMPLAYER_FILE_ERR_OK,
  FMPLAYER_FILE_ERR_NOMEM,
  FMPLAYER_FILE_ERR_FILEIO,
  FMPLAYER_FILE_ERR_BADFILE_SIZE,
  FMPLAYER_FILE_ERR_BADFILE,
  FMPLAYER_FILE_ERR_NOTFOUND,
  FMPLAYER_FILE_ERR_COUNT
};

struct fmplayer_file {
  void *path;
  enum fmplayer_file_type type;
  union {
    struct driver_pmd pmd;
    struct driver_fmp fmp;
  } driver;
  bool pmd_ppc_err;
  bool fmp_pvi_err;
  bool fmp_ppz_err;
  void *buf;
  void *ppzbuf[2];
  // for display with FMDSP
  // might be NULL
  // currently only supports sjis (CP932)
  // string valid while file object valid
  const char *filename_sjis;
};
struct fmplayer_file *fmplayer_file_alloc(const void *path, enum fmplayer_file_error *error);
void fmplayer_file_free(const struct fmplayer_file *fmfile);
void fmplayer_file_load(struct fmdriver_work *work, struct fmplayer_file *fmfile, int loopcnt);

const char *fmplayer_file_strerror(enum fmplayer_file_error error);
const wchar_t *fmplayer_file_strerror_w(enum fmplayer_file_error error);

// path: wchar_t* on windows, else char*
// examples:
//   fmplayer_fileread("/home/foo/bar.mz", 0, 0, &filesize); 
//   fmplayer_fileread("/home/foo/bar.mz", "BAZ", ".PVI", &filesize);
void *fmplayer_fileread(const void *path, const char *pcmname, const char *extension, size_t maxsize, size_t *filesize, enum fmplayer_file_error *error);

// allocates string in sjis
// free with free()
char *fmplayer_path_filename_sjis(const void *path);

void *fmplayer_path_dup(const void *path);

#endif // MYON_FMPLAYER_FILE_H_INCLUDED
