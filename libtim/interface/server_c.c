/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    server_c.c - TiMidity server written by Masanao Izumo <iz@onicos.co.jp>
        Mon Apr 5 1999: Initial created.

    Launch TiMidity server: (example)
    % timidity -ir 7777

    Protcol note:
    The protocol is based on OSS interface.
    TiMidity server has 2 TCP/IP connection.  They are control port and
    data port.
    Control port:
        ASCII text protocol like FTP control port. See command_help for
        control protocol in this source code.
    Data port:
        One way Binary stream data from client to server. The format of
        stream is same as OSS sequencer stream.

    TODO:
        Protocol specification to be documented.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <signal.h>

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#else
#include "server_defs.h"
#endif /* HAVE_SYS_SOUNDCARD_H */

#include "timidity.h"
#include "common.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "recache.h"
#include "output.h"
#include "aq.h"
#include "timer.h"

#define SERVER_VERSION "1.0.4"

/* #define DEBUG_DUMP_SEQ 1 */
#define MIDI_COMMAND_PER_SEC	100
#define DEFAULT_LOW_TIMEAT	0.4
#define DEFAULT_HIGH_TIMEAT	0.6
#define DEFAULT_TIMEBASE	100 /* HZ? */
#define SIG_TIMEOUT_SEC		3


static int cmd_help(int argc, char **argv);
static int cmd_open(int argc, char **argv);
static int cmd_close(int argc, char **argv);
static int cmd_timebase(int argc, char **argv);
static int cmd_reset(int argc, char **argv);
static int cmd_patch(int argc, char **argv);
static int cmd_quit(int argc, char **argv);
static int cmd_queue(int argc, char **argv);
static int cmd_maxqueue(int argc, char **argv);
static int cmd_time(int argc, char **argv);
static int cmd_nop(int argc, char **argv);
static int cmd_autoreduce(int argc, char **argv);
static int cmd_setbuf(int argc, char **argv);
static int cmd_protocol(int argc, char **argv);

struct
{
    char *cmd, *help;
    int minarg, maxarg;
    int (* proc)(int argc, char **argv);  /* argv[0] is command name
					   * argv[1..argc-1] is the arg.
					   * return: 0=success, -1=fatal-error,
					   *         1=connection-closed
					   */
} cmd_table[] =
{
    {"HELP",
	"HELP\tDisplay this message",
	1, 1, cmd_help},
    {"OPEN",
	"OPEN {lsb|msb}\n"
	"\tOpen data connection\n"
	"\tlsb: The byte order of data stream is LSB\n"
	"\tmsb: The byte order of data stream is MSB",
	2, 2, cmd_open},
    {"CLOSE",
	"CLOSE\tShutdown current data connection",
	1, 1, cmd_close},
    {"TIMEBASE",
	"TIMEBASE [timebase]\n\tSet time base",
	1, 2, cmd_timebase},
    {"RESET",
	"RESET\tInitialize all of MIDI status",
	1, 1, cmd_reset},
    {"PATCH",
	"PATCH {drumset|bank} <bank-no> <prog-no>\n\tLoad specified patch",
	4, 4, cmd_patch},
    {"QUIT",
	"QUIT\tClose control connection",
	1, 1, cmd_quit},
    {"QUEUE",
	"QUEUE\tTiMidity tells the time of audio buffer queue in second",
	1, 1, cmd_queue},
    {"MAXQUEUE",
	"MAXQUEUE\n"
	"\tTiMidity tells the maxmum time of audio buffer queue in second",
	1, 1, cmd_maxqueue},
    {"TIME",
	"TIME\tTiMidity tells the current playing time in second",
	1, 1, cmd_time},
    {"NOP",
	"NOP\tDo nothing",
	1, 1, cmd_nop},
    {"AUTOREDUCE",
	"AUTOREDUCE {on|off} [msec]\n\tEnable/Disable auto voice reduction",
	2, 3, cmd_autoreduce},
    {"SETBUF",
	"SETBUF low hi\n\tSpecify low/hi sec of buffer queue",
	3, 3, cmd_setbuf},
    {"PROTOCOL",
	"PROTOCOL [sequencer|midi]\n\tSpecify the protocol to use",
	1, 2, cmd_protocol},

    {NULL, NULL, 0, 0, NULL} /* terminate */
};

/*
TEMPO [<tempo>]\n\
	Change the tempo.  If the argument is omitted, TiMidity tells the\n\
	current tempo.\n\
KEYSHIFT <{+|-}offs>\n\
	Change the base key. (0 to reset)\n\
SPEED <{+|-}offs>\n\
	Change the play speed. (0 to reset)\n\
MODE {gs|xg|gm}\n\
	Specify default MIDI system mode\n\
SYNTH [gus|awe]\n\
	Specify Synth type emulation. (no argument to default)\n\
*/


static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int ctl_write(char *buffer, int32 size);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_event(CtlEvent *e);
static int ctl_pass_playing_list(int n, char *args[]);

/**********************************/
/* export the interface functions */

#define ctl server_control_mode

ControlMode ctl=
{
    "remote interface", 'r',
    "server",
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    ctl_pass_playing_list,
    ctl_read,
    ctl_write,
    cmsg,
    ctl_event
};


struct fd_read_buffer
{
    char buff[BUFSIZ];
    /* count: beginning of read pointer
     * size:  end of read pointer
     * fd:    file descripter for input
     */
    int count, size, fd;
};
static int fdgets(char *buff, size_t buff_size, struct fd_read_buffer *p);
static int fdputs(char *s, int fd);
static uint32 data2long(uint8* data);
static uint16 data2short(uint8* data);
static int do_control_command_nonblock(void);

static struct fd_read_buffer control_fd_buffer;
static int data_fd = -1, control_fd = -1;
static int data_port, control_port;
static int is_lsb_data = 1;
static int curr_timebase = DEFAULT_TIMEBASE;
static int32 sample_correction;
static int32 sample_increment;
static int32 sample_correction;
static int32 sample_cum;
static int32 curr_event_samples, event_time_offset;
static int32 curr_tick, tick_offs;
static double start_time;
static int tmr_running, notmr_running;
enum { PROTO_SEQ, PROTO_MIDI };
static int proto;
static int is_system_prefix, rstatus;
static struct sockaddr_storage control_client;
static double low_time_at = DEFAULT_LOW_TIMEAT;
static double high_time_at = DEFAULT_HIGH_TIMEAT;
static FILE *outfp = NULL;

