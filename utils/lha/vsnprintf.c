/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * IMPORTANT NOTE:
 * --------------
 * From ftp://ftp.cs.berkeley.edu/pub/4bsd/README.Impt.License.Change
 * paragraph 3 above is now null and void.
 */

/* SNPRINTF.C  
 * fjc 7-31-97 Modified by Mib Software to be a standalone snprintf.c module.
 *      http://www.mibsoftware.com
 * Mib Software does not warrant this software any differently than the
 * University of California, Berkeley as described above.  All warranties
 * are disclaimed.  Use this software at your own risk.
 *
 *      All code referencing FILE * functions was eliminated, since it could
 *      never be called.  All header files and necessary files are collapsed
 *      into one file, internal functions are declared static.  This should
 *      allow inclusion into libraries with less chance of namespace collisions.
 *
 *      snprintf should be the only externally visible item.
 *     
 *      As of 7-31-97 FLOATING_POINT is NOT provided.  The code is somewhat
 *        non-portable, so it is disabled.
 */

/* Define FLOATING_POINT to get floating point. */
/*
#define	FLOATING_POINT
*/

#include <sys/types.h>
#define u_long unsigned long
#define u_short unsigned short
#define u_int unsigned int

#undef __P
#if defined(__STDC__)
# include <stdarg.h>
# if !defined(__P)
#  define __P(x) x
# endif
#else
# define __P(x) ()
# if !defined(const)
#  define const
# endif
# include <varargs.h>
#endif
#ifndef _BSD_VA_LIST_ 
#define	_BSD_VA_LIST_ va_list
#endif

#ifdef __STDC__
# include <limits.h>
#else
# ifndef LONG_MAX
#  ifdef HAVE_LIMITS_H
#   include <limits.h>
#  else
    /* assuming 32bit(2's compliment) long */
#   define LONG_MAX 2147483647
#  endif
# endif
#endif

#if defined(__hpux) && !defined(__GNUC__)
#define const
#endif

#if defined(sgi)
#undef __const
#define __const
#endif /* People who don't like const sys_error */

#include <stddef.h>

#include <string.h>

#ifndef NULL
#define	NULL	0
#endif

/*
 * NB: to fit things in six character monocase externals, the stdio
 * code uses the prefix `__s' for stdio objects, typically followed
 * by a three-character attempt at a mnemonic.
 */

/* stdio buffers */
struct __sbuf {
	unsigned char *_base;
	int	_size;
};


/*
 * stdio state variables.
 *
 * The following always hold:
 *
 *	if (_flags&(__SLBF|__SWR)) == (__SLBF|__SWR),
 *		_lbfsize is -_bf._size, else _lbfsize is 0
 *	if _flags&__SRD, _w is 0
 *	if _flags&__SWR, _r is 0
 *
 * This ensures that the getc and putc macros (or inline functions) never
 * try to write or read from a file that is in `read' or `write' mode.
 * (Moreover, they can, and do, automatically switch from read mode to
 * write mode, and back, on "r+" and "w+" files.)
 *
 * _lbfsize is used only to make the inline line-buffered output stream
 * code as compact as possible.
 *
 * _ub, _up, and _ur are used when ungetc() pushes back more characters
 * than fit in the current _bf, or when ungetc() pushes back a character
 * that does not match the previous one in _bf.  When this happens,
 * _ub._base becomes non-nil (i.e., a stream has ungetc() data iff
 * _ub._base!=NULL) and _up and _ur save the current values of _p and _r.
 *
 * NB: see WARNING above before changing the layout of this structure!
 */
typedef	struct __sFILE {
	unsigned char *_p;	/* current position in (some) buffer */
	int	_r;		/* read space left for getc() */
	int	_w;		/* write space left for putc() */
	short	_flags;		/* flags, below; this FILE is free if 0 */
	short	_file;		/* fileno, if Unix descriptor, else -1 */
	struct	__sbuf _bf;	/* the buffer (at least 1 byte, if !NULL) */
	int	_lbfsize;	/* 0 or -_bf._size, for inline putc */
} FILE;


#define	__SLBF	0x0001		/* line buffered */
#define	__SNBF	0x0002		/* unbuffered */
#define	__SRD	0x0004		/* OK to read */
#define	__SWR	0x0008		/* OK to write */
	/* RD and WR are never simultaneously asserted */
