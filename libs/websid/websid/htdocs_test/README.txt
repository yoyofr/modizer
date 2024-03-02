Limitations
-----------
The test suite (Wolfgang Lorenz's) is run using a web-page and a slighty patched WebSid version
that allows to load respective test-programs instead of the usual music files (CAUTION: the loader
expects the page to be in a top-level folder called "websid_test".). 

The test driver is not automated but basically a manual music player. This means than 
tests run as slowly as they would on the real HW. It also means that it is somewhat tiresome
to manually start the tests one after the other (eventhough they are organized in a sequential 
playlist).

ToDo: Use dedicated driver that runs the emulation as fast as possible (make sure to not block 
the browser completly) and add some reliable automatic detection if a test was successful. 
Automatically chain tests as long as they are successful or just print a consolidated 
status at the end (to at least allow for unattended regression tests).


Reference
---------
All the tests seem to succeed when run in Vice 3.4 ("C64 old PAL, CIA 6526 (old)" setting,
running the exact same test programs used here). Ideally the test results should be 
the same when the tests are run using the WebSid emulator.

The CPU tests are rather obvious to interpret and are therefore not listed below.
Also some of the timing related tests are for functionalities irrelevant to music playback  
and they are explicitly excluded (see playlist in index.html).

Expected test bahavior, i.e. output in Vice: When successful all the tests eventully print 
one line with the name of the test followed by " - OK".

ALL THE TESTS then print the name of the next test on the next line (i.e. the respective
chaining is hardcoded into the tests and the tests are invoked using the chaining mode so that
the end of a test can be more easily detected)

If a test does NOT print anything after the test name, then it has crashed, i.e. it is 
a FAIL. Any additional output (besides "- OK" is some kind of FAIL that needs to be
investigated!

Most of the tests seem to be fast and "immediately" print out the complete line, e.g. 
	mmufetch
	cia1tb123
	cia1tab
	icr01
	imr
	oneshot
	cntdef

some tests have a short delay before printing the OK:
	cputiming
	irq
	nmi

finally the below slow tests take about 30 seconds before printing the OK (note: screen output
shows what the test has output so far and it is NOT the final status.. due to the fact that the
test is driven by the regular realtime player, it means than tests take as long as they would 
take on the real HW, i.e. wait for the " - OK" before moving to the next test):
	cia1ta 
	cia1tb 
	cia2ta 
	cia2tb



----------------------------------------------------------------------------------------------------


Current status:

The irrelevant tests are explicitly commented out in the index.html playlists. If certain irrelevant
parts of a test are disabled then a respective information message it printed in the browser's
console window.

All performed tests now successfully complete with "- OK".