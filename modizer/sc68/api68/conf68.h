/**
 * @ingroup   api68_devel
 * @file      api68/conf68.h
 * @author    Benjamin Gerard<ben@sashipa.com>
 * @date      1999/07/27
 * @brief     configuration file.
 *
 * $Id: conf68.h 503 2005-06-24 08:52:56Z loke $
 */

#ifndef _CONF68_H_
#define _CONF68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup  api68_conf  configuration file
 *  @ingroup   api68_devel
 *
 *  This module prodives functions to access sc68 configuration file.
 *
 *  @{
 */

/** Config entry types. */
typedef enum {
  SC68CONFIG_UNDEFINED = 0, /** Value is not set.     */
  SC68CONFIG_INT,           /** Value is an integer.  */
  SC68CONFIG_STRING,        /** Value is a string.    */
  SC68CONFIG_ERROR = -1     /** Reserved for errors.  */
} SC68config_type_t;
 
/** Config. */
typedef struct _SC68config_s SC68config_t;

/** Check and correct config values.
 */
int SC68config_valid(SC68config_t * conf);

/** Get index of named config entry.
 *
 *  @param  conf  config.
 *  @param  name  name of entry.
 *
 *  @return index
 *  @retval -1 error
 */
int SC68config_get_idx(const SC68config_t * conf,
		       const char * name);

/** Get type and range of a config entry.
 *
 *  @param  conf  config.
 *  @param  idx   index.
 *  @param  min   store min value (0 ignore).
 *  @param  max   store max value (0 ignore).
 *  @param  def   store default value (0 ignore).
 *
 *  @return type of config entry
 *  @retval SC68CONFIG_ERROR on error
 */
SC68config_type_t SC68config_range(const SC68config_t * conf, int idx,
                                   int * min, int * max, int * def);

/** Get value of a config entry.
 *
 *  @param  conf  config.
 *  @param  v     input: pointer to index; output: integer value or unmodified.
 *  @param  name  input: pointer to name; output: string value or unmodified.
 *
 *  @return type of config entry
 *  @retval SC68CONFIG_ERROR on error
 */
SC68config_type_t SC68config_get(const SC68config_t * conf,
				 int * v,
				 const char ** name);

/** Set value of a config entry.
 *
 *  @param  conf  config.
 *  @param  idx   index of config entry used only if name not found.
 *  @param  name  pointer to name (0:use idx).
 *  @param  v     value used to set SC68CONFIG_INT entry.
 *  @param  s     value used to set SC68CONFIG_STRING entry.
 *
 *  @return type of config entry
 *  @retval SC68CONFIG_ERROR on error
 */
SC68config_type_t SC68config_set(SC68config_t * conf,
				 int idx,
				 const char * name,
				 int v,
				 const char * s);

/** Load config from file.
 */
int SC68config_load(SC68config_t * conf);

/** Save config into file.
 */
int SC68config_save(SC68config_t * conf);

/** Fill config struct with default value.
 */
int SC68config_default(SC68config_t * conf);

/** Create config. */
SC68config_t * SC68config_create(int size);

/** Destroy config. */
void SC68config_destroy(SC68config_t * conf);

/**
 *  @}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _CONF68_H_ */