#define	__SRW	0x0010		/* open for reading & writing */
#define	__SEOF	0x0020		/* found EOF */
#define	__SERR	0x0040		/* found error */
#define	__SMBF	0x0080		/* _buf is from malloc */
#define	__SAPP	0x0100		/* fdopen()ed in append mode */
#define	__SSTR	0x0200		/* this is an sprintf/snprintf string */
#define	__SOPT	0x0400		/* do fseek() optimisation */
#define	__SNPT	0x0800		/* do not do fseek() optimisation */
#define	__SOFF	0x1000		/* set iff _offset is in fact correct */
#define	__SMOD	0x2000		/* true => fgetln modified _p text */


#define	EOF	(-1)


#define	__sfeof(p)	(((p)->_flags & __SEOF) != 0)
#define	__sferror(p)	(((p)->_flags & __SERR) != 0)
#define	__sclearerr(p)	((void)((p)->_flags &= ~(__SERR|__SEOF)))
#define	__sfileno(p)	((p)->_file)

#define	feof(p)		__sfeof(p)
#define	ferror(p)	__sferror(p)
#define	clearerr(p)	__sclearerr(p)

#ifndef _ANSI_SOURCE
#define	fileno(p)	__sfileno(p)
#endif


#if defined(__hpux) && !defined(__GNUC__) || defined(__DECC)
#include <string.h>
#endif

/*
 * I/O descriptors for __sfvwrite().
 */
struct __siov {
	void	*iov_base;
	size_t	iov_len;
};
struct __suio {
	struct	__siov *uio_iov;
	int	uio_iovcnt;
	int	uio_resid;
};

/*
 * Write some memory regions.  Return zero on success, EOF on error.
 *
 * This routine is large and unsightly, but most of the ugliness due
 * to the three different kinds of output buffering is handled here.
 */
static int BSD__sfvwrite(fp, uio)
	register FILE *fp;
	register struct __suio *uio;
{
	register size_t len;
	register char *p;
	register struct __siov *iov;
	register int w;

	if ((len = uio->uio_resid) == 0)
		return (0);
#ifndef __hpux
#define	MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#define	COPY(n)	  (void)memcpy((void *)fp->_p, (void *)p, (size_t)(n))

	iov = uio->uio_iov;
	p = iov->iov_base;
	len = iov->iov_len;
	iov++;
#define GETIOV(extra_work) \
	while (len == 0) { \
		extra_work; \
		p = iov->iov_base; \
		len = iov->iov_len; \
		iov++; \
	}
	if (fp->_flags & __SNBF) {
	/* fjc 7-31-97 Will never happen.  We are working with
					   strings only
	*/
	} else if ((fp->_flags & __SLBF) == 0) {
	/*
		 * Fully buffered: fill partially full buffer, if any,
		 * and then flush.  If there is no partial buffer, write
		 * one _bf._size byte chunk directly (without copying).
		 *
		 * String output is a special case: write as many bytes
		 * as fit, but pretend we wrote everything.  This makes
		 * snprintf() return the number of bytes needed, rather
		 * than the number used, and avoids its write function
		 * (so that the write function can be invalid).
		 */
		do {
			GETIOV(;);
			w = fp->_w;
			if (fp->_flags & __SSTR) {
				if (len < w)
					w = len;
				COPY(w);	/* copy MIN(fp->_w,len), */
				fp->_w -= w;
				fp->_p += w;
				w = len;	/* but pretend copied all */
			} else {
				/* fjc 7-31-97 Will never happen.  We are working with
								   strings only
				*/
			}
			p += w;
			len -= w;
		} while ((uio->uio_resid -= w) != 0);
	} else {
		/* fjc 7-31-97 Will never happen.  We are working with
						   strings only
		*/
	}
	return (0);

err:
	fp->_flags |= __SERR;
	return (EOF);
}

/*
 * Actual printf innards.
 *
 * This code is large and complicated...
 */

#if !defined(__CYGWIN32__) && defined(__hpux) && !defined(__GNUC__)
#include <stdlib.h>
#endif

/*
 * Flush out all the vectors defined by the given uio,
 * then reset it so that it can be reused.
 */