#define CONTROL_FD_OUT (control_port ? control_fd : STDOUT_FILENO)

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
    ctl.opened = 1;
    ctl.flags &= ~(CTLF_LIST_RANDOM|CTLF_LIST_SORT);
    outfp = stderr;
    return 0;
}

static void ctl_close(void)
{
    if(!ctl.opened)
	return;
    if(data_fd != -1)
    {
	close(data_fd);
	data_fd = -1;
    }
    if(control_fd != -1)
    {
	close(control_fd);
	control_fd = -1;
    }
}

/*ARGSUSED*/
static int ctl_read(int32 *valp)
{
    if(data_fd != -1)
	do_control_command_nonblock();
    return RC_NONE;
}

static int ctl_write(char *buffer, int32 size)
{
    static int warned = 0;
    if (!warned) {
	fprintf(stderr, "Warning: STDOUT redirected to data socket\n");
	warned = 1;
    }
    if(data_fd != -1)
	return send(data_fd, buffer, size, MSG_DONTWAIT);
    return -1;
}

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
    va_list ap;

    if((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
       ctl.verbosity < verbosity_level)
	return 0;

    if(outfp == NULL)
	outfp = stderr;

    va_start(ap, fmt);
    vfprintf(outfp, fmt, ap);
    fputs(NLS, outfp);
    fflush(outfp);
    va_end(ap);

    return 0;
}

static void ctl_event(CtlEvent *e)
{
}

static int pasv_open(int *port)
{
    int sfd, s;
    struct addrinfo hints, *result, *rp;
    char service[NI_MAXSERV];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    snprintf(service, sizeof(service), "%d", *port);
    
    s = getaddrinfo(NULL, service, &hints, &result);
    if (s)
    {
        fprintf(stderr, "getaddrinfo %s", gai_strerror(s));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        if (rp->ai_family != AF_INET && rp->ai_family != AF_INET6)
             continue;

        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (sfd == -1)
            continue;

#ifdef SO_REUSEADDR
        {
            int on = 1;
            setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (caddr_t)&on, sizeof(on));
        }
#endif /* SO_REUSEADDR */

        ctl.cmsg(CMSG_INFO, VERB_DEBUG, "Bind TCP/IP port=%d", *port);
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) != 0) {
            perror("bind");
            close(sfd);
        } else
            break;

        close(sfd);
    }

    if (rp == NULL)
    {
	fprintf(stderr, "Could not bind\n");
	freeaddrinfo(result);
	return -1;
    }

    freeaddrinfo(result);

    if(*port == 0)
    {
	struct sockaddr_storage server;
	socklen_t len = sizeof(server);

	if(getsockname(sfd, (struct sockaddr *)&server, &len) < 0)
	{
	    perror("getsockname");
	    close(sfd);
	    return -1;
	}

        /* Not quite protocol independent */
        switch (((struct sockaddr *) &server)->sa_family)
        {
            case AF_INET:
                *port = ntohs(((struct sockaddr_in *) &server)->sin_port);
                break;
            case AF_INET6:
                *port = ntohs(((struct sockaddr_in6 *) &server)->sin6_port);
                break;
        }
    }

    /* Set it up to wait for connections. */
    if(listen(sfd, 1) < 0)
    {
	perror("listen");
	close(sfd);
	return -1;
    }

    return sfd;
}

static RETSIGTYPE sig_timeout(int sig)
{
    signal(SIGALRM, sig_timeout); /* For SysV base */
    /* Expect EINTR */
}

static void doit(void);
static int send_status(int status, char *message, ...);
static void compute_sample_increment(void);
static void server_reset(void);

static int ctl_pass_playing_list(int n, char *args[])
{
    int sock;

    if(n != 2 && n != 1)
    {
	fprintf(stderr, "Usage: timidity -ir control-port [data-port]\n");
	return 1;
    }

    control_port = atoi(args[0]);
    if(n == 2)
	data_port = atoi(args[1]);
    else
	data_port = 0;

    if (control_port) {
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);    /* Handle broken pipe */
#endif /* SIGPIPE */
	sock = pasv_open(&control_port);
	if(sock == -1)
	    return 1;
    }
    opt_realtime_playing = 1; /* Enable loading patch while playing */
    allocate_cache_size = 0; /* Don't use pre-calclated samples */
/*  aq_set_soft_queue(-1.0, 0.0); */
    alarm(0);
    signal(SIGALRM, sig_timeout);

    while(1)
    {
	socklen_t addrlen;

	addrlen = sizeof(control_client);
	memset(&control_client, 0, addrlen);

	if (control_port) {
	    if((control_fd = accept(sock,
				(struct sockaddr *)&control_client,
				&addrlen)) < 0)
	    {
		if(errno == EINTR)
		    continue;
		perror("accept");
		close(sock);
		return 1;
	    }
	}
	else control_fd = 0;

	if (play_mode->acntl(PM_REQ_PLAY_START, NULL) < 0)
	{
	    ctl.cmsg(CMSG_FATAL, VERB_NORMAL,
		     "Couldn't start %s (`%c')",
		     play_mode->id_name, play_mode->id_character);
	    send_status(510, "Couldn't start %s (`%c')",
			play_mode->id_name, play_mode->id_character);
	    if (control_port) {
		close(control_fd);
		control_fd = -1;
	    }
	    break;
	}

	server_reset();

	ctl.cmsg(CMSG_INFO, VERB_NOISY, "Connected");
	doit();
	ctl.cmsg(CMSG_INFO, VERB_NOISY, "Connection closed");

	play_mode->acntl(PM_REQ_PLAY_END, NULL);

	if(control_fd != -1 && control_port)
	{
	    close(control_fd);
	    control_fd = -1;
	}
	if(data_fd != -1)
	{
	    close(data_fd);
	    data_fd = -1;
	}
	free_instruments(0);
	free_global_mblock();
	if (!control_port)
	    break;
    }
    return 0;
}

