/*
* Call it cheating...
*
* The hacks are a means to explore the features that would be
* needed to properly deal with the affected songs.
*
* The remaining hacks address the lack of VIC sprite related
* bad-cycle handling and the special case "interrupt during RTI".
*
* WebSid (c) 2020 Jürgen Wothke
* version 0.94
*/

#include <string.h>

#include "hacks.h"

#include "memory.h"
#include "vic.h"
#include "cpu.h"

static uint8_t (*_defaultStunFunc)(uint8_t x, uint16_t y, uint8_t cpr);

/*
* Immigrant_Song.sid: hardcore badline timing
* I don't hear much of a difference.. might just ditch this one..
*/
static void patchImmigrantSongIfNeeded(uint16_t init_addr) {
	// the timing of this song is absolutely unforgiving.. if NMI fires 1 cycle to
	// soon or one cycle to late then it will eventually hit a badline
	uint8_t pattern[] = {0xd1,0x0b,0x20,0xcc,0x0c,0x20,0x39};

	if ((init_addr == 0x080d) && memMatch(0x0826, pattern, 7)) {
		// just disable the display (which is causing the trouble in the first place)
		memWriteRAM(0x0821, 0x0b);
	}
}

static uint8_t disabledStun(uint8_t x, uint16_t y, uint8_t cpr) {
	// completely disable regular badline logic
	return 0;
}

/*
* Utopia_tune_6.sid: hardcore sprite timing
*/
static void patchUtopia6IfNeeded(uint16_t init_addr) {
	// timing critical song that depends on sprite-delays.. without the proper
	// bad-sprite-cycles the timing is off, and the D011-based badline
	// avoidance logic actually causes MORE badlines (slowing the song down).

	uint8_t pattern[] = {0xce, 0x16, 0xd0, 0xee, 0x16, 0xd0};
	if ((init_addr == 0x9200) && memMatch(0x8b05, pattern, 6)) {
		vicSetStunImpl(&disabledStun);

		// disable IRQ ACKN.. causing the IRQ handler to be immediately
		// called again (did not check why this is needed)
		memWriteRAM(0x8E49, 0x0);
	}
}
/*
* Mr_Meaner.sid: NMI/IRQ handling issue
*/
static void patchMrMeanerIfNeeded(uint16_t init_addr) {
	// timing critical song with extremely slow NMI digi player: with a 9kHz-10kHz digi sample rate, the 
	// up to ~100 cycles used by its NMI routine are very close to the maximum cycles actually available
	// between NMIs. The song has the display turned on and probably the original timing actually manages 
	// to navigate around the badlines.
	// But the emulator eventually ends up in a state where the INT-flag is no longer cleared - leading
	// to IRQs no longer being triggered. There still must be some flaw in WebSid's badline impl or a 
	// flaw in some border case for some simultaneous NMI and IRQ scenario. The problem does not appear 
	// while the badline handling is disabled.

	uint8_t pattern[] = {0xad, 0x0e, 0xdd, 0xad, 0xd9, 0x16, 0x8d, 0xd6, 0x16};
	if ((init_addr == 0x1000) && memMatch(0x157c, pattern, 9)) {
		vicSetStunImpl(&disabledStun);
	}
}