static int
BSD__sprint(fp, uio)
	FILE *fp;
	register struct __suio *uio;
{
	register int err;

	if (uio->uio_resid == 0) {
		uio->uio_iovcnt = 0;
		return (0);
	}
	err = BSD__sfvwrite(fp, uio);
	uio->uio_resid = 0;
	uio->uio_iovcnt = 0;
	return (err);
}


/*
 * Helper function for `fprintf to unbuffered unix file': creates a
 * temporary buffer.  We only work on write-only files; this avoids
 * worries about ungetc buffers and so forth.
 */
static int
BSD__sbprintf(fp, fmt, ap)
	register FILE *fp;
	const char *fmt;
	va_list ap;
{
/* We don't support files. */
	return 0;
}


/*
 * Macros for converting digits to letters and vice versa
 */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

/*
 * Convert an unsigned long to ASCII for printf purposes, returning
 * a pointer to the first character of the string representation.
 * Octal numbers can be forced to have a leading zero; hex numbers
 * use the given digits.
 */
static char *
BSD__ultoa(val, endp, base, octzero, xdigs)
	register u_long val;
	char *endp;
	int base, octzero;
	char *xdigs;
{
	register char *cp = endp;
	register long sval;

	/*
	 * Handle the three cases separately, in the hope of getting
	 * better/faster code.
	 */
	switch (base) {
	case 10:
		if (val < 10) {	/* many numbers are 1 digit */
			*--cp = to_char(val);
			return (cp);
		}
		/*
		 * On many machines, unsigned arithmetic is harder than
		 * signed arithmetic, so we do at most one unsigned mod and
		 * divide; this is sufficient to reduce the range of
		 * the incoming value to where signed arithmetic works.
		 */
		if (val > LONG_MAX) {
			*--cp = to_char(val % 10);
			sval = val / 10;
		} else
			sval = val;
		do {
			*--cp = to_char(sval % 10);
			sval /= 10;
		} while (sval != 0);
		break;

	case 8:
		do {
			*--cp = to_char(val & 7);
			val >>= 3;
		} while (val);
		if (octzero && *cp != '0')
			*--cp = '0';
		break;

	case 16:
		do {
			*--cp = xdigs[val & 15];
			val >>= 4;
		} while (val);
		break;

	default:			/* oops */
		/* 
		abort();
		*/
		break;	/* fjc 7-31-97.  Don't reference abort() here */
	}
	return (cp);
}

#ifdef FLOATING_POINT
#include <math.h>
/* #include "floatio.h" */

#ifndef MAXEXP
# define MAXEXP 1024
#endif

#ifndef MAXFRACT
# define MAXFRACT 64
#endif

#define	BUF		(MAXEXP+MAXFRACT+1)	/* + decimal point */
#define	DEFPREC		6

static char *cvt __P((double, int, int, char *, int *, int, int *));
static int exponent __P((char *, int, int));

#else /* no FLOATING_POINT */

#define	BUF		68

#endif /* FLOATING_POINT */


/*
 * Flags used during conversion.
 */
#define	ALT		0x001		/* alternate form */
#define	HEXPREFIX	0x002		/* add 0x or 0X prefix */
#define	LADJUST		0x004		/* left adjustment */
#define	LONGDBL		0x008		/* long double; unimplemented */
#define	LONGINT		0x010		/* long integer */

#ifdef _HAVE_SANE_QUAD_
#define	QUADINT		0x020		/* quad integer */
#endif /* _HAVE_SANE_QUAD_ */

