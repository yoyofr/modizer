/**
 * @ingroup   file68_registry68_devel
 * @file      file68/registry68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      2003/08/11
 * @brief     Registry header
 *
 * $Id: registry68.h 503 2005-06-24 08:52:56Z loke $
 *
 */

/* Copyright (C) 1998-2003 Benjamin Gerard */

#ifndef _REGISTRY68_H_
#define _REGISTRY68_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup   file68_registry68_devel  Registry
 *  @ingroup    file68_devel
 *
 *    Provides registry access functions.
 *
 *  @{
 */

/** Registry key type (override Microsoft HKEY type) */
typedef void * registry68_key_t;

/** @name  Reserved registry key handles.
 *  @{
 */
extern const registry68_key_t registry68_CRK; /**< aka HKEY_CLASSES_ROOT   */
extern const registry68_key_t registry68_CCK; /**< aka HKEY_CURRENT_CONFIG */
extern const registry68_key_t registry68_CUK; /**< aka HKEY_CURRENT_USER   */
extern const registry68_key_t registry68_LMK; /**< aka HKEY_LOCAL_MACHINE  */
extern const registry68_key_t registry68_UK;  /**< aka HKEY_USERS          */
extern const registry68_key_t registry68_IK;  /**< Invalid key             */
/**@}*/

/** Last error message. */
extern char registry68_errorstr[];

/** Open a named hierarchic key.
 *
 *  @param hkey     Opened key handle or one of reserved registry key handles.
 *  @param kname    Hierarchic key name. Slash '/' caractere is interpreted
 *                  as sub-key separator.
 *
 *  @return Registry key handle
 *  @retval registry68InvalidKey Error
 */
registry68_key_t registry68_open(registry68_key_t hkey,
				 char *kname);

/** Get value of a named hierarchic string key.
 *
 *  @param hkey     Opened key handle or one of reserved registry key handles.
 *  @param kname    Hierarchic key name. Slash '/' caractere is interpreted
 *                  as sub-key separator.
 *  @param kdata    Returned string storage location
 *  @param kdatasz  Maximum size of kdata buffer.
 *
 *  @return ErrorNo
 *  @retval 0  Success
 *  @retval <0 Error
 */
int registry68_gets(registry68_key_t hkey,
		    const char *kname,
		    char *kdata,
		    int kdatasz);

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _REGISTRY68_H_ */
