# pt3player
<pre>
multi-pt3 player by Volutar 2021-12-21
Ayumi AY/YM synth engine by Peter Sovetov

Plays multiple pt3 songs simultaneously, assuming each per AY chip, up to 6. 

Run (For windows build):
playpt3.exe <song1.pt3> <song2.pt3> <song3.pt3>

or
Drag'n'drop multiple pt3's on playpt3.exe

Keys:

esc - exit
pgup/pgdown - change volume
f - switch fast forward (x4)
space - pause/unpause
r - reload song files, and keep playing / unpause
home - rewind to the beginning
left/right - rewind 2s backward/forward
1,2,3,4,5,6 - switch off/on particular track


Config (playpt3.txt):

sample_rate 44100 = sound samplerate
is_ym 0 = 1 means yamaha, 0 means AY
clock_rate 1750000 = AY/YM chip frequency
frame_rate 50 = interrupt rate
pan_a 10 = channel A, 10 means almost left
pan_b 50 = channel B, 50 means center
pan_c 90 = channel C, 90 means almost right
eqp_stereo_on 1 = non-linear stereo pan (sqrt)
dc_filter_on 1 = adjust DC level (when 1 - it moves values to center)
note_table -1 = force note table (below), -1 to default

Note tables first divisor (/difference):
0 - 0xC21
1 - 0xC22
2 - 0xEF8
3 - 0xD3E
4 - 0xD10
5 - 0xCDA /0x113
6 - 0xCDA /0x112
</pre>