#define	SHORTINT	0x040		/* short integer */
#define	ZEROPAD		0x080		/* zero (as opposed to blank) pad */
#define FPT		0x100		/* Floating point number */
static int
BSD_vfprintf(fp, fmt0, ap)
	FILE *fp;
	const char *fmt0;
	va_list ap;
{
	register char *fmt;	/* format string */
	register int ch;	/* character from fmt */
	register int n;		/* handy integer (short term usage) */
	register char *cp;	/* handy char pointer (short term usage) */
	register struct __siov *iovp;/* for PRINT macro */
	register int flags;	/* flags as above */
	int ret;		/* return value accumulator */
	int width;		/* width from format (%8d), or 0 */
	int prec;		/* precision from format (%.3d), or -1 */
	char sign;		/* sign prefix (' ', '+', '-', or \0) */
#ifdef FLOATING_POINT
	char softsign;		/* temporary negative sign for floats */
	double _double;		/* double precision arguments %[eEfgG] */
	int expt;		/* integer value of exponent */
	int expsize;		/* character count for expstr */
	int ndig;		/* actual number of digits returned by cvt */
	char expstr[7];		/* buffer for exponent string */
#endif
	u_long	ulval;		/* integer arguments %[diouxX] */
#ifdef _HAVE_SANE_QUAD_
	u_quad_t uqval;		/* %q integers */
#endif /* _HAVE_SANE_QUAD_ */
	int base;		/* base for [diouxX] conversion */
	int dprec;		/* a copy of prec if [diouxX], 0 otherwise */
	int fieldsz;		/* field size expanded by sign, etc */
	int realsz;		/* field size expanded by dprec */
	int size;		/* size of converted field or string */
	char *xdigs;		/* digits for [xX] conversion */
#define NIOV 8
	struct __suio uio;	/* output information: summary */
	struct __siov iov[NIOV];/* ... and individual io vectors */
	char buf[BUF];		/* space for %c, %[diouxX], %[eEfgG] */
	char ox[2];		/* space for 0x hex-prefix */

	/*
	 * Choose PADSIZE to trade efficiency vs. size.  If larger printf
	 * fields occur frequently, increase PADSIZE and make the initialisers
	 * below longer.
	 */
#define	PADSIZE	16		/* pad chunk size */
	static char blanks[PADSIZE] =
	 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
	static char zeroes[PADSIZE] =
	 {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};

	/*
	 * BEWARE, these `goto error' on error, and PAD uses `n'.
	 */
#define	PRINT(ptr, len) { \
	iovp->iov_base = (ptr); \
	iovp->iov_len = (len); \
	uio.uio_resid += (len); \
	iovp++; \
	if (++uio.uio_iovcnt >= NIOV) { \
		if (BSD__sprint(fp, &uio)) \
			goto error; \
		iovp = iov; \
	} \
}
#define	PAD(howmany, with) { \
	if ((n = (howmany)) > 0) { \
		while (n > PADSIZE) { \
			PRINT(with, PADSIZE); \
			n -= PADSIZE; \
		} \
		PRINT(with, n); \
	} \
}
#define	FLUSH() { \
	if (uio.uio_resid && BSD__sprint(fp, &uio)) \
		goto error; \
	uio.uio_iovcnt = 0; \
	iovp = iov; \
}

	/*
	 * To extend shorts properly, we need both signed and unsigned
	 * argument extraction methods.
	 */
#define	SARG() \
	(flags&LONGINT ? va_arg(ap, long) : \
	    flags&SHORTINT ? (long)(short)va_arg(ap, int) : \
	    (long)va_arg(ap, int))