#define MAX_GETCMD_PARAMS 8
/* return:
 * 0: success
 * 1: error
 *-1: fatal error (will be close the connection)
 */
static int control_getcmd(char **params, int *nparams)
{
    static char buff[BUFSIZ];
    int n;

    /* read line */
    n = fdgets(buff, sizeof(buff), &control_fd_buffer);
    if(n == -1)
    {
	perror("read");
	return -1;
    }
    if(n == 0)
	return 1;
    if((params[0] = strtok(buff, " \t\r\n\240")) == NULL)
	return 0;
    *nparams = 0;
    while(params[*nparams] && *nparams < MAX_GETCMD_PARAMS)
	params[++(*nparams)] = strtok(NULL," \t\r\n\240");
    return 0;
}

static int send_status(int status, char *message, ...)
{
    va_list ap;
    char buff[BUFSIZ];

    va_start(ap, message);
    sprintf(buff, "%03d ", status);
    vsnprintf(buff + 4, sizeof(buff) - 5, message, ap);
    va_end(ap);
    strncat(buff, "\n", BUFSIZ - strlen(buff) - 1);
    buff[BUFSIZ-1] = '\0';      /* force terminate */
    if(write(CONTROL_FD_OUT, buff, strlen(buff)) == -1)
	return -1;
    return 0;
}

static void seq_play_event(MidiEvent *ev)
{
    if(tmr_running)
	ev->time = curr_event_samples;
    else
    {
	if(IS_STREAM_TRACE)
	{
	    event_time_offset += play_mode->rate / MIDI_COMMAND_PER_SEC;
	    ev->time = curr_event_samples;
	}
	else
	{
	    double past_time = get_current_calender_time() - start_time -
		    (double)(curr_event_samples + event_time_offset) / play_mode->rate;
	    ev->time = (int32)(past_time * play_mode->rate);
	}
    }
    ev->time += event_time_offset;
    play_event(ev);
}

static void tmr_reset(void)
{
    curr_event_samples =
	event_time_offset =
	    sample_cum = 0;
    playmidi_tmr_reset();
    curr_timebase = DEFAULT_TIMEBASE;
    curr_tick = tick_offs = 0;
    start_time = get_current_calender_time();
}

static void compute_sample_increment(void)
{
    double a;
    a = (double)current_play_tempo * (double)play_mode->rate
	* (65536.0/500000.0) / (double)curr_timebase,
    sample_correction = (int32)(a) & 0xFFFF;
    sample_increment = (int32)(a) >> 16;
}

static void add_tick(int tick)
{
    int32 samples_to_do;
    MidiEvent ev;

    samples_to_do = sample_increment * tick;
    sample_cum += sample_correction * tick;
    if(sample_cum & 0xFFFF0000)
    {
	samples_to_do += ((sample_cum >> 16) & 0xFFFF);
	sample_cum &= 0x0000FFFF;
    }
    curr_event_samples += samples_to_do;
    curr_tick += tick;
    ev.type = ME_NONE;
    seq_play_event(&ev);
}

static int tick2sample(int tick)
{
    int32 samples, cum;

    samples = sample_increment * tick;
    cum = sample_correction * tick;
    if(cum & 0xFFFF0000)
	samples += ((sample_cum >> 16) & 0xFFFF);
    return samples;
}

int time2tick(double sec)
{
    return (int)(sec * curr_timebase);
}


static void stop_playing(void)
{
    if(upper_voices)
    {
	MidiEvent ev;
	ev.type = ME_EOT;
	ev.a = 0;
	ev.b = 0;
	seq_play_event(&ev);
	aq_flush(0);
    }

    notmr_running = 0;
}

static int do_control_command(void);
static int do_control_command_nonblock(void);
static int do_sequencer(void);
static void do_chn_voice(uint8 *);
static void do_chn_common(uint8 *);
static void do_timing(uint8 *);
static void do_sysex(uint8 *, int len);
static void do_extended(uint8 *);
static void do_timeout(void);
static void server_seq_sync(double tm);

static uint8 data_buffer[BUFSIZ];
static int data_buffer_len;

static void doit(void)
{
    memset(&control_fd_buffer, 0, sizeof(control_fd_buffer));
    control_fd_buffer.fd = control_fd;

    send_status(220, "TiMidity++ %s%s ready (Server Version %s)",
    		(strcmp(timidity_version, "current")) ? "v" : "",
    		timidity_version, SERVER_VERSION);

/*    while(data_fd != -1 && control_fd != -1) */
    while(control_fd != -1)
    {
	fd_set fds;
	int n, maxfd;

	if(data_fd == -1)
	{
	    if(do_control_command())
		break;
	}
	else
	{
	    long usec;

	    FD_ZERO(&fds);
	    FD_SET(control_fd, &fds);
	    FD_SET(data_fd, &fds);
	    if(control_fd > data_fd)
		maxfd = control_fd;
	    else
		maxfd = data_fd;

	    if(data_fd != -1 && (tmr_running || notmr_running))
	    {
		    double wait_time, filled;

		    if (IS_STREAM_TRACE)
			filled = aq_filled();
		    else
			filled = high_time_at * play_mode->rate;

		    wait_time = filled / play_mode->rate - low_time_at;
		    if(wait_time <= 0)
			usec = 0;
		    else
			usec = (long)(wait_time * 1000000);
	    }
	    else
		usec = -1;

	    if(usec >= 0)
	    {
		struct timeval timeout;
		timeout.tv_sec = usec / 1000000;
		timeout.tv_usec = usec % 1000000;
		n = select(maxfd + 1, &fds, NULL, NULL, &timeout);
	    }
	    else
		n = select(maxfd + 1, &fds, NULL, NULL, NULL);

	    if(n < 0)
	    {
		perror("select");
		break;
	    }

	    if(n == 0)
	    {
		do_timeout();
		continue;
	    }

	    if(control_fd != -1 && FD_ISSET(control_fd, &fds))
	    {
		if(do_control_command())
		{
		    close(control_fd);
		    control_fd = -1;
		}
	    }
	    else if(data_fd != -1 && FD_ISSET(data_fd, &fds))
	    {
		notmr_running = !tmr_running;
		if(do_sequencer())
		{
		    close(data_fd);
		    data_fd = -1;
		    send_status(403, "Data connection is closed");
		}
	    }
	}
    }

    if(data_fd != -1)
	stop_playing();
}

