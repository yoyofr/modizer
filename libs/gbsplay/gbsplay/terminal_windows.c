/*
 * gbsplay is a Gameboy sound player
 *
 * 2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <unistd.h>
#include <stdlib.h>
#include <windows.h>

#include "common.h"
#include "terminal.h"

HANDLE hStdin = INVALID_HANDLE_VALUE;
HANDLE hStdout = INVALID_HANDLE_VALUE;
DWORD oldInMode;
DWORD oldOutMode;

BOOL WINAPI ctrl_handler(DWORD dwCtrlType)
{
	printf(_("\nCaught signal %d, exiting...\n"), (int)dwCtrlType);
	restore_terminal();
	exit(1);
}

void setup_terminal(void)
{
	DWORD newInMode = ENABLE_WINDOW_INPUT;
	DWORD newOutMode;

	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Failed to get console handles\n");
		return;
	}
	if (!GetConsoleMode(hStdin, &oldInMode)) {
		fprintf(stderr, "Failed to get input mode\n");
		return;
	}
	if (!GetConsoleMode(hStdout, &oldOutMode)) {
		fprintf(stderr, "Failed to get output mode\n");
		return;
	}
	if (!SetConsoleCtrlHandler(ctrl_handler, true)) {
		fprintf(stderr, "Failed to set ctrl handler\n");
	}
	if (!SetConsoleMode(hStdin, newInMode)) {
		fprintf(stderr, "Failed to set new input mode\n");
	}
	newOutMode = oldOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
	if (!SetConsoleMode(hStdout, newOutMode)) {
		newOutMode = oldOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		if (!SetConsoleMode(hStdout, newOutMode)) {
			fprintf(stderr, "Failed to enable virtual terminal mode\n");
		}
	}
}

void restore_terminal(void)
{
	if (hStdin != INVALID_HANDLE_VALUE) {
		SetConsoleMode(hStdin, oldInMode);
	}
	if (hStdout != INVALID_HANDLE_VALUE) {
		SetConsoleMode(hStdout, oldOutMode);
	}
}

long get_input(char *c) {
	DWORD numEvents = 0;
	INPUT_RECORD inBuf;
	KEY_EVENT_RECORD *ker;
	if (!GetNumberOfConsoleInputEvents(hStdin, &numEvents) || numEvents == 0) {
		return 0;
	}
	if (!ReadConsoleInput(hStdin, &inBuf, 1, &numEvents)) {
		return 0;
	}
	if (numEvents == 0 || inBuf.EventType != KEY_EVENT) {
		return 0;
	}
	ker = &inBuf.Event.KeyEvent;
	if (!ker->bKeyDown) {
		/* Key up! */
		*c = ker->uChar.AsciiChar;
		return 1;
	}
	return 0;
}
