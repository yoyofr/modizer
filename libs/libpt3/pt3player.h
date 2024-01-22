/* PT3PLAYER.H
 *
 * player functions for PT3
 *
 */

#ifndef PT3PLAYER_H

#define PT3PLAYER_H

//MAIN FUNCTION - instantly stops music
void func_mute();
//MAIN FUNCTION - runs 1 tick of music
void func_play_tick(int ch);
//MAIN FUNCTION - setup music for playing
int func_setup_music(uint8_t* music_ptr, int length, int ch, int first);
int func_restart_music(int ch);


void func_getregs(uint8_t *dest, int ch);

extern int forced_notetable;

#endif