static void do_timeout(void)
{
    double fill_time;

    if(data_fd == -1)
	return;
    aq_add(NULL, 0);
    if (IS_STREAM_TRACE)
	fill_time = high_time_at - (double)aq_filled() / play_mode->rate;
    else
	fill_time = get_current_calender_time() - start_time -
		(double)(curr_event_samples + event_time_offset) / play_mode->rate;
    if(fill_time <= 0)
	return;

    if(tmr_running)
	add_tick(time2tick(fill_time));
    else
    {
	MidiEvent ev;
	event_time_offset += (int32)(fill_time *
				     play_mode->rate);
	ev.time = curr_event_samples + event_time_offset;
	ev.type = ME_NONE;
	play_event(&ev);
    }
}

/* -1=error, 0=success, 1=connection-closed */
static int data_flush(int discard)
{
    fd_set fds;
    char buff[BUFSIZ];
    struct timeval timeout;
    int n;

    while(1)
    {
	FD_ZERO(&fds);
	FD_SET(data_fd, &fds);
	timeout.tv_sec = 0;
	if(discard)
	    timeout.tv_usec = 100000;
	else
	    timeout.tv_usec = 0;
	if((n = select(data_fd + 1, &fds, NULL, NULL, &timeout)) < 0)
	{
	    perror("select");
	    return -1;
	}
	if(n == 0)
	    break;
	if(discard)
	{
	    if((n = read(data_fd, buff, sizeof(buff))) < 0)
	    {
		perror("read");
		return -1;
	    }
	    if(n == 0)
		return 1;
	}
	else
	{
	    int status;
	    if((status = do_sequencer()) != 0)
		return status;
	}
    }
    return 0;
}

static void server_seq_sync(double tm)
{
    double t;
    aq_soft_flush();
    t = (double)aq_filled() / play_mode->rate;
    if(t > tm)
	usleep((unsigned long)((t - tm) * 1000000));
}

static void server_reset(void)
{
    playmidi_stream_init();
    if(free_instruments_afterwards)
	free_instruments(0);
    data_buffer_len = 0;
    rstatus = 0;
    do_sysex(NULL, 0); /* Initialize SysEx buffer */
    low_time_at = DEFAULT_LOW_TIMEAT;
    high_time_at = DEFAULT_HIGH_TIMEAT;
    reduce_voice_threshold = 0; /* Disable auto reduction voice */
    compute_sample_increment();
    tmr_reset();
    tmr_running = notmr_running = 0;
    start_time = get_current_calender_time();
    proto = PROTO_SEQ;
}

/* -1=error, 0=success, 1=connection-closed */
static int do_control_command(void)
{
    int status;
    char *params[MAX_GETCMD_PARAMS];
    int nparams;
    int i;

    if((status = control_getcmd(params, &nparams)) == -1)
	return -1;
    if(status == 1)
    {
	send_status(500, "Error");
	return 1;
    }

    if(nparams == 0 ||  *params == NULL || **params == '\0')
	return 0;

    for(i = 0; cmd_table[i].cmd; i++)
	if(strcasecmp(params[0], cmd_table[i].cmd) == 0)
	{
	    if(nparams < cmd_table[i].minarg)
		return send_status(501, "'%s': Arguments is too few",
				   params[0]);
	    if(nparams > cmd_table[i].maxarg)
		return send_status(501, "'%s': Arguments is too many",
				   params[0]);
	    return cmd_table[i].proc(nparams, params);
	}
    return send_status(500, "'%s': command not understood.", params[0]);
}

static int cmd_help(int argc, char **argv)
{
    int i;

    if(send_status(200, "Help ok"))
	return -1;

    for(i = 0; cmd_table[i].cmd; i++)
    {
	if(fdputs(cmd_table[i].help, CONTROL_FD_OUT))
	    return -1;
	if(fdputs("\n", CONTROL_FD_OUT))
	    return -1;
    }
    return fdputs(".\n", CONTROL_FD_OUT);
}

static int cmd_open(int argc, char **argv)
{
    int sock;
    struct sockaddr_storage in;
    socklen_t addrlen;
    int port;

    if(data_fd != -1)
	return send_status(125, "Data connection is already opened");

    if(strcasecmp(argv[1], "lsb") == 0)
	is_lsb_data = 1;
    else if(strcasecmp(argv[1], "msb") == 0)
	is_lsb_data = 0;
    else
	return send_status(502, "OPEN: Invalid argument: %s", argv[1]);

    port = data_port;
    if((sock = pasv_open(&port)) == -1)
	return send_status(511, "Can't open data connection");

    addrlen = sizeof(in);
    memset(&in, 0, addrlen);
    send_status(200, "%d is ready acceptable", port);

    alarm(SIG_TIMEOUT_SEC);
    data_fd = accept(sock, (struct sockaddr *)&in, &addrlen);
    alarm(0);

    if(data_fd < 0)
    {
	send_status(512, "Accept error");
	close(sock);
	return 0;
    }
    close(sock);

     /* Not quite protocol independent */
     if(control_port) switch (((struct sockaddr *) &control_client)->sa_family)
     {
         case AF_INET:
            if (((struct sockaddr_in *) &control_client)->sin_addr.s_addr !=
                ((struct sockaddr_in *) &in)->sin_addr.s_addr)
            {
              close(data_fd);
              data_fd = -1;
              return send_status(513, "Security violation: Address mismatch" );
            }
            break;
         case AF_INET6:
            if (memcmp(
                ((struct sockaddr_in6 *) &control_client)->sin6_addr.s6_addr,
                ((struct sockaddr_in6 *) &in)->sin6_addr.s6_addr, 16))
            {
              close(data_fd);
              data_fd = -1;
              return send_status(513, "Security violation: Address mismatch" );
            }
            break;
     }


    data_buffer_len = 0;
    do_sysex(NULL, 0); /* Initialize SysEx buffer */
    tmr_reset();
    send_status(200, "Ready data connection");

    return 0;
}

