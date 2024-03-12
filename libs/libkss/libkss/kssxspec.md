# KSSX Format Spec
This document is an unofficial format specification of extended KSS (KSSX), derivative from [KSS Music Format v1.03](http://www.purose.net/befis/download/nezplug/kssspec.txt).

## HISTORY
The KSS file is a kind of machine memory image for Z80-based emulators to reproduce the SCC and PSG sound of MSX. The original KSS format was proposed by Cync. If the author remebers correctly, the name KSS stands for Konami Sound System and the original format only supports PSG and SCC. The original KSS player was distributed through internet without the official KSS format specification document.

Later Mamiya extended the format for MSX-MUSIC, MSX-AUDIO, SEGA mk3 and GameGear sound. Then he also documented [the format](http://www.purose.net/befis/download/nezplug/kssspec.txt).

# FILE FORMAT

```
0000   4 BYTES   magic 'KSSX'
0004   WORD(LE)  load address of Z80 address space (0000-FFFF)
0006   WORD(LE)  length of initial data in byte (0000-FFFF)
0008   ORD(LE)   init address of Z80 address space (0000-FFFF)
000A   WORD(LE)  play address of Z80 address space (0000-FFFF)
000C   BYTE      start(offset) no of the 1st bank
000D   BYTE      8/16kbytes banked extra data blocks support
                   bits 0-6: number of 8/16kbytes banked extra data blocks
                   bit 7: if set, this song uses KSS 8k mapper
000E   BYTE      extra header size 00H or 10H
000F   BYTE      extra sound chip support.
                   bit0: Use YM2413
                   bits 1: mode flag (SN76489)
                   bits 5-7: reserved. they *must* be 0
                  
                   if bit1 = '0', MSX mode
                     D76543210
                      0V0XXR0F
                       | | ||+- bit0: Use FMPAC
                       | | |+-- bit1: 0 (SN76489 is disabled)
                       | | +--- bit2: Use RAM
                       | +----- bit3-4: 1: MSX-AUDIO
                       |                2: Majutushi D/A
                       |                3: MSX-AUDIO (STEREO)
                       +------- bit6: 0: NTSC MODE
                                      1: PAL MODE

                   if bit1 = '1', SEGA mode
                     D76543210
                      0V00RG1F
                       |  |||+- bit0: Use FMUNIT
                       |  ||+-- bit1: 1 (SN76489 is enabled)
                       |  |+--- bit2: Use GG stereo
                       |  +---- bit3: Use RAM
                       +------- bit6: 0: NTSC MODE
                                      1: PAL MODE

0010    m BYTES   extra header data (see the next section)
0010+m  n BYTES   specified length initial data

0010+m+n  16kBYTES  1st 16k block of extra data(option)
4010+m+n  16kBYTES  2nd 16k block of extra data(option)
...

0010+m+n  8kBYTES   1st 8k block of extra data(option)
2010+m+n  8kBYTES   2nd 8k block of extra data(option)
...

```

## EXTRA HEADER DATA

```
0000    DWORD(LE)   offset to the end of file
0004    DWORD(LE)   reserved (must be 0)
0008    WORD(LE)    number of the first song
000A    WORD(LE)    number of the last song
000C    BYTE(LE)    PSG/SNG volume
000D    BYTE(LE)    SCC volume
000E    BYTE(LE)    MSX-MUSIC/FM-UNIT volume
000F    BYTE(LE)    MSX-AUDIO volume

volume = $80(MIN)...$E0(x1/4)...$F0(x1/2)...$00(x1)...$10(x2)...$10(x2)...$20(x4)..$7F(MAX)
(0.375dB/step)
```
