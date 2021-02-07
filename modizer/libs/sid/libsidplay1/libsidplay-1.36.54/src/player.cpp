//
// /home/ms/source/sidplay/libsidplay/RCS/player.cpp,v
//

#include "player.h"
#include "myendian.h"
#include "6510_.h"

extern ubyte playRamRom;  // MPU bank-switch setting for player call

// These texts are used to override the sidtune settings.
static const char text_PAL_VBI[] = "50 Hz VBI (PAL)";
static const char text_PAL_CIA[] = "CIA 1 Timer A (PAL)";
static const char text_NTSC_VBI[] = "60 Hz VBI (NTSC)";
static const char text_NTSC_CIA[] = "CIA 1 Timer A (NTSC)";

static const int FREQ_VBI_PAL = 50;   // Hz
static const int FREQ_VBI_NTSC = 60;  //

// Table used to check sidtune for usage of PlaySID digis.
static const uword c64addrTable[] =
{
	// PlaySID extended SID registers (0xd49d left out).
	//0xd41d, 0xd41e, 0xd41f,  // SID is too often written to as 32 bytes!
	0xd43d, 0xd43e, 0xd43f,
	0xd45d, 0xd45e, 0xd45f, 0xd47d, 0xd47e, 0xd47f,
	//0xd51d, 0xd51e, 0xd51f,  // SID is too often written to as 32 bytes!
	0xd53d, 0xd53e, 0xd53f,
	0xd55d, 0xd55e, 0xd55f, 0xd57d, 0xd57e, 0xd57f
};
// Number of addresses in c64addrTable[].
static const int numberOfC64addr = sizeof(c64addrTable) / sizeof(uword);
static ubyte oldValues[numberOfC64addr];


bool sidEmuInitializeSong(emuEngine & thisEmuEngine,
						  sidTune & thisTune,
						  uword songNumber)
{
	// Do a regular song initialization.
	bool ret = sidEmuInitializeSongOld(thisEmuEngine,thisTune,songNumber);
	if (ret && (thisEmuEngine.config.digiPlayerScans!=0))
	{
		// Run the music player for a couple of player calls and check for
		// changes in the PlaySID extended SID registers. If no digis are
		// used, apply a higher amplification on each SID voice. First
		// check also covers writings of the player INIT routine. Old values
        // are stored before song INIT.
		bool useDigis = false;
		int loops = thisEmuEngine.config.digiPlayerScans;
		while (loops)
		{
			for (int i = 0; i < numberOfC64addr; i++)
			{
				if (oldValues[i] != c64mem2[c64addrTable[i]])
				{
					useDigis = true;
					break;
				}
				oldValues[i] = c64mem2[c64addrTable[i]];
			}
			if (useDigis)
			{
				break;
			}
			uword replayPC = thisTune.getPlayAddr();
			// playRamRom was set by sidEmuInitializeSongOld(..).
			if ( replayPC == 0 )
			{
				playRamRom = c64mem1[1];
				if ((playRamRom & 2) != 0)  // isKernal?
				{
					replayPC = readLEword(c64mem1+0x0314);  // IRQ
				}
				else
				{
					replayPC = readLEword(c64mem1+0xfffe);  // NMI
				}
			}
			interpreter(replayPC,playRamRom,0,0,0);
			loops--;
		};
		thisEmuEngine.amplifyThreeVoiceTunes(!useDigis);
		// Here re-init song to start at beginning.
		ret = sidEmuInitializeSongOld(thisEmuEngine,thisTune,songNumber);
	}
	return ret;
}


