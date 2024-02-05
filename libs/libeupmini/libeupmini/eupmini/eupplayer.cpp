// $Id: eupplayer.cc,v 1.9 2000/04/12 23:17:18 hayasaka Exp $

// note: EUP player is basically using a subset of MIDI and even the used command codes
// mirror those used in MIDI

#include <assert.h>
#include <iostream>
#include <cstring>
#include <math.h>
#include <sys/stat.h>

#include "eupplayer.hpp"
#include "sintbl.hpp"

/* 0 is 0
   1 is like heplay
   2 is genuine*/
#define OVERSTEP 2

#ifdef DEBUG
#include <stdarg.h>
static void dbprintf(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
}
#define DB(a) //dbprintf##a
#else
#define DB(a)
#endif


#define DB_PROCESSING(mes) DB(("* com=%02x tr=%02x sl=%02x sh=%02x p4=%02x p5=%02x (%s)\n", \
  (pl->_curP)[0], (pl->_curP)[1], (pl->_curP)[2], \
  (pl->_curP)[3], (pl->_curP)[4], (pl->_curP)[5], mes))

#if OVERSTEP == 2
#define COMPRESSOVERSTEPS if (stepTime >= 384) stepTime = 383
/* .... >= 384 means steptime >= measure length.
    May be present in .eup files created with HEAT.  */
#else
#define COMPRESSOVERSTEPS
#endif

#define WAIT4NEXTSTEP \
  int stepTime = (pl->_curP)[2] + 0x80*(pl->_curP)[3]; \
  COMPRESSOVERSTEPS; \
  if (stepTime > pl->_stepTime) return 1;


int EUPPlayer_cmd_8x(int cmd, EUPPlayer *pl)
{
    /* note off */
    DB_PROCESSING("Note off");

    /* It cannot come by itself, so it is an error. However, right after DATA CONTINUE
        Is it possible then? */

    return EUPPlayer_cmd_INVALID(cmd, pl);
}

static u_char _warn_once[8];

static u_char _in_progress_Sysex;
static u_char _track_Sysex;

void warn_once(int idx, const char* msg) {
	u_char *flag = &_warn_once[idx];
	if (*flag) {
		*flag = 0;
		fprintf(stderr, "error: song uses unsupported command: %s\n", msg);
	}
}
int EUPPlayer_cmd_9x(int cmd, EUPPlayer *pl)
{
    /* note on*/
    WAIT4NEXTSTEP;						// note-on (below handling) is triggered as soon as the step time is reached..
    DB_PROCESSING("Note on");

    if (((pl->_curP)[6] & 0xf0) != 0x80) {
        /* I have to put out a message that the following command is invalid  */
        return EUPPlayer_cmd_INVALID(cmd, pl);
    }

    if ((cmd & 0x0f) != 0) {
        DB(("MIDI-ch is not zero (%02x).\n", cmd));    // pb_theme etc.
    }

    int track = (pl->_curP)[1];
    int note = (pl->_curP)[4];
    int onVelo = (pl->_curP)[5];

	// undocumented; the "note off" command always seems to directly follow the
	// "note on" and this is what is handled below:

	// the gateTime seems to be the duration that the note it played before
	// "release" is triggered

    int gateTime = (0x1*(pl->_curP[7]) + 0x10*(pl->_curP[8]) +
                    0x100*(pl->_curP[9]) + 0x1000*(pl->_curP[10]));


    int offVelo = (pl->_curP)[11];
    //DB(("tr %02x\n", track));
    //DB(("step=%d, track=%d, note=%d, on=%d, gate=%d, off=%d\n", stepTime, track, note, onVelo, gateTime, offVelo));
    if (offVelo == 0 || offVelo >= 0x80) {
        offVelo = onVelo;    // Is this okay? (gray p.437)
    }
    pl->_outputDev->note(pl->_track2channel[track], note, onVelo, offVelo, gateTime);

    pl->_curP += 12;
    return 0;
}

int EUPPlayer_cmd_ax(int cmd, EUPPlayer *pl)
{
    /* polyphonic aftertouch */
    DB_PROCESSING("Polyphonic after touch");

	warn_once(0, "polyphonic aftertouch");
    return EUPPlayer_cmd_NOTSUPPORTED(cmd, pl);
}

