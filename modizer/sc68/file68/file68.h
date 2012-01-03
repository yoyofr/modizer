/**
 * @ingroup   file68_file68_devel
 * @file      file68/file68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      1998/09/03
 * @brief     sc68 file header.
 *
 * $Id: file68.h 559 2006-02-19 14:02:26Z loke $
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _FILE68_H_
#define _FILE68_H_

#include "istream68.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup  file68_file68_devel sc68 files
 *  @ingroup   file68_devel
 *
 *     Provides various functions for sc68 file manipulation.
 *
 *  @{
 */

/** SC68 file identification string definition.
 *  @see SC68file_idstr
 */
#define SC68_IDSTR "SC68 Music-file / (c) (BeN)jamin Gerard / SasHipA-Dev  "

/** SC68 file identification string definition V2.
 *  Introduce this new indentitification string for SC68 file format V2.
 *  Aim to defeat some webserver text mode auto-detect but it does not
 *  work as I have expected.
 *  @see SC68file_idstr_v2.
 */
#define SC68_IDSTR_V2 {'S','C','6','8', 0,2,255,0xa9, 0xDF,0xEB,0xD1,0}

#define SC68_NOFILENAME "???"  /**< SC68 unknown filename or author.        */
#define SC68_LOADADDR   0x8000 /**< Default load address in 68K memory.     */
#define SC68_MAX_TRACK  99     /**< Maximum track per disk (display rules). */


/** @name  Features flag definitions for music68_t.
 *  @{
 */
#define SC68_YM        1     /**< YM-2149 actif.                  */
#define SC68_STE       2     /**< STE sound actif.                */
#define SC68_AMIGA     4     /**< AMIGA sound actif.              */
#define SC68_STECHOICE 8     /**< Optionnal STF/STE (not tested). */
/**@}*/


/** SC68 file chunk header. */
typedef struct
{
  char id[4];   /**< Must be "SC??".            */
  char size[4]; /**< Size in bytes (MSB first). */
} chunk68_t;


/** @name SC68 file chunk definitions.
 *  @{
 */

#define CH68_CHUNK     "SC"    /**< Chunk identifier */

#define CH68_BASE      "68"    /**< Start of file */
#define CH68_FNAME     "FN"    /**< File name */
#define CH68_DEFAULT   "DF"    /**< Default music */

#define CH68_MUSIC     "MU"    /**< Music section start */
#define CH68_MNAME     "MN"    /**< Music name */
#define CH68_ANAME     "AN"    /**< Author name */
#define CH68_CNAME     "CN"    /**< Composer name */
#define CH68_D0        "D0"    /**< D0 value */
#define CH68_AT        "AT"    /**< Load address */
#define CH68_TIME      "TI"    /**< length in seconds */
#define CH68_FRAME     "FR"    /**< length in frames */
#define CH68_FRQ       "FQ"    /**< Main replay frequency in Hz */
#define CH68_LOOP      "LP"    /**< Number of loop */

#define CH68_TYP       "TY"    /**< Not standard st file */
#define CH68_IMG       "IM"    /**< Picture */
#define CH68_REPLAY    "RE"    /**< External replay */

#define CH68_MDATA     "DA"    /**< Music data */

#define CH68_EOF       "EF"    /**< End of file */

/**@}*/

/** Hardware and features flags. */
typedef union {
  /** Flags bit field. */
  struct {
    unsigned ym:1;        /**< Music uses YM-2149 (ST).          */
    unsigned ste:1;       /**< Music uses STE specific hardware. */
    unsigned amiga:1;     /**< Music uses Paula Amiga hardware.  */
    unsigned stechoice:1; /**< Music allow STF/STE choices.      */
  } bit;
  /** All flags in one. */
  unsigned all;
} hwflags68_t;

/** SC68 music (track) structure. */
typedef struct
{

  /** @name  Music replay parameters.
   *  @{
   */
  unsigned d0;          /**< D0 value to init this music.            */
  unsigned a0;          /**< A0 Loading address. @see SC68_LOADADDR. */
  unsigned frq;         /**< Frequency in Hz (default:50).           */
  unsigned start_ms;    /**< Start time in ms from disk 1st track.   */
  unsigned time_ms;     /**< Duration in ms.                         */
  unsigned frames;      /**< Number of frame.                        */
  int loop;             /**< Default number of loop (0:infinite).    */
  int track;            /**< Track remapping number (0:default).     */

  hwflags68_t hwflags;  /**< Hardware and features.                  */

  /**@}*/

  /** @name  Human readable information.
   *  @{
   */
  char * name;          /**< Music name.           */
  char * author;        /**< Author name.          */
  char * composer;      /**< Composer name.        */
  char * replay;        /**< External replay name. */
  char * converter;     /**< Converter name.       */
  char * ripper;     /**< Copier name.          */
  /**@}*/

  /** @name  Music data.
   *  @{
   */
  unsigned datasz;      /**< data size in bytes. */
  char    *data;        /**< Music data.         */
  /**@}*/

} music68_t;


/** SC68 music disk structure.
 *
 *     The disk68_t structure is the memory representation for an SC68 disk. 
 *     Each SC68 file could several music or tracks, in the limit of a
 *     maximum of 99 tracks per file. Each music is independant, but some
 *     information, including music data, could be inherit from previous
 *     track. In a general case, tracks are grouped by theme, that could be
 *     a Demo or a Game.
 *
 */
