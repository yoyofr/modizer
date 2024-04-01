/*
 * gbsplay is a Gameboy sound player
 *
 * 2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include <fcntl.h>

#include "common.h"
#include "terminal.h"

static long terminit;
static struct termios ots;

void exit_handler(int signum)
{
	printf(_("\nCaught signal %d, exiting...\n"), signum);
	restore_terminal();
	exit(1);
}

void stop_handler(int signum)
{
	restore_terminal();
}

void cont_handler(int signum)
{
	setup_terminal();
	redraw = true;
}

static void setup_handlers(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = exit_handler;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sa.sa_handler = stop_handler;
	sigaction(SIGSTOP, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sa.sa_handler = cont_handler;
	sigaction(SIGCONT, &sa, NULL);
}

void setup_terminal(void)
{
	struct termios ts;

	setup_handlers();

	if (tcgetattr(STDIN_FILENO, &ts) == -1)
		return;

	ots = ts;
	ts.c_lflag &= ~(ICANON | ECHO | ECHONL);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &ts);
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	terminit = 1;
}

void restore_terminal(void)
{
	if (terminit)
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &ots);
}

long get_input(char *c)
{
	return read(STDIN_FILENO, c, 1) == 1;
}