static int cmd_close(int argc, char **argv)
{
    if(data_fd != -1)
    {
	close(data_fd);
	data_fd = -1;
	return send_status(302, "Data connection is closed");
    }
    return send_status(302, "Data connection is already closed");
}

static int cmd_queue(int argc, char **argv)
{
    int32 qsamples;

    aq_add(NULL, 0); /* Update software queue */
    if(!aq_fill_buffer_flag)
	qsamples = aq_soft_filled() + aq_filled();
    else
	qsamples = 0;
    return send_status(200, "%f sec", (double)qsamples / play_mode->rate);
}

static int cmd_maxqueue(int argc, char **argv)
{
    return send_status(200, "%f sec",
		       (double)aq_get_dev_queuesize() / play_mode->rate);
}

static int cmd_time(int argc, char **argv)
{
    return send_status(200, "%f sec", (double)aq_samples());
}

static int cmd_quit(int argc, char **argv)
{
    send_status(200, "Bye");
    return 1;
}

static int cmd_timebase(int argc, char **argv)
{
    int i;

    if(argc == 1)
	return send_status(200, "%d OK", curr_timebase);
    i = atoi(argv[1]);
    if(i < 1)
	i = 1;
    else if(i > 1000)
	i = 1000;
    if(i != curr_timebase)
    {
	curr_timebase = i;
	compute_sample_increment();
	tick_offs = curr_tick;
	start_time = get_current_calender_time();
    }
    return send_status(200, "OK");
}

static int cmd_patch(int argc, char **argv)
{
    int dr, bank, prog;

    if(strcasecmp(argv[1], "drumset") == 0)
	dr = 1;
    else if(strcasecmp(argv[1], "bank") == 0)
	dr = 0;
    else
	return send_status(502, "PATCH: Invalid argument: %s", argv[1]); 

    bank = atoi(argv[2]);
    prog = atoi(argv[3]);
    if(bank < 0 || bank > 127 || prog < 0 || prog > 127)
	return send_status(502, "PATCH: Invalid argument"); 
    if(play_midi_load_instrument(dr, bank, prog) == NULL)
	return send_status(514, "PATCH: Can't load the patch");
    return send_status(200, "OK");
}

static int cmd_reset(int argc, char **argv)
{
    int status;

    if(data_fd >= 0)
    {
	stop_playing();
	if((status = data_flush(1)) != 0)
	    return status;
    }
    server_reset();
    return send_status(200, "OK");
}

static int cmd_autoreduce(int argc, char **argv)
{
    if(strcasecmp(argv[1], "on") == 0)
    {
	if(argc == 3)
	    reduce_voice_threshold = atoi(argv[2]);
	else
	    reduce_voice_threshold = -1;
    }
    else if(strcasecmp(argv[1], "off") == 0)
	reduce_voice_threshold = 0;
    else
	return send_status(502, "AUTOREDUCE: Invalid argument: %s",
			   argv[1]);
    return send_status(200, "OK");
}

static int cmd_setbuf(int argc, char **argv)
{
    low_time_at = atof(argv[1]);
    high_time_at = atof(argv[2]);
    return send_status(200, "OK");
}

static int cmd_protocol(int argc, char **argv)
{
    if (argc < 2)
	return send_status(200, "Current protocol is %s",
		proto == PROTO_SEQ ? "sequencer" : "midi");
    if (strcasecmp(argv[1], "sequencer") == 0)
	proto = PROTO_SEQ;
    else if (strcasecmp(argv[1], "midi") == 0)
	proto = PROTO_MIDI;
    else return send_status(500, "Invalid protocol name %s", argv[1]);

    return send_status(200, "Protocol set to %s",
	    proto == PROTO_SEQ ? "sequencer" : "midi");
}

static int cmd_nop(int argc, char **argv)
{
    return send_status(200, "NOP OK");
}

