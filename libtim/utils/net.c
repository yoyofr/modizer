/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#if !defined(WINSOCK)
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>  /* for inet_ntoa */
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#endif

#include "timidity.h"
#include "common.h"
#include "net.h"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif /* INADDR_NONE */

#if !defined(WINSOCK)
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif /* INVALID_SOCKET */
#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif /* SOCKET_ERROR */
#endif

#if (defined WIN32) && (_WIN32_WINNT < 0x0501)
typedef int (WSAAPI * GETADDRINFO) (const char FAR *, const char FAR *,
                                          const struct addrinfo FAR *,
                                          struct addrinfo FAR * FAR *);

typedef void (WSAAPI * FREEADDRINFO) ( struct addrinfo FAR * );
extern GETADDRINFO ws2_getaddrinfo;
extern FREEADDRINFO ws2_freeaddrinfo;
#define getaddrinfo ws2_getaddrinfo
#define freeaddrinfo ws2_freeaddrinfo
#endif

SOCKET open_socket(char *host, unsigned short port)
{
    SOCKET fd;
    struct addrinfo hints, *result, *rp;
    char service[NI_MAXSERV];
    int s;

#if defined(WINSOCK)
    static int first = 1;
    if(first) {
	WSADATA dmy;
	WSAStartup(MAKEWORD(1,1), &dmy);
	first = 0;
    }
#endif

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
#ifdef AI_ADDRCONFIG
    /* By default, only look up addresses using address types for
     * which a local interface is configured (i.e. no IPv6 if no IPv6
     * interfaces. */
    hints.ai_flags = AI_ADDRCONFIG;
#endif

    snprintf(service, sizeof(service), "%d", port);

    s = getaddrinfo(host, service, &hints, &result);
#ifdef AI_ADDRCONFIG
    if (s == EAI_BADFLAGS) {
        /* Retry with no flags if AI_ADDRCONFIG was rejected. */
        hints.ai_flags = 0;
        s = getaddrinfo(host, service, &hints, &result);
    }
#endif

    if (s)
        return (SOCKET)-1;

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        if (rp->ai_family != AF_INET && rp->ai_family != AF_INET6)
            continue;

        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (fd != -1 && connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;

        if (fd != -1) {
            close(fd);
            fd = -1;
        }
    }

    freeaddrinfo(result);
    return (SOCKET) fd;
}

#if !defined(__W32__) || defined(__CYGWIN32__)
long socket_write(SOCKET fd, char *buff, long bufsiz)
{
    return write(fd, buff, bufsiz);
}

int socket_shutdown(SOCKET fd, int how)
{
    return shutdown(fd, how);
}

FILE *socket_fdopen(SOCKET fd, char *mode)
{
    return fdopen(fd, mode);
}

char *socket_fgets(char *buff, int n, FILE *fp)
{
    return fgets(buff, n, fp);
}

int socket_fgetc(FILE *fp)
{
    return getc(fp);
}

long socket_fread(void *buff, long bufsiz, FILE *fp)
{
    return (long)fread(buff, 1, bufsiz, fp);
}

long socket_fwrite(void *buff, long bufsiz, FILE *fp)
{
    return (long)fwrite(buff, 1, bufsiz, fp);
}

int socket_fclose(FILE *fp)
{
    return fclose(fp);
}

int socket_fflush(FILE *fp)
{
    return fflush(fp);
}
#else /* __W32__ */

#include "url.h"

/* Fake FILE * */

typedef struct _w32_fp_socket_t
{
    SOCKET fd; /* Now, It has only fd */
} w32_fp_socket;
#define W32_FP2SOCKET(fp) (((w32_fp_socket *)(fp))->fd)

static long socket_safe_recv(SOCKET fd, char *buff, long bufsiz)
{
    fd_set fds;
    int maxfd, n;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    maxfd = fd + 1;

    n = select(maxfd, &fds, NULL, NULL, NULL);
    if(n <= 0)
	return n;
    return recv(fd, buff, bufsiz, 0);
}

long socket_write(SOCKET fd, char *buff, long bufsiz)
{
    return send(fd, buff, bufsiz, 0);
}

int socket_shutdown(SOCKET fd, int how)
{
    return shutdown(fd, how);
}

#pragma argsused
FILE *socket_fdopen(SOCKET fd, char *mode)
{
    FILE *fp;

    /* w32_fp_socket fake FILE.
     * `mode' argument is ignored.
     */
    if((fp = (FILE *)safe_malloc(sizeof(w32_fp_socket))) == NULL)
	return NULL;
    memset(fp, 0, sizeof(w32_fp_socket));
    W32_FP2SOCKET(fp) = fd;
    return fp;
}

static int socket_getc(SOCKET fd)
{
    int n;
    unsigned char c;
    
    n = socket_safe_recv(fd, (char *)&c, 1);
    if(n <= 0)
	return EOF;
    return (int)c;
}

char *socket_fgets(char *buff, int n, FILE *fp)
{
    SOCKET fd = W32_FP2SOCKET(fp);
    int len;

    n--; /* for '\0' */
    if(n == 0)
	*buff = '\0';
    if(n <= 0)
	return buff;

    len = 0;
    while(len < n)
    {
	int c;
	c = socket_getc(fd);
	if(c == -1)
	    break;
	buff[len++] = c;
	if(c == url_newline_code)
	    break;
    }
    if(len == 0)
	return NULL;
    buff[len] = '\0';
    return buff;
}

int socket_fgetc(FILE *fp)
{
    SOCKET fd = W32_FP2SOCKET(fp);
    return socket_getc(fd);
}

long socket_fread(void *buff, long bufsiz, FILE *fp)
{
    SOCKET fd = W32_FP2SOCKET(fp);
    return socket_safe_recv(fd, buff, bufsiz);
}

long socket_fwrite(void *buff, long bufsiz, FILE *fp)
{
    SOCKET fd = W32_FP2SOCKET(fp);
    return socket_write(fd, buff, bufsiz);
}

int socket_fclose(FILE *fp)
{
    SOCKET fd = W32_FP2SOCKET(fp);
    int ret;
    ret = closesocket(fd);
    free(fp);
    return ret;
}

int socket_fflush(FILE *fp)
{
    return 0;
}

#endif
