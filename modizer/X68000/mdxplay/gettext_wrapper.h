/*
  gettext_wrapper.h  gettext wrapper

  Made by Daisuke Nagano <breeze_nagano@nifty.ne.jp>
  Mar.29.1998 

 */

#ifndef _GETTEXT_WRAPPER_H_
#define _GETTEXT_WRAPPER_H_

#ifdef ENABLE_NLS
# include <locale.h>
# include <libintl.h>
# define _(string) gettext(string)

#else /* ENABLE_NLS */
# define _(string) string
#endif /* ENABLE_NLS */

#endif /* _GETTEXT_WRAPPER_H_ */