static int do_control_command_nonblock(void)
{
    struct timeval timeout;
    int n;
    fd_set fds;

    if(control_fd == -1)
	return 1;
    FD_ZERO(&fds);
    FD_SET(control_fd, &fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    n = select(control_fd + 1, &fds, NULL, NULL, &timeout);
    if(n > 0 && FD_ISSET(control_fd, &fds))
	return do_control_command();
    return 0;
}

static int fdgets(char *buff, size_t buff_size, struct fd_read_buffer *p)
{
    int n, len, count, size, fd;
    char *buff_endp = buff + buff_size - 1, *pbuff, *beg;

    fd = p->fd;
    if(buff_size == 0)
	return 0;
    else if(buff_size == 1) /* buff == buff_endp */
    {
	*buff = '\0';
	return 0;
    }

    len = 0;
    count = p->count;
    size = p->size;
    pbuff = p->buff;
    beg = buff;
    do
    {
	if(count == size)
	{
	    if((n = read(fd, pbuff, BUFSIZ)) <= 0)
	    {
		*buff = '\0';
		if(n == 0)
		{
		    p->count = p->size = 0;
		    return buff - beg;
		}
		return -1; /* < 0 error */
	    }
	    count = p->count = 0;
	    size = p->size = n;
	}
	*buff++ = pbuff[count++];
    } while(*(buff - 1) != '\n' && buff != buff_endp);
    *buff = '\0';
    p->count = count;
    return buff - beg;
}

static int fdputs(char *s, int fd)
{
    if(write(fd, s, strlen(s)) == -1)
	return -1;
    return 0;
}

#ifdef LITTLE_ENDIAN
static uint32 data2long(uint8* data)
{
    uint32 x;
    memcpy(&x, data, sizeof(x));
    if(!is_lsb_data)
	x = XCHG_LONG(x);
    return x;
}
static uint16 data2short(uint8* data)
{
    uint16 x;
    memcpy(&x, data, sizeof(x));
    if(!is_lsb_data)
	x = XCHG_SHORT(x);
    return x;
}
#else
static uint32 data2long(uint8* data)
{
    uint32 x;
    memcpy(&x, data, sizeof(x));
    if(is_lsb_data)
	x = XCHG_LONG(x);
    return x;
}
static uint16 data2short(uint8* data)
{
    uint16 x;
    memcpy(&x, data, sizeof(x));
    if(is_lsb_data)
	x = XCHG_SHORT(x);
    return x;
}
#endif

static int do_sequencer(void)
{
    int n, offset, frame_size, data_offs, data_frame_num;
    unsigned char status;
    MidiEvent ev;

    n = read(data_fd, data_buffer + data_buffer_len,
	     sizeof(data_buffer) - data_buffer_len);

    if(n <= 0)
    {
	stop_playing();
	return n;
    }

#ifdef DEBUG_DUMP_SEQ
    {
	int i;
	for(i = 0; i < n; i++)
	    fprintf(stderr, "%02x", data_buffer[data_buffer_len + i]);
	fprintf(stderr, "\n");
    }
#endif /* DEBUG_DUMP_SEQ */

    data_buffer_len += n;
    offset = 0;
    frame_size = (proto == PROTO_SEQ ? 4 : 1);
    while(offset < data_buffer_len)
    {
	int cmd;
	cmd = (proto == PROTO_SEQ ? data_buffer[offset] : SEQ_MIDIPUTC);

#define READ_NEEDBUF(x) if(offset + (x) > data_buffer_len) goto done;
#define READ_ADVBUF(x)  offset += (x);
	switch(cmd)
	{
	  case EV_CHN_VOICE:
	    READ_NEEDBUF(8);
	    do_chn_voice(data_buffer + offset);
	    READ_ADVBUF(8);
	    break;
	  case EV_CHN_COMMON:
	    READ_NEEDBUF(8);
	    do_chn_common(data_buffer + offset);
	    READ_ADVBUF(8);
	    break;
	  case EV_TIMING:
	    READ_NEEDBUF(8);
	    do_timing(data_buffer + offset);
	    READ_ADVBUF(8);
	    break;
	  case EV_SYSEX:
	    READ_NEEDBUF(8);
	    do_sysex(data_buffer + offset + 2, 6);
	    READ_ADVBUF(8);
	    break;
	  case SEQ_MIDIPUTC:
	    READ_NEEDBUF(proto == PROTO_SEQ ? 2 : 1);
	    data_offs = (proto == PROTO_SEQ ? 1 : 0);
	    data_frame_num = 0;

	    if(is_system_prefix)
	    {
		READ_NEEDBUF(frame_size);
		do_sysex(data_buffer + offset + data_offs, 1);
		if(data_buffer[offset + data_offs] == 0xf7)
		    is_system_prefix = 0;  /* End SysEX */
		READ_ADVBUF(frame_size);
		break;
	    }

	    status = data_buffer[offset + data_offs];
	    if (status & 0x80) {
		/* data bytes follows status byte */
		data_frame_num++;
	    } else {
		/* use "running status" */
		status = rstatus;
		if (!(status & 0x80)) {
		    /* no status byte, skip this crap */
		    READ_NEEDBUF(frame_size);
	    	    ctl.cmsg(CMSG_INFO, VERB_NORMAL, "no status byte for %#x, skipping",
			    data_buffer[offset + data_offs]);
		    READ_ADVBUF(frame_size);
		    break;
		}
	    }
#define DATA_BYTE(num) (data_buffer[offset + data_offs + \
    ((num) + data_frame_num) * frame_size])
	    switch(status & 0xf0)
	    {
	      case MIDI_NOTEON:
		READ_NEEDBUF((data_frame_num + 2) * frame_size);
		ev.channel = status & 0x0f;
		ev.a       = DATA_BYTE(0) & 0x7f;
		ev.b       = DATA_BYTE(1) & 0x7f;
		if(ev.b != 0)
		    ev.type = ME_NOTEON;
		else
		    ev.type = ME_NOTEOFF;
		seq_play_event(&ev);
		READ_ADVBUF((data_frame_num + 2) * frame_size);
		break;

	      case MIDI_NOTEOFF:
		READ_NEEDBUF((data_frame_num + 2) * frame_size);
		ev.type    = ME_NOTEOFF;
		ev.channel = status & 0x0f;
		ev.a       = DATA_BYTE(0) & 0x7f;
		ev.b       = DATA_BYTE(1) & 0x7f;
		seq_play_event(&ev);
		READ_ADVBUF((data_frame_num + 2) * frame_size);
		break;

	      case MIDI_KEY_PRESSURE:
		READ_NEEDBUF((data_frame_num + 2) * frame_size);
		ev.type    = ME_KEYPRESSURE;
		ev.channel = status & 0x0f;
		ev.a       = DATA_BYTE(0) & 0x7f;
		ev.b       = DATA_BYTE(1) & 0x7f;
		seq_play_event(&ev);
		READ_ADVBUF((data_frame_num + 2) * frame_size);
		break;

	      case MIDI_CTL_CHANGE:
		READ_NEEDBUF((data_frame_num + 2) * frame_size);
		if(convert_midi_control_change(status & 0x0f,
					       DATA_BYTE(0),
					       DATA_BYTE(1),
					       &ev))
		    seq_play_event(&ev);
		READ_ADVBUF((data_frame_num + 2) * frame_size);
		break;

	      case MIDI_PGM_CHANGE:
		READ_NEEDBUF((data_frame_num + 1) * frame_size);
		ev.type    = ME_PROGRAM;
		ev.channel = status & 0x0f;
		ev.a       = DATA_BYTE(0) & 0x7f;
		ev.b       = 0;
		seq_play_event(&ev);
		READ_ADVBUF((data_frame_num + 1) * frame_size);
		break;

	      case MIDI_CHN_PRESSURE:
		READ_NEEDBUF((data_frame_num + 1) * frame_size);
		ev.type    = ME_CHANNEL_PRESSURE;
		ev.channel = status & 0x0f;
		ev.a       = DATA_BYTE(0) & 0x7f;
		ev.b       = 0;
		seq_play_event(&ev);
		READ_ADVBUF((data_frame_num + 1) * frame_size);
		break;

	      case MIDI_PITCH_BEND:
		READ_NEEDBUF((data_frame_num + 2) * frame_size);
		ev.type    = ME_PITCHWHEEL;
		ev.channel = status & 0x0f;
		ev.a       = DATA_BYTE(0) & 0x7f;
		ev.b       = DATA_BYTE(1) & 0x7f;
		seq_play_event(&ev);
		READ_ADVBUF((data_frame_num + 2) * frame_size);
		break;

	      case MIDI_SYSTEM_PREFIX:
	        switch (status & 0x0f) {
	          /* Common System Messages */
	          case 0x00:		/* SysEx */
		    READ_NEEDBUF(frame_size);
		    do_sysex(data_buffer + offset + data_offs, 1);
		    is_system_prefix = 1; /* Start SysEX */
		    READ_ADVBUF(frame_size);
		    break;

		  case 0x01:		/* Quarter Frame */
		    READ_NEEDBUF(frame_size * 2);
		    READ_ADVBUF(frame_size * 2);
		    break;

		  case 0x02:		/* Song Position */
		    READ_NEEDBUF(frame_size * 3);
		    READ_ADVBUF(frame_size * 3);
		    break;

		  case 0x03:		/* Song Select */
		    READ_NEEDBUF(frame_size * 2);
		    READ_ADVBUF(frame_size * 2);
		    break;

		  case 0x04:
		  case 0x05:		/* Undef */
		    ctl.cmsg(CMSG_ERROR, VERB_NORMAL,
			"Undefined Common system message %x\n", status & 0x0f);
		    READ_NEEDBUF(frame_size);
		    READ_ADVBUF(frame_size);
		    break;

		  case 0x06:		/* Tune Request */
		    READ_NEEDBUF(frame_size);
		    READ_ADVBUF(frame_size);
		    break;

		  case 0x07:		/* EOX */
		    ctl.cmsg(CMSG_ERROR, VERB_NORMAL, "Unexpected EOX\n");
		    READ_NEEDBUF(frame_size);
		    READ_ADVBUF(frame_size);
		    break;

		  /* System Real Time Messages */
		  case 0x08:		/* MIDI Clock */
		    READ_NEEDBUF(frame_size);
		    READ_ADVBUF(frame_size);
		    break;

		  case 0x09:		/* MIDI Tick */
		    READ_NEEDBUF(frame_size);
		    READ_ADVBUF(frame_size);
		    break;

		  case 0x0A:		/* MIDI Start */
		    READ_NEEDBUF(frame_size);
		    READ_ADVBUF(frame_size);
		    break;

		  case 0x0B:		/* MIDI Continue */
		    READ_NEEDBUF(frame_size);
		    READ_ADVBUF(frame_size);
		    break;

		  case 0x0C:		/* MIDI Stop */
		    READ_NEEDBUF(frame_size);
		    stop_playing();
		    READ_ADVBUF(frame_size);
		    break;

		  case 0x0D:		/* Undef */
		    ctl.cmsg(CMSG_ERROR, VERB_NORMAL,
			"Undefined Real-Time system message %x\n", status & 0x0f);
		    READ_NEEDBUF(frame_size);
		    READ_ADVBUF(frame_size);
		    break;

		  case 0x0E:		/* Active Sensing */
		    READ_NEEDBUF(frame_size);
		    READ_ADVBUF(frame_size);
		    break;

		  case 0x0F:		/* Reset */
		    ctl.cmsg(CMSG_ERROR, VERB_NORMAL, "MIDI Reset\n");
		    READ_NEEDBUF(frame_size);
		    tmr_reset();
		    READ_ADVBUF(frame_size);
		    break;
		}
		status = 0;
		break;

	      default:
		ctl.cmsg(CMSG_ERROR, VERB_NORMAL,
			 "Undefined SEQ_MIDIPUTC 0x%02x",
			 data_buffer[offset + data_offs]);
		send_status(402, "Undefined SEQ_MIDIPUTC 0x%02x",
			    data_buffer[offset + data_offs]);
		return 1;
	    }
    	    rstatus = status;
	    break;

	  case SEQ_FULLSIZE:
	    /* WARNING: This data may be devided into some socket fragments. */
	    offset = data_buffer_len;
	    ctl.cmsg(CMSG_WARNING, VERB_NORMAL,
		     "SEQ_FULLSIZE is received.  This command is not safety.");
	    break;

	  case SEQ_EXTENDED:
	    READ_NEEDBUF(8);
	    do_extended(data_buffer + offset);
	    READ_ADVBUF(8);
	    break;

	  case EV_SEQ_LOCAL:
	  case SEQ_PRIVATE:
	    READ_NEEDBUF(8);
	    /* Ignore */
	    READ_ADVBUF(8);
	    break;

	  default:
	    ctl.cmsg(CMSG_ERROR, VERB_NORMAL,
		     "Undefined data 0x%02x", data_buffer[offset - 1]);
	    send_status(401, "Wrong data is recieved (seqcmd=0x%02x)",
			cmd);
	    stop_playing();
	    return 1;
	}
#undef READ_NEEDBUF
#undef READ_ADVBUF
#undef DATA_BYTE
    }

  done:
    if(offset)
    {
	data_buffer_len -= offset;
	memmove(data_buffer, data_buffer + offset, data_buffer_len);
    }
    return 0;
}