int EUPPlayer_cmd_bx(int cmd, EUPPlayer *pl)
{
    /* control change */
    WAIT4NEXTSTEP;
    DB_PROCESSING("Control change");
    int track = (pl->_curP)[1];
    int control = (pl->_curP)[4];
    int value = (pl->_curP)[5];
    pl->_outputDev->controlChange(pl->_track2channel[track], control, value);
    pl->_curP += 6;
    return 0;
}

int EUPPlayer_cmd_cx(int cmd, EUPPlayer *pl)
{
    /* program change */
    WAIT4NEXTSTEP;
    DB_PROCESSING("Program change");
    int track = (pl->_curP)[1];
    int num = (pl->_curP)[4];
    pl->_outputDev->programChange(pl->_track2channel[track], num);
    pl->_curP += 6;
    return 0;
}

int EUPPlayer_cmd_dx(int cmd, EUPPlayer *pl)
{
    /* channel aftertouch */
    DB_PROCESSING("Channel after touch");

	warn_once(1, "polyphonic aftertouch");
    return EUPPlayer_cmd_NOTSUPPORTED(cmd, pl);
}

int EUPPlayer_cmd_ex(int cmd, EUPPlayer *pl)
{
    /* pitch bend */

	// this is basically the MIDI command, i.e. the data bytes in MIDI are 7-bits. the 14-bit
	// bend value is interpreted like this (it isn't handled correctly in the original eupplayer
	// impl): 0x0 means 2 semitones DOWN; 0x2000 means "unchanged"; 0x3fff means (almost) 2 semitones UP

    WAIT4NEXTSTEP;
    DB_PROCESSING("Pitch bend");

    int track = (pl->_curP)[1];
	int value = ((pl->_curP)[4]) | (((pl->_curP)[5]) << 7);
    pl->_outputDev->pitchBend(pl->_track2channel[track], value);

    pl->_curP += 6;
    return 0;
}

int EUPPlayer_cmd_f0(int cmd, EUPPlayer *pl)
{
    /* Exclusive status */

	// testcase: Crocus Garden (the song uses this 10x directly at the start
	// (always track=0); each time using the exact same "41 10 42 12 40 01" payload; why 10x?)
	// testcase: RE_SYNC 4 settings, e.g. "41 10 42 12 00 00", "41 10 42 12 40 1b", etc
	// the first 4 bytes are always the same..

	// might be direct "TL" register pokes to override the TL from the instrument buffer?
	// but without bank-selector this doesn't make sense, also 10x the same setting?

	_in_progress_Sysex = 1;		// just used to skip the bogus warnings
	_track_Sysex= (pl->_curP)[1];

    DB_PROCESSING("Exclusive status");
	warn_once(2, "Exclusive status");

    return EUPPlayer_cmd_NOTSUPPORTED(cmd, pl);
}

int EUPPlayer_cmd_f1(int cmd, EUPPlayer *pl)
{
    /* undefined */
    DB_PROCESSING("Undefined");
    return EUPPlayer_cmd_INVALID(cmd, pl);
}

int EUPPlayer_cmd_f2(int cmd, EUPPlayer *pl)
{
    /* bar marker */
    WAIT4NEXTSTEP;
    DB_PROCESSING("Bar");

#if OVERSTEP == 0 || OVERSTEP == 2
    pl->_stepTime = 0;
    // I think it should be like this if it's according to the specification
    // However, it seems that the genuine player is doing something even stranger.
#endif
#if OVERSTEP == 1
    pl->_stepTime -= stepTime;
    // This is like heplay
#endif
    pl->_curP += 6;
    return 0;
}

int EUPPlayer_cmd_f3(int cmd, EUPPlayer *pl)
{
    /* undefined */
    DB_PROCESSING("Undefined");
    return EUPPlayer_cmd_INVALID(cmd, pl);
}

int EUPPlayer_cmd_f4(int cmd, EUPPlayer *pl)
{
    /* undefined */
    DB_PROCESSING("Undefined");
    return EUPPlayer_cmd_INVALID(cmd, pl);
}