/*
findings regarding sprites-bad-cycles handling:

- sprite size: 24x21 pixels
- PAL: visible area within the border is 200 lines high. screen has 312
lines. vblank takes around 30 scanlines, i.e. total of around 282 visible
scanlines.. supposing that sprites start at 0 then the maximum reachable by
an Y-expanded sprite would be 255+42=> 297, i.e. the complete visible screen
can be covered with sprites but there are some lines during vblank where
sprites will have no effect


summary of background infos from "VIC by Christian Bauer":

per shown sprite and raster the VIC may perform 4 bus accesses: "psss" ("p" is
the "sprite data pointer"  and "s" is "sprite data") these accesses occur
in 4 consecutive "half-cycles" the 1st and 3rd of which are handled during the
phase always allocated to the VIC and only the 2nd and 4th are performed in the
phase normally allocated to the CPU.. i.e. they cause bad-cyles for the CPU)

"..p-accesses are done in every raster line and not only when the belonging
sprite is just displayed" => doesn't matter to the CPU since they are performed
in the half-cycle phase dedicated to the VIC anyway

"..s-accesses are done in every raster line in which the sprite is visible
(for the sprites 0-2, it is always in the line before.." each sprite has a
hard-coded cycle-position when this access is performed (e.g. for sprite 3
it is cycles 0/1).

"In the first phases of cycle 55 and 56 (PAL only!), the VIC checks for every
sprite if the corresponding MxE bit [i.e. "sprite enable"] in register $d015 is
set and the Y coordinate of the sprite (odd registers $d001-$d00f) match the
lower 8 bits of RASTER".. this spec seems to be INCORRECT! what about the
handling of the (extended) height of the sprite? respective "sprite data"
needs to be read for every line where it is supposed to be displayed!

if "sprite-data" needs to be read the bus is blocked for CPU-reads 3 cycles
earlier (if there are at least 4 cycle "gaps" due to 2 or more sprites that
are NOT enabled, then this may repeat!, example: enabled 0,3,6 => then a
3-cycle CPU-read stun would be used before EACH of these "sprite data"
accesses)

=> assumption: sprite enabling is checked/setup once at cycle 55/56
(unfortunately the respective explanations in Bauer's text are ambiguous)

"In the first phase of cycle 58... display of the sprite is turned on"
=> how does this relate to what "happend" in cycles 55/56? supposing sprite 0
enable it turned ON in cycle 57.. the required 3-cycle CPU-read stun would
NOT have been triggered so there is NO WAY the VIC could steal the necessary
"sprite data" read cycles in cyles 58/59!

conclusions:

- assuming that respective sprite enabling can be "statically" updated once
per scanline (e.g. at cycle 56), the following would still need to be
implemented (in worst case the timing of VIC register changes might need to
be tracked - including sprite y-coords and y-expansion status?):

  * different handling for sprites 0-2 and 3-7 (properly map the setting to
    the 2 scanlines that it affects)
  * calculate sprites that are visible on a specific scanlines (which
    makes for quite a few extra comparisons)
  * rules for keeping bus blocked (e.g. if a disabled sprite is between
    two enabled ones, etc) and for prepending 3-cycle "CPU-read stun"
	must be checked in each cycle (might be done with a precalculated
    256x63 lookup table, so the extra runtime costs might be tolerable)

- the specifications in the available docs leave certain details rather sketchy
and there is a considerable risk that a respective impl would still be incorrect,
i.e. songs that really depend on cycle exact delay-timing would still fail (e.g.
a "off by one" may accumulate and cause "badline avoidance" scemes to
catastrophically fail.. completely destroying the song's timing - see
Utopia_tune_6.sid).

- very few songs actually depend on sprite-bad-cycles (and those that do use
a limited set of sprite-configurations) => until there is a signicicant
amount of songs that would actually benefit it isn't worth the trouble to
add a generic implementation
*/
static uint8_t swallowStun(uint8_t x, uint16_t y, uint8_t cpr) {
	// timing with all 8 sprites active on the line:
	// sprites 3-7 use 10 cycles starting at cycle 0
	// sprites 0-2 use 6 cycles at very end of line
	// before sprite 0 there is 3 cycle "stun on read"

	if (memReadIO(0xd015) == 0xff) {
		if (y < 0x10 || y > (0x10 + 0x29 * 6 + 1)) {
			// see used sprite Y positions of 6 lines of sprites
		} else {
			if ((x < 10)) {
				return 2;
			}
			uint8_t s= cpr - 4 - 3;	// should be 6 not 4! (but sounds better like this)
			if ((x >= s)) {
				if ((x >= (s + 3))) {
					return 2;
				} else {
					return 1;
				}
			}
		}
	}
	return 0;
}

static void patchSwallowIfNeeded(uint16_t init_addr) {
	// Comaland_tune_3.sid & Fantasmolytic_tune_2.sid

	// timing critical song that depends on sprite-delays.. switches
	// between 0 (0 added bad-cycles) and 8 sprites (~17 added bad cycles)
	// sprites are not shown on all lines, i.e. there are lines without
	// the slowdown (regular badlines seem to be irrelevant here)

	uint8_t pattern1[] = {0x8E,0x16,0xD0,0xA5,0xE0,0x69,0x29};
	uint8_t pattern2[] = {0x8E,0x16,0xD0,0xA5,0xC1,0x69,0x29};
	if ((init_addr == 0x2000) &&
		(memMatch(0x28C8, pattern1, 7) || memMatch(0x28C8, pattern2, 7))) {
		vicSetStunImpl(&swallowStun);
	}
}

static uint8_t weAreDemoStun(uint8_t x, uint16_t y, uint8_t cpr) {
	// "we are demo" uses a total of 8 sprites that are statically
	// positioned on different lines of the screen.. the used grouping causes
	// bad-cycles at different positions of the scanline..

	// known limitation: at the boundary lines between the blocks the "line-break"
	// would need to be handled differently (see "line-break" at cycle 58)

	if (memReadIO(0xd015) == 0xff) {
		// flaw: for line 67 sprite 2 would need to already have been read
		// in line 66, etc (see other boundary lines)
		if (y >= 67 && y < 88) {
			// sprites 2,3,4; i.e. cycles 62,63//1,2//3,4
			if (x <= 4)
				return 2;
			else if (x >= (62 - 3)) return (x >= 62) ? 2 : 1;

			} else if (y >= 88 && y < 123) {
			// sprites 5,6,7; i.e. cycles 5,6//7,8//9,10
			if (x >= (5 - 3) && x < 11) return (x >= 5) ? 2 : 1;

		} else if (y >= 123 && y < 165){
			// sprites 0,1; i.e. cycles 58,59//60,61
			if (x >= (58 - 3) && x < 62) return (x >= 58) ? 2 : 1;
		}
	}
	return  _defaultStunFunc(x, y, cpr); // use original badline impl
}