#define	UARG() \
	(flags&LONGINT ? va_arg(ap, u_long) : \
	    flags&SHORTINT ? (u_long)(u_short)va_arg(ap, int) : \
	    (u_long)va_arg(ap, u_int))

	/* optimise fprintf(stderr) (and other unbuffered Unix files) */
	if ((fp->_flags & (__SNBF|__SWR|__SRW)) == (__SNBF|__SWR) &&
	    fp->_file >= 0)
		return (BSD__sbprintf(fp, fmt0, ap));

	fmt = (char *)fmt0;
	uio.uio_iov = iovp = iov;
	uio.uio_resid = 0;
	uio.uio_iovcnt = 0;
	ret = 0;

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++)
			/* void */;
		if ((n = fmt - cp) != 0) {
			PRINT(cp, n);
			ret += n;
		}
		if (ch == '\0')
			goto done;
		fmt++;		/* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		case ' ':
			/*
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '#':
			flags |= ALT;
			goto rflag;
		case '*':
			/*
			 * ``A negative field width argument is taken as a
			 * - flag followed by a positive field width.''
			 *	-- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			if ((width = va_arg(ap, int)) >= 0)
				goto rflag;
			width = -width;
			/* FALLTHROUGH */
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				n = va_arg(ap, int);
				prec = n < 0 ? -1 : n;
				goto rflag;
			}
			n = 0;
			while (is_digit(ch)) {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			}
			prec = n < 0 ? -1 : n;
			goto reswitch;
		case '0':
			/*
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			} while (is_digit(ch));
			width = n;
			goto reswitch;
#ifdef FLOATING_POINT
		case 'L':
			flags |= LONGDBL;
			goto rflag;
#endif
		case 'h':
			flags |= SHORTINT;
			goto rflag;
		case 'l':
			flags |= LONGINT;
			goto rflag;
#ifdef _HAVE_SANE_QUAD_
		case 'q':
			flags |= QUADINT;
			goto rflag;
#endif /* _HAVE_SANE_QUAD_ */
		case 'c':
			*(cp = buf) = va_arg(ap, int);
			size = 1;
			sign = '\0';
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
#ifdef _HAVE_SANE_QUAD_
			if (flags & QUADINT) {
				uqval = va_arg(ap, quad_t);
				if ((quad_t)uqval < 0) {
					uqval = -uqval;
					sign = '-';
				}
			} else {
#else /* _HAVE_SANE_QUAD_ */
			{
#endif /* _HAVE_SANE_QUAD_ */
				ulval = SARG();
				if ((long)ulval < 0) {
					ulval = -ulval;
					sign = '-';
				}
			}
			base = 10;
			goto number;
#ifdef FLOATING_POINT
		case 'e':		/* anomalous precision */
		case 'E':
			prec = (prec == -1) ?
				DEFPREC + 1 : prec + 1;
			/* FALLTHROUGH */
			goto fp_begin;
		case 'f':		/* always print trailing zeroes */
			if (prec != 0)
				flags |= ALT;
		case 'g':
		case 'G':
			if (prec == -1)
				prec = DEFPREC;
fp_begin:		_double = va_arg(ap, double);
			/* do this before tricky precision changes */
			if (isinf(_double)) {
				if (_double < 0)
					sign = '-';
				cp = "Inf";
				size = 3;
				break;
			}
			if (isnan(_double)) {
				cp = "NaN";
				size = 3;
				break;
			}
			flags |= FPT;
			cp = cvt(_double, prec, flags, &softsign,
				&expt, ch, &ndig);
			if (ch == 'g' || ch == 'G') {
				if (expt <= -4 || expt > prec)
					ch = (ch == 'g') ? 'e' : 'E';
				else
					ch = 'g';
			} 
			if (ch <= 'e') {	/* 'e' or 'E' fmt */
				--expt;
				expsize = exponent(expstr, expt, ch);
				size = expsize + ndig;
				if (ndig > 1 || flags & ALT)
					++size;
			} else if (ch == 'f') {		/* f fmt */
				if (expt > 0) {
					size = expt;
					if (prec || flags & ALT)
						size += prec + 1;
				} else	/* "0.X" */
					size = prec + 2;
			} else if (expt >= ndig) {	/* fixed g fmt */
				size = expt;
				if (flags & ALT)
					++size;
			} else
				size = ndig + (expt > 0 ?
					1 : 2 - expt);

			if (softsign)
				sign = '-';
			break;
#endif /* FLOATING_POINT */
		case 'n':
#ifdef _HAVE_SANE_QUAD_
			if (flags & QUADINT)
				*va_arg(ap, quad_t *) = ret;
			else if (flags & LONGINT)
#else /* _HAVE_SANE_QUAD_ */
			if (flags & LONGINT)
#endif /* _HAVE_SANE_QUAD_ */
				*va_arg(ap, long *) = ret;
			else if (flags & SHORTINT)
				*va_arg(ap, short *) = ret;
			else
				*va_arg(ap, int *) = ret;
			continue;	/* no output */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
#ifdef _HAVE_SANE_QUAD_
			if (flags & QUADINT)
				uqval = va_arg(ap, u_quad_t);
			else
#endif /* _HAVE_SANE_QUAD_ */
				ulval = UARG();
			base = 8;
			goto nosign;
		case 'p':
			/*
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */
			ulval = (u_long)va_arg(ap, void *);
			base = 16;
			xdigs = "0123456789abcdef";
#ifdef _HAVE_SANE_QUAD_
			flags = (flags & ~QUADINT) | HEXPREFIX;
#else /* _HAVE_SANE_QUAD_ */
			flags = (flags) | HEXPREFIX;
#endif /* _HAVE_SANE_QUAD_ */
			ch = 'x';
			goto nosign;
		case 's':
			if ((cp = va_arg(ap, char *)) == NULL)
				cp = "(null)";
			if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
				char *p = (char *)memchr(cp, 0, prec);

				if (p != NULL) {
					size = p - cp;
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else
				size = strlen(cp);
			sign = '\0';
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
#ifdef _HAVE_SANE_QUAD_
			if (flags & QUADINT)
				uqval = va_arg(ap, u_quad_t);
			else
#endif /* _HAVE_SANE_QUAD_ */
				ulval = UARG();
			base = 10;
			goto nosign;
		case 'X':
			xdigs = "0123456789ABCDEF";
			goto hex;
		case 'x':
			xdigs = "0123456789abcdef";
hex:
#ifdef _HAVE_SANE_QUAD_
			if (flags & QUADINT)
				uqval = va_arg(ap, u_quad_t);
			else
#endif /* _HAVE_SANE_QUAD_ */
				ulval = UARG();
			base = 16;
			/* leading 0x/X only if non-zero */
			if (flags & ALT &&
#ifdef _HAVE_SANE_QUAD_
			    (flags & QUADINT ? uqval != 0 : ulval != 0))
#else /* _HAVE_SANE_QUAD_ */
			    ulval != 0)
#endif /* _HAVE_SANE_QUAD_ */
				flags |= HEXPREFIX;

			/* unsigned conversions */
nosign:			sign = '\0';
			/*
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:			if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 */
			cp = buf + BUF;
#ifdef _HAVE_SANE_QUAD_
			if (flags & QUADINT) {
				if (uqval != 0 || prec != 0)
					cp = __uqtoa(uqval, cp, base,
					    flags & ALT, xdigs);
			} else {
#else /* _HAVE_SANE_QUAD_ */
			{
#endif /* _HAVE_SANE_QUAD_ */
				if (ulval != 0 || prec != 0)
					cp = BSD__ultoa(ulval, cp, base,
					    flags & ALT, xdigs);
			}
			size = buf + BUF - cp;
			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			/* pretend it was %c with argument ch */
			cp = buf;
			*cp = ch;
			size = 1;
			sign = '\0';
			break;
		}

		/*
		 * All reasonable formats wind up here.  At this point, `cp'
		 * points to a string which (if not flags&LADJUST) should be
		 * padded out to `width' places.  If flags&ZEROPAD, it should
		 * first be prefixed by any sign or other prefix; otherwise,
		 * it should be blank padded before the prefix is emitted.
		 * After any left-hand padding and prefixing, emit zeroes
		 * required by a decimal [diouxX] precision, then print the
		 * string proper, then emit zeroes required by any leftover
		 * floating precision; finally, if LADJUST, pad with blanks.
		 *
		 * Compute actual size, so we know how much to pad.
		 * fieldsz excludes decimal prec; realsz includes it.
		 */
		fieldsz = size;
		if (sign)
			fieldsz++;
		else if (flags & HEXPREFIX)
			fieldsz += 2;
		realsz = dprec > fieldsz ? dprec : fieldsz;

		/* right-adjusting blank padding */
		if ((flags & (LADJUST|ZEROPAD)) == 0)
			PAD(width - realsz, blanks);

		/* prefix */
		if (sign) {
			PRINT(&sign, 1);
		} else if (flags & HEXPREFIX) {
			ox[0] = '0';
			ox[1] = ch;
			PRINT(ox, 2);
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD)
			PAD(width - realsz, zeroes);

		/* leading zeroes from decimal precision */
		PAD(dprec - fieldsz, zeroes);

		/* the string or number proper */
#ifdef FLOATING_POINT
		if ((flags & FPT) == 0) {
			PRINT(cp, size);
		} else {	/* glue together f_p fragments */
			if (ch >= 'f') {	/* 'f' or 'g' */
				if (_double == 0) {
				/* kludge for __dtoa irregularity */
					if (prec == 0 ||
					    (flags & ALT) == 0) {
						PRINT("0", 1);
					} else {
						PRINT("0.", 2);
						PAD(ndig - 1, zeroes);
					}
				} else if (expt <= 0) {
					PRINT("0.", 2);
					PAD(-expt, zeroes);
					PRINT(cp, ndig);
				} else if (expt >= ndig) {
					PRINT(cp, ndig);
					PAD(expt - ndig, zeroes);
					if (flags & ALT)
						PRINT(".", 1);
				} else {
					PRINT(cp, expt);
					cp += expt;
					PRINT(".", 1);
					PRINT(cp, ndig-expt);
				}
			} else {	/* 'e' or 'E' */
				if (ndig > 1 || flags & ALT) {
					ox[0] = *cp++;
					ox[1] = '.';
					PRINT(ox, 2);
					if (_double || flags & ALT == 0) {
						PRINT(cp, ndig-1);
					} else	/* 0.[0..] */
						/* __dtoa irregularity */
						PAD(ndig - 1, zeroes);
				} else	/* XeYYY */
					PRINT(cp, 1);
				PRINT(expstr, expsize);
			}
		}
#else
		PRINT(cp, size);
#endif
		/* left-adjusting padding (always blank) */
		if (flags & LADJUST)
			PAD(width - realsz, blanks);

		/* finally, adjust ret */
		ret += width > realsz ? width : realsz;

		FLUSH();	/* copy out the I/O vectors */
	}
