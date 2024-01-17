/**
 * @ingroup   lib_sc68
 * @file      sc68/conf68.h
 * @brief     configuration file.
 * @author    Benjamin Gerard
 * @date      1999/07/27
 */

/* Copyright (c) 1998-2016 Benjamin Gerard */

#ifndef SC68_CONF68_H
#define SC68_CONF68_H

#ifndef CONF68_API
# ifdef SC68_EXTERN
#  define CONF68_API SC68_EXTERN
# elif defined(__cplusplus)
#  define CONF68_API extern "C"
# else
#  define CONF68_API
# endif
#endif

/**
 *  @defgroup  lib_sc68_conf  Access to sc68 configuration
 *  @ingroup   lib_sc68
 *
 *  This module prodives functions to access sc68 configuration.
 *
 *  @{
 */

CONF68_API
/**
 *  Load config.
 *
 *  @param  name  name of application
 *
 *  @return error code
 *  @retval 0 on success
 *  @retval 0 on error
 */
int config68_load(const char * name);

CONF68_API
/**
 *  Save config.
 *
 *  @param  name  name of application
 *
 *  @return error code
 *  @retval 0 on success
 *  @retval 0 on error
 */
int config68_save(const char * name);


CONF68_API
/**
 *  Library one time init.
 *
 *  @param  argc  argument count.
 *  @param  argv  argument values.
 *
 *  @return number of argument remaining in argv
 *  @retval 0 on success
 *  @retval 0 on error
 */
int config68_init(int argc, char * argv[]);

CONF68_API
/**
 *  Library one time shutdown shutdown.
 */
void config68_shutdown(void);

/**
 * @}
 */

#endif