bool sidEmuInitializeSongOld(emuEngine & thisEmuEngine,
							 sidTune & thisTune,
							 uword songNumber)
{
	if (!thisEmuEngine.isReady || !thisTune.status )
	{
		return false;
	}
	else
	{
		// ------------------------------------------- Determine clock/speed.
		
		// Get speed/clock setting for song and preselect
		// speed/clock descriptions strings, reg = song init akkumulator.
		ubyte reg = thisTune.selectSong(songNumber) -1;

		const char* the_description;

		ubyte the_clock = thisTune.info.clockSpeed;
        // Choose clock speed emu user prefers. If sidtune doesn't
        // contain clock speed setting (=0), use emu setting.
        if ( the_clock == SIDTUNE_CLOCK_ANY )
            the_clock &= thisEmuEngine.config.clockSpeed;
        else if ( !the_clock )
            the_clock = thisEmuEngine.config.clockSpeed;
        
		ubyte the_speed = thisTune.info.songSpeed;
		
		if (thisEmuEngine.config.forceSongSpeed)
		{
			the_clock = thisEmuEngine.config.clockSpeed;
		}

		// Select speed description string.
		if (the_clock == SIDTUNE_CLOCK_PAL)
		{
			if (the_speed == SIDTUNE_SPEED_VBI)
			{
				the_description = text_PAL_VBI;
			}
			else  //if (thisTune.info.songSpeed == SIDTUNE_SPEED_CIA_1A)
			{
				the_description = text_PAL_CIA;
			}
		}
		else  //if (the_clock == SIDTUNE_CLOCK_NTSC)
		{
			if (the_speed == SIDTUNE_SPEED_VBI)
			{
				the_description = text_NTSC_VBI;
			}
			else  //if (thisTune.info.songSpeed == SIDTUNE_SPEED_CIA_1A)
			{
				the_description = text_NTSC_CIA;
			}
		}
		
		// Substitute correct VBI frequency.
		if ((the_clock==SIDTUNE_CLOCK_PAL) &&
			(the_speed==SIDTUNE_SPEED_VBI))
		{
			the_speed = FREQ_VBI_PAL;
		}
		
		if ((the_clock==SIDTUNE_CLOCK_NTSC) &&
			(the_speed==SIDTUNE_SPEED_VBI))
		{
			the_speed = FREQ_VBI_NTSC;
		}
			
		// Transfer the speed settings to the emulator engine.
		// From here on we don't touch the SID clock speed setting.
        extern void sidEmuConfigureClock( int );
        sidEmuConfigureClock( the_clock );
		extern void sidEmuSetReplayingSpeed(int clockMode, uword callsPerSec);  // --> 6581_.cpp
		sidEmuSetReplayingSpeed(the_clock,the_speed);

        // Make available chosen speed setting in userspace.
        thisTune.info.clockSpeed = the_clock;
        thisTune.info.songSpeed = the_speed;
		thisTune.info.speedString = the_description;
 
		// ------------------------------------------------------------------
		
		thisEmuEngine.MPUreset();
				
		if ( !thisTune.placeSidTuneInC64mem(thisEmuEngine.MPUreturnRAMbase()) )
		{
			return false;
		}

		if (thisTune.info.musPlayer)
		{
			thisTune.MUS_installPlayer(thisEmuEngine.MPUreturnRAMbase());
		}
		
		thisEmuEngine.amplifyThreeVoiceTunes(false);  // assume digis are used
		if ( !thisEmuEngine.reset() )  // currently always returns true
		{
			return false;
		}

        if (thisEmuEngine.config.digiPlayerScans!=0)
        {
            // Save the SID registers to allow later comparison.
            for (int i = 0; i < numberOfC64addr; i++)
            {
                oldValues[i] = c64mem2[c64addrTable[i]];
            }
        }
		
		// In PlaySID-mode the interpreter will ignore some of the parameters.
		//bool retcode =
		interpreter(thisTune.info.initAddr,c64memRamRom(thisTune.info.initAddr),reg,reg,reg);
		playRamRom = c64memRamRom(thisTune.info.playAddr);
		
		// This code is only used to be able to print out the initial IRQ address.
		if (thisTune.info.playAddr == 0)
		{
			// Get the address of the interrupt handler.
			if ((c64mem1[1] & 2) != 0)  // isKernal?
			{
				thisTune.setIRQaddress(readEndian(c64mem1[0x0315],c64mem1[0x0314]));  // IRQ
			}
			else
			{
				thisTune.setIRQaddress(readEndian(c64mem1[0xffff],c64mem1[0xfffe]));  // NMI
			}
		}
		else
		{
			thisTune.setIRQaddress(0);
		}

#if defined(SIDEMU_TIME_COUNT)
		thisEmuEngine.resetSecondsThisSong();
#endif
		return true;

	}  // top else (emu&sidtune okay)
}