done:
	FLUSH();
error:
	return (__sferror(fp) ? EOF : ret);
	/* NOTREACHED */
}

#ifdef FLOATING_POINT

extern char *BSD__dtoa __P((double, int, int, int *, int *, char **));

static char *
cvt(value, ndigits, flags, sign, decpt, ch, length)
	double value;
	int ndigits, flags, *decpt, ch, *length;
	char *sign;
{
	int mode, dsgn;
	char *digits, *bp, *rve;

	if (ch == 'f')
		mode = 3;
	else {
		mode = 2;
	}
	if (value < 0) {
		value = -value;
		*sign = '-';
	} else
		*sign = '\000';
	digits = BSD__dtoa(value, mode, ndigits, decpt, &dsgn, &rve);
	if (flags & ALT) {	/* Print trailing zeros */
		bp = digits + ndigits;
		if (ch == 'f') {
			if (*digits == '0' && value)
				*decpt = -ndigits + 1;
			bp += *decpt;
		}
		if (value == 0)	/* kludge for __dtoa irregularity */
			rve = bp;
		while (rve < bp)
			*rve++ = '0';
	}
	*length = rve - digits;
	return (digits);
}

static int
exponent(p0, exp, fmtch)
	char *p0;
	int exp, fmtch;
{
	register char *p, *t;
	char expbuf[MAXEXP];

	p = p0;
	*p++ = fmtch;
	if (exp < 0) {
		exp = -exp;
		*p++ = '-';
	}
	else
		*p++ = '+';
	t = expbuf + MAXEXP;
	if (exp > 9) {
		do {
			*--t = to_char(exp % 10);
		} while ((exp /= 10) > 9);
		*--t = to_char(exp);
		for (; t < expbuf + MAXEXP; *p++ = *t++);
	}
	else {
		*p++ = '0';
		*p++ = to_char(exp);
	}
	return (p - p0);
}
#endif /* FLOATING_POINT */

int
vsnprintf(str, n, fmt, ap)
	char *str;
	size_t n;
	const char *fmt;
	_BSD_VA_LIST_ ap;
{
	int ret;
	FILE f;

	if ((int)n < 1)
		return (EOF);
	f._flags = __SWR | __SSTR;
	f._bf._base = f._p = (unsigned char *)str;
	f._bf._size = f._w = n - 1;
	ret = BSD_vfprintf(&f, fmt, ap);
	*f._p = 0;
	return (ret);
}

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)snprintf.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#if defined(__STDC__)
# include <stdarg.h>
#else
# include <varargs.h>
#endif

int
#if defined(__STDC__)
snprintf(char *str, size_t n, char const *fmt, ...)
#else
snprintf(str, n, fmt, va_alist)
char *str, *fmt;
size_t n;
va_dcl
#endif
{
	int ret;
	va_list ap;
	FILE f;

	if ((int)n < 1)
		return (EOF);

#if defined(__STDC__)
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	f._flags = __SWR | __SSTR;
	f._bf._base = f._p = (unsigned char *)str;
	f._bf._size = f._w = n - 1;
	ret = BSD_vfprintf(&f, fmt, ap);
	*f._p = 0;
	va_end(ap);
	return (ret);
}
