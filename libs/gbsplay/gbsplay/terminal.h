/*
 * gbsplay is a Gameboy sound player
 *
 * 2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _TERMINAL_H_
#define _TERMINAL_H_

void setup_terminal(void);
void restore_terminal(void);
long get_input(char *c);

extern long redraw;

#endif /* _TERMINAL_H_ */