static void patchWeAreDemoIfNeeded(uint16_t init_addr) {
	// We_Are_Demo_tune_2.sid: another example for sprite related
	// bad-cycles.

	uint8_t pattern[] = {0x8E,0x18,0xD4,0x79,0x00,0x09,0x85,0xE1};
	if ((init_addr == 0x0c60) && (memMatch(0x0B10, pattern, 8))) {
		_defaultStunFunc = vicGetStunImpl();
		vicSetStunImpl(&weAreDemoStun);
	}
}

static void patchGraphixsmaniaIfNeeded(uint16_t init_addr) {
	// Graphixmania_2_part_6.sid: For the sake of good old MDA times..
	// making Tim's song greater.. (seems Mat had forgotten to disable the
	// D418 write in the regular IRQ player when adding the NMI digi
	// player - which leads to unnecessary noise even on the real
	// hardware..)

		uint8_t pattern[] = {0xB8,0x29,0x0F,0x8D,0x18,0xD4};
	if ((init_addr == 0x7000) && (memMatch(0x1214, pattern, 6))) {
		memWriteRAM(0x48F9, 0xad);
	}
}

static void patchSynthmeldIfNeeded(uint16_t init_addr) {
	// Synthmeld.sid is a nice testcase for a potentially fixable timing
	// flaw that to this day still exists in the WebSid impl (other
	// emulators like sidplay26 ACID64 seem to equally struggle with this
	// song - maybe suffering from the same or from a different
	// implementation flaw)

	// The problem is caused by a timer-B (chained to timer-A) IRQ not
	// triggering at the exact right moment. This might be due to a flaw
	// in the CIA impl, the IRQ triggering impl, the exact badline timing
	// (no sprites are involved), or the song might just expect a
	// different CIA model .

	uint8_t pattern[] = {0xEE,0x9C,0xE8,0xA9,0x07,0x10,0x3A,0xC9,0xE0,0x90,0x1F};
	if ((init_addr == 0x0b00) && (memMatch(0xB398, pattern, 11))) {
		// it seems that the basic hardcoded play-loop is correctly timed
		// for the regular badline handling and the problematic IRQs are
		// probably meant to replace the some part to handle the logo-
		// animation of the original demo

		// not needed since respective branches that would trigger
		// IRQs are no longer used:
		// memWriteRAM(0xAB58, 0x80);	// disable all timer IRQs

		memWriteRAM(0xB39C, 0x00);	// set the lda #$00, i.e. always use the same branch
		memWriteRAM(0xB398, 0xea);	// disable counter (keep 6-cycle timing)
		memWriteRAM(0xB399, 0xea);	// disable counter
		memWriteRAM(0xB39a, 0xea);	// disable counter
	}
}

static void patchGamePlayerIfNeeded(uint16_t init_addr) {
	// Game_Player.sid is a nice testcase for another timing flaw regarding 
	// NMI that interrupts RTI
	uint8_t pattern[] = {0x8D,0xD8,0x0B,0xAD,0xAE,0x21};
	if ((init_addr == 0x0810) && (memMatch(0x0B85, pattern, 6))) {
		cpuHackNMI(1);	// incorrect NMI behavior: prevent NMI from firing during RTI
	}
}

static void patchFeelingGoodIfNeeded(uint16_t init_addr) {
	// Feeling_Good.sid is a another testcase for timing flaw regarding 
	// NMI that interrupts RTI
	uint8_t pattern[] = {0xee,0xe5, 0x1c, 0xee, 0x1a, 0x1d};
	if ((init_addr == 0x1000) && (memMatch(0x1cf7, pattern, 6))) {
		cpuHackNMI(1);	// incorrect NMI behavior: prevent NMI from firing during RTI
	}
}

static void patch4NonBlondesIfNeeded(uint16_t init_addr) {
	// 4_Non_Blondes-Whats_Up_Remix.sid is a nice testcase for a VIC(badlines) related flaw	
	
	uint8_t pattern[] = {0x84,0xFD,0xA0,0x00,0xB1,0xFA};
	if ((init_addr == 0x082E) && (memMatch(0x0903, pattern, 6))) {
		memWriteRAM(0x0835, 0x0b);	// disable display
	}
}

void hackIfNeeded(uint16_t init_addr) {

	cpuHackNMI(0);	// disable incorrect NMI behavior
	
	patchMrMeanerIfNeeded(init_addr);
	
	patchFeelingGoodIfNeeded(init_addr);

	patch4NonBlondesIfNeeded(init_addr);
	
	patchGamePlayerIfNeeded(init_addr);

	patchSynthmeldIfNeeded(init_addr);

	patchImmigrantSongIfNeeded(init_addr);

	patchUtopia6IfNeeded(init_addr);

	patchSwallowIfNeeded(init_addr);

	patchWeAreDemoIfNeeded(init_addr);

	patchGraphixsmaniaIfNeeded(init_addr);
}
