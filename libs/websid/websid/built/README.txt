note: This folder contains the precompiled "stereo" library stuff (the lib has been split in 2 to work around the 
commandline length limitations). Since this is a 3rd party (Google) lib that is uses as-is, I do not expect 
frequent changes nor the need to recompile.

The makeEmscripten.bat script therefore deals with this rather stupidly: Then the libs exist then they are just
used - regardless if the source code might have changed. The libs have to be manually deleted in order to 
trigger a recompile. (makeEmscripten.bat must be invoked repeatedly since it will only compile one lib per call)