int EUPPlayer_cmd_f5(int cmd, EUPPlayer *pl)
{
    /* undefined */
    DB_PROCESSING("Undefined");
    return EUPPlayer_cmd_INVALID(cmd, pl);
}

int EUPPlayer_cmd_f6(int cmd, EUPPlayer *pl)
{
    /* undefined */
    DB_PROCESSING("Undefined");
    return EUPPlayer_cmd_INVALID(cmd, pl);
}

int EUPPlayer_cmd_f7(int cmd, EUPPlayer *pl)
{
    /* END OF EXCLUSIVE */

	// seems that songs that use "Start of exclusive" don't even bother to call
	// this to end the msg... see Crocus Garden
    DB_PROCESSING("End of exclusive");

    return EUPPlayer_cmd_NOTSUPPORTED(cmd, pl);
}

int EUPPlayer_cmd_f8(int cmd, EUPPlayer *pl)
{
    /* tempo */
    WAIT4NEXTSTEP;
    DB_PROCESSING("Tempo");
    int t = 30 + (pl->_curP)[4] + ((pl->_curP)[5] << 7);
    pl->tempo(t);
    (pl->_curP) += 6;
    return 0;
}

int EUPPlayer_cmd_f9(int cmd, EUPPlayer *pl)
{
    /* undefined */
    DB_PROCESSING("Undefined");

    return EUPPlayer_cmd_INVALID(cmd, pl);
}

int EUPPlayer_cmd_fa(int cmd, EUPPlayer *pl)
{
    /* USER CALL PROGRAM */
    DB_PROCESSING("User call program");

	warn_once(3, "User call program");
    return EUPPlayer_cmd_NOTSUPPORTED(cmd, pl);
}

int EUPPlayer_cmd_fb(int cmd, EUPPlayer *pl)
{
    /* pattern number */
    DB_PROCESSING("Pattern number");

	warn_once(4, "Pattern number");
    return EUPPlayer_cmd_NOTSUPPORTED(cmd, pl);
}

int EUPPlayer_cmd_fc(int cmd, EUPPlayer *pl)
{
    /* TRACK COMMAND */

	// test case: "Long Afternoon on Earth /whoo", "blizzard valley" use this...

    DB_PROCESSING("Track command");
	warn_once(5, "Track command");
    return EUPPlayer_cmd_NOTSUPPORTED(cmd, pl);
}

int EUPPlayer_cmd_fd(int cmd, EUPPlayer *pl)
{
    /* DATA CONTINUE */
    WAIT4NEXTSTEP;
    DB_PROCESSING("Data continue");
    pl->stopPlaying(); /* originally paused */

	warn_once(6, "Data continue");
    return EUPPlayer_cmd_NOTSUPPORTED(cmd, pl);
}

int EUPPlayer_cmd_fe(int cmd, EUPPlayer *pl)
{
    /* end marker */
    WAIT4NEXTSTEP;
    DB_PROCESSING("End mark");
    DB(("EUPPlayer: playing terminated.\n"));
    pl->stopPlaying();
    return 1;
}

int EUPPlayer_cmd_ff(int cmd, EUPPlayer *pl)
{
    /* dummy code */
    DB_PROCESSING("Dummy");
    pl->_curP += 6;
    return 0;
}

typedef int (*CommandProc)(int, EUPPlayer *);

static CommandProc const _fCommands[0x10] = {
    EUPPlayer_cmd_f0, EUPPlayer_cmd_f1, EUPPlayer_cmd_f2, EUPPlayer_cmd_f3,
    EUPPlayer_cmd_f4, EUPPlayer_cmd_f5, EUPPlayer_cmd_f6, EUPPlayer_cmd_f7,
    EUPPlayer_cmd_f8, EUPPlayer_cmd_f9, EUPPlayer_cmd_fa, EUPPlayer_cmd_fb,
    EUPPlayer_cmd_fc, EUPPlayer_cmd_fd, EUPPlayer_cmd_fe, EUPPlayer_cmd_ff,
};

int EUPPlayer_cmd_fx(int cmd, EUPPlayer *pl)
{
    return (_fCommands[cmd & 0x0f])(cmd, pl);
}

/* EUPPlayer */

