#include "common/fmplayer_file.h"
#include <gio/gio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

static void *fileread(GFile *f, size_t maxsize, size_t *filesize, enum fmplayer_file_error *error) {
  GFileInfo *finfo = 0;
  GFileInputStream *fstream = 0;
  void *buf = 0;
  finfo = g_file_query_info(f, G_FILE_ATTRIBUTE_STANDARD_SIZE, 0, 0, 0);
  if (!finfo) {
    if (error) *error = FMPLAYER_FILE_ERR_FILEIO;
    goto err;
  }
  gsize filelen;
  {
    goffset sfilelen = g_file_info_get_size(finfo);
    if (sfilelen < 0) {
      if (error) *error = FMPLAYER_FILE_ERR_FILEIO;
      goto err;
    }
    filelen = sfilelen;
  }
  if (maxsize && (filelen > maxsize)) {
    if (error) *error = FMPLAYER_FILE_ERR_BADFILE_SIZE;
    goto err;
  }
  fstream = g_file_read(f, 0, 0);
  if (!fstream) {
    if (error) *error = FMPLAYER_FILE_ERR_NOTFOUND;
    goto err;
  }
  buf = malloc(filelen);
  if (!buf) {
    if (error) *error = FMPLAYER_FILE_ERR_NOMEM;
    goto err;
  }
  gsize fileread;
  g_input_stream_read_all(G_INPUT_STREAM(fstream), buf, filelen, &fileread, 0, 0);
  if (fileread != filelen) {
    if (error) *error = FMPLAYER_FILE_ERR_FILEIO;
    goto err;
  }
  *filesize = filelen;
  g_object_unref(G_OBJECT(fstream));
  g_object_unref(G_OBJECT(finfo));
  return buf;
err:
  free(buf);
  if (fstream) g_object_unref(G_OBJECT(fstream));
  if (finfo) g_object_unref(G_OBJECT(finfo));
  return 0;
}

void *fmplayer_fileread(const void *pathptr, const char *pcmname, const char *extension,
                        size_t maxsize, size_t *filesize, enum fmplayer_file_error *error) {
  GFile *file = 0, *dir = 0;
  GFileEnumerator *direnum = 0;
  char *pcmnamebuf = 0;
  const char *uri = pathptr;
  file = g_file_new_for_uri(uri);
  if (!pcmname) {
    void *buf = fileread(file, maxsize, filesize, error);
    g_object_unref(G_OBJECT(file));
    return buf;
  }
  if (extension) {
    size_t namebuflen = strlen(pcmname) + strlen(extension) + 1;
    pcmnamebuf = malloc(namebuflen);
    if (!pcmnamebuf) goto err;
    strcpy(pcmnamebuf, pcmname);
    strcat(pcmnamebuf, extension);
    pcmname = pcmnamebuf;
  }

  dir = g_file_get_parent(file);
  if (!dir) {
    if (error) *error = FMPLAYER_FILE_ERR_NOMEM;
    goto err;
  }
  direnum = g_file_enumerate_children(dir,
                                      G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                      G_FILE_ATTRIBUTE_STANDARD_TARGET_URI,
                                      G_FILE_QUERY_INFO_NONE,
                                      0, 0);
  if (!direnum) {
    if (error) *error = FMPLAYER_FILE_ERR_NOMEM;
    goto err;
  }
  for (;;) {
    GFileInfo *info = g_file_enumerator_next_file(direnum, 0, 0);
    if (!info) {
      if (error) *error = FMPLAYER_FILE_ERR_FILEIO;
      goto err;
    }
    GFile *pcmfile = g_file_enumerator_get_child(direnum, info);
    if (!strcasecmp(g_file_info_get_name(info), pcmname)) {
      void *buf = fileread(pcmfile, maxsize, filesize, error);
      g_object_unref(G_OBJECT(pcmfile));
      g_object_unref(G_OBJECT(info));
      g_object_unref(G_OBJECT(direnum));
      g_object_unref(G_OBJECT(dir));
      g_object_unref(G_OBJECT(file));
      free(pcmnamebuf);
      return buf;
    }
    g_object_unref(G_OBJECT(pcmfile));
    g_object_unref(G_OBJECT(info));
  }
  if (error) *error = FMPLAYER_FILE_ERR_NOTFOUND;
err:
  if (direnum) g_object_unref(G_OBJECT(direnum));
  if (dir) g_object_unref(G_OBJECT(dir));
  g_object_unref(G_OBJECT(file));
  free(pcmnamebuf);
  return 0;
}

void *fmplayer_path_dup(const void *path) {
  return strdup(path);
}

char *fmplayer_path_filename_sjis(const void *pathptr) {
  const char *uri = pathptr;
  GFile *file = 0;
  GFileInfo *finfo = 0;
  const char *filename = 0;
  char *filenamesjis = 0, *filenamebuf = 0;
  file = g_file_new_for_uri(uri);
  if (!file) goto err;
  finfo = g_file_query_info(
      file,
      G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
      G_FILE_QUERY_INFO_NONE,
      0,
      0);
  if (!finfo) goto err;
  filename = g_file_info_get_attribute_string(
      finfo, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
  filenamesjis = g_convert_with_fallback(
      filename, -1,
      "cp932", "utf-8", "?",
      0, 0, 0);
  if (!filenamesjis) goto err;
  filenamebuf = strdup(filenamesjis);
err:
  if (filenamesjis) g_free(filenamesjis);
  if (finfo) g_object_unref(G_OBJECT(finfo));
  if (file) g_object_unref(G_OBJECT(file));
  return filenamebuf;
}