static void do_chn_voice(uint8 *data)
{
    int type, chn, note, parm;
    MidiEvent ev;

    type = data[2];
    chn  = data[3] % MAX_CHANNELS;
    note = data[4] & 0x7f;
    parm = data[5] & 0x7f;

    ev.channel = chn;
    ev.a       = note;
    ev.b       = parm;
    switch(type)
    {
      case MIDI_NOTEON:
	if(parm > 0)
	{
	    ev.type = ME_NOTEON;
	    seq_play_event(&ev);
	    break;
	}
	/* FALLTHROUGH */
      case MIDI_NOTEOFF:
	ev.type = ME_NOTEOFF;
	seq_play_event(&ev);
	break;
      case MIDI_KEY_PRESSURE:
	ev.type = ME_KEYPRESSURE;
	seq_play_event(&ev);
	break;
    }
}

static void do_chn_common(uint8 *data)
{
    int type, chn, p1, p2, w14;
    MidiEvent ev;

    type = data[2];
    chn  = data[3] % MAX_CHANNELS;
    p1 = data[4] & 0x7f;
    p2 = data[5] & 0x7f;
    w14 = data2short(data + 6) & 0x3fff;

    if(type == 0xff) /* Meta event */
    {
	/* This event is special event for timidity.  (not OSS compatible) */
	switch(data[3])
	{
	  case 0x2f: /* End of midi */
	    stop_playing();
	    tmr_reset();
	    break;

	  case 0x7f: /* Sequencer-Specific Meta-Event */
	    switch(data[4])
	    {
	      case 0x01: /* MIDI Reset */
		ev.type = ME_RESET;
		ev.a = DEFAULT_SYSTEM_MODE;
		ev.b = 0;
		seq_play_event(&ev);
		break;

	      case 0x02:  { /* Used to sync. */
		    double target_time, queue_time, sleep_time;
		    if(w14 == 0)
		    {
			aq_flush(0); /* wait until playout */
			send_status(301, "0 Sync OK");
			break;
		    }

		    aq_soft_flush();
		    target_time = (double)w14 / curr_timebase;
		    queue_time = (double)aq_filled() / play_mode->rate;
		    sleep_time = queue_time - target_time;
		    if(sleep_time > 0)
		    {
			send_status(301, "%g Sync OK", sleep_time);
			usleep((unsigned long)(sleep_time * 1000000));
		    }
		    else
			send_status(301, "0 Sync OK");
		}
		break;
	    }
	}
	return;
    }

    ev.channel = chn;
    switch(type)
    {
      case MIDI_CTL_CHANGE:
	if(convert_midi_control_change(chn, p1, w14, &ev))
	    seq_play_event(&ev);
	break;
      case MIDI_PGM_CHANGE:
	ev.type = ME_PROGRAM;
	ev.a    = p1;
	ev.b    = 0;
	seq_play_event(&ev);
	break;
      case MIDI_CHN_PRESSURE:
	ev.type = ME_CHANNEL_PRESSURE;
	ev.a    = p1;
	ev.b    = p2;
	seq_play_event(&ev);
	break;
      case MIDI_PITCH_BEND:
	ev.type = ME_PITCHWHEEL;
	ev.a    = w14 & 0x7f;
	ev.b    = (w14>>7) & 0x7f;
	seq_play_event(&ev);
	break;
    }
}