EUPPlayer::EUPPlayer()
{
	memset(_warn_once, 1, sizeof(_warn_once));

	_in_progress_Sysex = 0;

    for (int i = 0; i < _maxTrackNum; i++) {
        _track2channel[i] = 0;
    }

    _outputDev = NULL;
    _stepTime = 0;
    _tempo = -1;
    this->stopPlaying();
}

EUPPlayer::~EUPPlayer()
{
    this->stopPlaying();
}

void EUPPlayer::outputDevice(PolyphonicAudioDevice *outputDev)
{
    this->stopPlaying();
    _outputDev = outputDev;
    this->tempo(this->tempo());
}

void EUPPlayer::mapTrack_toChannel(int track, int channel)
{
    if (track<0) return;
    if (track>=_maxTrackNum) return;
    if (channel<0) return;
    assert(track >= 0);
    assert(track < _maxTrackNum);
    assert(channel >= 0);
    //assert(channel < _maxChannelNum);

    _track2channel[track] = channel;
}

void EUPPlayer::startPlaying(u_char const *ptr)
{
    _isPlaying = 0;
    if (ptr != NULL) {
        _curP = ptr;
        _isPlaying = 1;
        _stepTime = 0;
    }

#if 0
    for (int track = 0; track < _maxTrackNum; track++) {
        _outputDev->programChange(_track2channel[track], 0);
    }
#endif
}

void EUPPlayer::stopPlaying()
{
    _isPlaying = 0;
}

bool EUPPlayer::isPlaying() const
{
    return (_isPlaying) ? true : false;
}


// note: seems that eupplayer impl is based on the command description
// from the "red book" (all the described cases are present - though not all are handled)

static CommandProc const _commands[0x08] = {
  EUPPlayer_cmd_8x, EUPPlayer_cmd_9x, EUPPlayer_cmd_ax, EUPPlayer_cmd_bx,
  EUPPlayer_cmd_cx, EUPPlayer_cmd_dx, EUPPlayer_cmd_ex, EUPPlayer_cmd_fx,
};

int EUPPlayer_cmd_INVALID(int cmd, EUPPlayer *pl)
{
    std::cerr << "EUPPlayer: invalid command '" << std::hex << cmd << "'.\n";
    DB(("EUPPlayer: invalid command '%02x'.\n", cmd));
    (pl->_curP) += 6;
    return 1;
}

int EUPPlayer_cmd_NOTSUPPORTED(int cmd, EUPPlayer *pl)
{
//    std::cerr << "EUPPlayer: not supported command '" << std::hex << cmd << "'.\n";
    DB(("EUPPlayer: not supported command '%02x'.\n", cmd));
    (pl->_curP) += 6;
    return 1;
}

void EUPPlayer::nextTick(bool nosound)
{
    // steptime advance one step

    if (this->isPlaying())
        for (;;) {
            int cmd = *_curP;
            CommandProc cmdProc = EUPPlayer_cmd_INVALID;

            if (cmd >= 0x80) {
                cmdProc = _commands[((cmd-0x80)>>4) & 0x07];
            }
            if (cmdProc(cmd, this)) {
				if (_in_progress_Sysex) {

					// END OF EXCLUSIVE doensn't even seem to be used
					// so just skip until there is a regular command

					fprintf(stderr, "track: %d - %02x %02x %02x %02x %02x %02x\n", _track_Sysex, _curP[0], _curP[1], _curP[2], _curP[3], _curP[4], _curP[5] );

					while ((*_curP) < 0x80) {
						_curP += 6;
					}
					_in_progress_Sysex= 0;
				} else {
					break;
				}
            }
        }

    if (_outputDev != NULL) {
        _outputDev->nextTick(nosound);
    }

    _stepTime++;
}

int EUPPlayer::tempo() const
{
    return _tempo;
}

void EUPPlayer::tempo(int t)
{
	// tempo in bpm (range 30 - 280)
    _tempo = t;

    if (_tempo <= 0) {
        return;
    }

    int t0 = 96 * t;
    struct timeval tv;
    tv.tv_sec = 60 / t0;
    tv.tv_usec = ((60*1000*1000) / t0) - tv.tv_sec*1000*1000;
    if (_outputDev != NULL) {
        _outputDev->timeStep(tv);
    }
}