typedef struct
{
  /** @name  Disk information.
   *  @{
   */
  int      default_six; /**< Perfered default music (default is 0).        */
  int      nb_six;      /**< Number of music track in file.                */
  unsigned time_ms;     /**< Total time for all tracks in ms.              */
  hwflags68_t hwflags;  /**< All tracks flags ORed.                        */
  char    *name;        /**< Disk name.                                    */
  /**@}*/

  /** @name  Music data.
   *  @{
   */
  music68_t mus[SC68_MAX_TRACK]; /**< Information for each music.        */
  char      data[1];             /**< raw data (must be last in struct). */
  /**@}*/

} disk68_t;

/** SC68 file identifier string.
 *
 * @see SC68_IDSTR
 */
extern const char SC68file_idstr[];

/** SC68 file identifier string V2.
 *
 * @see SC68_IDSTR_V2
 */
extern const char SC68file_idstr_v2[12];

/** @name  File verify functions.
 *  @{
 */

/** Verify SC68 file from stream.
 *
 *    The SC68file_verify() function opens, reads and closes given file to
 *    determine if it is a valid SC68 file. This function only checks for a
 *    valid file header, and does not perform any consistent error checking.
 *
 *  @param  is    input stream to verify
 *
 *  @return error-code
 *  @retval  0  success, seems to be a valid SC68 file
 *  @retval <0  failure, file error or invalid SC68 file
 *
 *  @see SC68file_load()
 *  @see SC68file_save()
 *  @see SC68file_diskname()
 */
int SC68file_verify(istream_t * is);

/** Verify SC68 file.
 *
 *  @param  fname  path to file to verify.
 */
int SC68file_verify_file(const char * fname);

/** Verify SC68 file mapped into memory buffer.
 *
 *  @param  buffer  buffer address
 *  @param  len     buffer length
 */
int SC68file_verify_mem(const void * buffer, int len);

/** Get SC68 disk name.
 *
 *    The SC68file_diskname() function opens, reads and closes given file to
 *    determine if it is a valid SC68 file. In the same time it tries to
 *    retrieve the stored disk name into the dest buffer with a maximum
 *    length of max bytes.  If the name overflows, the last byte of the dest
 *    buffer will be non zero.
 *
 *  @param  is    input stream
 *  @param  dest  disk name destination buffer
 *  @param  max   number of bytes of dest buffer
 *
 *  @return error-code
 *  @retval  0  success, found a disk-name
 *  @retval <0  failure, file error, invalid SC68 file or disk-name not found
 *
 *  @see SC68file_load()
 *  @see SC68file_save()
 *  @see SC68file_diskname()
 *
 *  @deprecated This function needs to be rewrite.
 */
int SC68file_diskname(istream_t * is, char * dest, int max);

/** Check if an URL is a standard sc68 one.
 *
 *  @param  url        URL to check
 *  @param  exts       extension list. (0 is default: ".sc68\0.sndh\0.snd\0").
 *  @param  is_remote  fill with 0/1 if respectevely URL is a local/remote
 *                     file. May be 0.
 */
int SC68file_is_our_file(const char * url,
			 const char * exts,
			 int * is_remote);

/**@}*/


/** @name  File load functions.
 *  @{
 */

/** Load SC68 file from stream.
 *
 *    The SC68file_load() function allocates memory and loads an SC68 file.
 *    The function performs all necessary initializations in the returned
 *    disk68_t structure. A single buffer has been allocated including
 *    disk68_t structure followed by music data. It is user charge to free
 *    memory by calling SC68_free() function.
 *
 *  @param  is  input stream
 *
 *  @return  pointer to allocated disk68_t disk structure
 *  @retval  0  failure
 *
 *  @see SC68file_verify()
 *  @see SC68file_save()
 */
disk68_t * SC68file_load(istream_t * is);

/** Load SC68 file.
 *
 *  @param  fname  path to file to load.
 */
disk68_t * SC68file_load_file(const char * fname);

/** Load SC68 file mapped into memory buffer.
 *
 *  @param  buffer  buffer address
 *  @param  len     buffer length
 */
disk68_t * SC68file_load_mem(const void * buffer, int len);

/**@}*/


/** @name  File save functions.
 *  @{
 */

/** Save SC68 disk into stream.
 *
 *  @param  os     output stream (must be seekable)
 *  @param  mb     pointer to SC68 disk to save
 *  @param  gzip   gzip compression level [0:no-gzip, 1..9 or -1]
 *
 *  @return error-code
 *  @retval  0  success
 *  @retval <0  failure
 *
 *  @see SC68file_load()
 *  @see SC68file_verify()
 *  @see SC68file_diskname()
 */
int SC68file_save(istream_t * os, const disk68_t * mb, int gzip);

/** Save SC68 disk into file.
 *
 *  @param  fname  filename.
 *  @param  mb     pointer to SC68 disk to save
 *  @param  gzip   gzip compression level [0:no-gzip, 1..9 or -1]
 */
int SC68file_save_file(const char * fname, const disk68_t * mb, int gzip);

/** Save SC68 disk into memory buffer.
 *
 *  @param  buffer destination buffer.
 *  @param  len    size of destination buffer.
 *  @param  mb     pointer to SC68 disk to save
 *  @param  gzip   gzip compression level [0:no-gzip, 1..9 or -1]
 */
int SC68file_save_mem(const char * buffer, int len, const disk68_t * mb,
		      int gzip);

/**@}*/

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _FILE68_H_ */