static void do_timing(uint8 *data)
{
    int32 val;

    val = data2long(data + 4);
    switch(data[1])
    {
      case TMR_START:
	tmr_running = 1;
	tmr_reset();
	event_time_offset = (int32)(play_mode->rate * high_time_at);
	break;

      case TMR_STOP:
	tmr_running = 0;
	break;

      case TMR_CONTINUE:
	if(!tmr_running)
	{
	    tmr_running = 1;
	    tick_offs = curr_tick;
	    start_time = get_current_calender_time();
	}
	break;

      case TMR_TEMPO:
#if 0 /* What should TMR_TEMPO work ? */
	if(val < 8)
	    val = 8;
	else if(val > 250)
	    val = 250;
	current_play_tempo = 60 * 1000000 / val;
	compute_sample_increment();
#endif
	break;

      case TMR_WAIT_ABS:

/*
   fprintf(stderr, "## TMR_WAIT_ABS: %d %d %d %g\n",
       curr_tick,
       curr_tick - (time2tick(get_current_calender_time()
			      - start_time) + tick_offs),
       event_time_offset,
       (double)aq_filled() / play_mode->rate);
 */

	val -= curr_tick;
	/*FALLTHROUGH*/
      case TMR_WAIT_REL:
	if(val <= 0)
	    break;
	add_tick(val);
	break;

#if 0
      case TMR_ECHO:
      case TMR_SPP:
#endif
      default:
/* fprintf(stderr, "## TMR=0x%02x is not supported\n", data[1]); */
	break;
    }
}


static void do_sysex(uint8 *data, int n)
{
    static uint8 sysexbuf[BUFSIZ];
    static int buflen;
    static int fillflag;
    int i;

    if(data == NULL)
    {
	buflen = fillflag = 0;
	is_system_prefix = 0;
	return;
    }

    for(i = 0; i < n; i++)
    {
	/* SysEx := /\xf0([^\xf7]*\xf7)/ */
	if(!fillflag)
	{
	    if(data[i] == 0xf0)
	    {
		fillflag = 1;
		continue;
	    }
	}
	if(fillflag)
	{
	    if(buflen < sizeof(sysexbuf))
		sysexbuf[buflen++] = data[i];
	    if(data[i] == 0xf7)
	    {
		MidiEvent ev;
		if(parse_sysex_event(sysexbuf, buflen, &ev))
		    seq_play_event(&ev);
		buflen = 0;
		fillflag = 0;
	    }
	}
    }
}

static void do_extended(uint8 *data)
{
    int value;
    MidiEvent ev;

    value = data[5] | data[6] * 256;
    switch(data[1])
    {
      case SEQ_CONTROLLER:
	switch(data[4])
	{
	  case CTRL_PITCH_BENDER: /* ?? */
	    break;
	  case CTRL_PITCH_BENDER_RANGE: /* ?? */
	    ev.channel = data[3] % MAX_CHANNELS;
	    ev.b = 0;
	    ev.a = 0;

	    /* LSB */
	    ev.type = ME_NRPN_LSB;
	    seq_play_event(&ev);

	    /* MSB */
	    ev.type = ME_NRPN_MSB;
	    seq_play_event(&ev);

	    /* Data entry */
	    ev.type = ME_DATA_ENTRY_MSB;
	    ev.a = value / 100; /* ?? */
	    seq_play_event(&ev);

	    break;
	  default:
	    break;
	}
	break;
      default:
	break;
    }
}


/*
 * interface_<id>_loader();
 */
ControlMode *interface_r_loader(void)
{
    return &ctl;
}
