# SpectreZX

Copyright (C) 2015 Juergen Wothke


## project structure

SpectreZX is based on ZXTune (zxtune-r3150). Everything needed for the WebAudio version is contained in this 
"emscripten" folder. The original ZXTune code is almost unchanged, i.e. should be trivial to 
merge future ZXTune fixes or updates.


## code changes
Some unused folders have been completely removed from "3rdparty" (curl.dll, FLAC, lame, ogg, runtime, sidplayfp, unrar, vorbis, xmp), and also folders 
"apps" & "test". The "boost" (partial; version 1.57.0) folder was added to remove the external dependency.  
Disabled some unused code via ifdef EMSCRIPTEN (e.g. SID/XMP). 

Unfortunatly the original ZXTune implementaton uses a non-portable approach of reading memory areas (i.e. input music files) 
directly into "packed" structs as a means to interpret them. While this approach may work on architectures that allow unaligned 
data access, it does NOT work here. Example: A compiler may expect a "short*" (16-bit) to point to an "even" memory addrsss and 
respective access logic is then based on that assumption. If that access logic later is used with some "odd" input address, chances 
are that it will fail miserably. The biggest effort of this project therefore was to patch the original ZXTune impl with more robust 
access logic for unaligned data (You may find the respectice patched areas by diff-ing with the original "zxtune-r3150" version 
or by searching for the keyword "EMSCRIPTEN".). 

Eventhough the code now seems to work reasonably well, it may still require additional fixes that I have missed. For the benefit 
of those that may wish to resume the fixing here some background infos: 

 1) Packed structs that contain built-in types or arrays of built-in types are fine:
	struct Dummy {
		short someArray[100];
	} __attribute__ ((packed))

	Even if a respective struct were populated from some unaligned memory address the compiler would correctly access the VALUES, e.g.
	
	short val= s->someArray[x]; // fine: the compiler will recognize the need for unaligned access (if any)
	
 2)	but the compiler will no longer know about unaligned data when simple pointers are involved:
	
	short *ptr= &s->someArray[x];	// fail!
	
	supposing that pointer is passed as an argument to some function that wishes to update the original memory location

	void f(short *ptr) {
		*ptr= 100;			// fail! (if memory address is not properly aligned)
	}

 3) to mitigate the above issue make sure to only propagate pointers to the packed structs - so that the compiler knows what 
    needs to be done, e.g. you may want to introduce extra wrappers:

	struct unalignedShort {
		short val;
	} __attribute__ ((packed))

	void f(unalignedShort *ptr) {
		ptr->val= 100;			// fine: compiler will know what to do..
	}

 4) Caution: From a usage point of view boost::array<..> may look like a built-in array but from an implementation point of view 
    it behaves very differently: Its access logic depends on the proper alignment of the array and it just won't work if it isn't 
	aligned! The same is true for any NON-built-in type. 



## build 

The "poor man's" make script first compiles separate sub-libraries within the "built" folder. These must be manually deleted 
in order to trigger a clean rebuild (the script itself DOES NOT detect which of the libs might be outdated)!

The code must be compiled using -DNO_DEBUG_LOGS because the built-in debug-messages will actually crash some of the players.
 

## known limitation

Because the original implementation remains largely untouched, the code doesn't cope with the async file 
loading approach normally taken by web apps. (You might want to have a look at my WebUADE for an 
example of a lib that has been patched to deal with load-on-demand..) Fortunately ZXTune currently 
does not seem to use any on demand file loads - if this ever was to change, then the current web 
port would need to be upgraded.


## License

This library is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at
your option) any later version. This library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

