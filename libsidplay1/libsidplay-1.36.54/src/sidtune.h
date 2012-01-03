//
// /home/ms/source/sidplay/libsidplay/include/RCS/sidtune.h,v
//
// This class is independent from the rest of the emulator engine
// and therefore can be used separately.
// It provides an interface to sidtunes in various file formats.
//

#ifndef SIDPLAY1_SIDTUNE_H
#define SIDPLAY1_SIDTUNE_H


#include <fstream>
using std::ofstream;
#include "mytypes.h"

const uint classMaxSongs = 256;   // also file format limit
const uint infoStringNum = 5;     // maximum
const uint infoStringLen = 80+1;  // 80 characters plus terminating zero

const udword maxSidtuneFileLen = 65536+2+0x7C;  // C64KB+LOAD+PSID

#ifndef MODIZER_COMPIL
const int SIDTUNE_SPEED_VBI = 0;      // Vertical-Blanking-Interrupt
const int SIDTUNE_SPEED_CIA_1A = 60;  // CIA 1 Timer A

const int SIDTUNE_CLOCK_UNKNOWN = 0;  // These are also used in the
const int SIDTUNE_CLOCK_PAL = 0x01;   // emulator engine!
const int SIDTUNE_CLOCK_NTSC = 0x02;  // (binary)
const int SIDTUNE_CLOCK_ANY = (SIDTUNE_CLOCK_PAL|SIDTUNE_CLOCK_NTSC);

const int SIDTUNE_SIDMODEL_UNKNOWN = 0;
const int SIDTUNE_SIDMODEL_6581 = 0x01;
const int SIDTUNE_SIDMODEL_8580 = 0x02;
const int SIDTUNE_SIDMODEL_ANY = (SIDTUNE_SIDMODEL_6581|SIDTUNE_SIDMODEL_8580);
#endif

// An instance of this structure is used to transport values to
// and from the ``sidTune-class'';
struct sidTuneInfo
{
	// Consider the following fields as read-only, because the sidTune class
	// does not provide an implementation of: bool setInfo(sidTuneInfo&).
	// Currently, the only way to get the class to accept values which
	// are written to these fields is by creating a derived class.
	//
	const char* formatString;   // the name of the identified file format
	const char* speedString;    // describing the speed a song is running at
	uword loadAddr;
	uword initAddr;
	uword playAddr;
	uword startSong;
	uword songs;
	//
	// Available after song initialization.
	//
	uword irqAddr;              // if (playAddr == 0), interrupt handler has been
	                            // installed and starts calling the C64 player
	                            // at this address
	uword currentSong;          // the one that has been initialized
	ubyte songSpeed;            // initialized playing speed
	ubyte clockSpeed;           // initialized clock chip speed
	bool musPlayer;             // whether Sidplayer routine has been installed
    bool psidSpecific;          // whether PlaySID specific extensions are used
    ubyte clock;                // required clock chip speed (PAL, NTSC)
    ubyte sidModel;             // required SID chip model
    bool fixLoad;               // whether load address might be duplicate
	uword lengthInSeconds;      // --- reserved ---
    
    ubyte relocStartPage;       // first available page for relocation
    ubyte relocPages;           // number of pages available for relocation
    
    uword reserved;             // for copying (remainder of) PSID "reserved" field
	//
	// Song title, credits, ...
	//
	ubyte numberOfInfoStrings;  // the number of available text info lines
	char* infoString[infoStringNum];
	char* nameString;           // name, author and copyright strings
	char* authorString;         // are duplicates of infoString[?]
	char* copyrightString;
	//
	uword numberOfCommentStrings;  // --- not yet supported ---
	char ** commentString;         // -"-
	//
	udword dataFileLen;         // length of single-file sidtune file
	udword c64dataLen;          // length of raw C64 data without load address
        char* path;                 // path to sidtune files; 0, if cwd
	char* dataFileName;         // a first file: e.g. ``*.c64''
	char* infoFileName;         // a second file: e.g. ``*.sid''
	//
	const char* statusString;   // error/status message of last operation
};


class emuEngine;


class sidTune
{

 public:  // ----------------------------------------------------------------

	// --- constructors ---

	// If your opendir() and readdir()->d_name return path names
    // that contain the forward slash (/) as file separator, but
    // your operating system uses a different character, there are
    // extra functions that can deal with this special case. Set
    // separatorIsSlash to true if you like path names to be split
    // correctly.
    // You don't need these extra functions if your systems file
    // separator is the forward slash.
    //
	// Load a sidtune from a file.
    //
	// To retrieve data from standard input pass in filename "-".
	// If you want to override the default filename extensions use this
	// contructor. Please note, that if the specified ``sidTuneFileName''
	// does exist and the loader is able to determine its file format,
	// this function does not try to append any file name extension.
	// See ``sidtune.cpp'' for the default list of file name extensions.
	// You can specific ``sidTuneFileName = 0'', if you do not want to
	// load a sidtune. You can later load one with open().
	sidTune(const char* sidTuneFileName, const char **fileNameExt = 0);
	sidTune(const char* sidTuneFileName, const bool separatorIsSlash,
            const char **fileNameExt = 0);
    
	// Load a single-file sidtune from a memory buffer.
	// Currently supported: PSID format
	sidTune(const ubyte* oneFileFormatSidtune, udword sidtuneLength);

	virtual ~sidTune();  // destructor

	// --- member functions ---

	// The sidTune class does not copy the list of file name extensions,
	// so make sure you keep it. If the provided pointer is 0, the
	// default list will be activated.
	void setFileNameExtensions(const char **fileNameExt);
	
	// Load a sidtune into an existing object.
	// From a file.
	bool open(const char* sidTuneFileName);
	bool open(const char* sidTuneFileName, const bool separatorIsSlash);
    
	// From a buffer.
	bool load(const ubyte* oneFileFormatSidtune, udword sidtuneLength);

	bool getInfo( struct sidTuneInfo& );
	virtual bool setInfo( struct sidTuneInfo& );  // dummy

	ubyte getSongSpeed()  { return info.songSpeed; }
	uword getInitAddr() { return info.initAddr; }
	uword getPlayAddr() { return info.playAddr; }

	// This function initializes the SID emulator engine to play the given
	// sidtune song.
	friend bool sidEmuInitializeSong(emuEngine &, sidTune &, uword songNum);

	// This is an old non-obsolete sub-function, that does not scan the sidtune
	// for digis. If (emuConfig.digiPlayerScan == 0), this functions does the
	// same as the one above.
	friend bool sidEmuInitializeSongOld(emuEngine &, sidTune &, uword songNum);

	// Determine current state of object (true = okay, false = error).
	// Upon error condition use ``getInfo'' to get a descriptive
	// text string in ``sidTuneInfo.statusString''.
	operator bool()  { return status; }
	bool getStatus()  { return status; }

	// --- file save & format conversion ---

	// These functions work for any successfully created object.
	// overWriteFlag: true  = Overwrite existing file.
	//                false = Default, return error when file already
	//                        exists.
	// One could imagine an "Are you sure ?"-checkbox before overwriting
	// any file.
	// returns: true = Successful, false = Error condition.
	bool saveC64dataFile( const char* destFileName, bool overWriteFlag = false );
	bool saveSIDfile( const char* destFileName, bool overWriteFlag = false );
	bool savePSIDfile( const char* destFileName, bool overWriteFlag = false );

	// This function can be used to remove a duplicate C64 load address in
	// the C64 data (example: FE 0F 00 10 4C ...). A duplicate load address
	// of offset 0x02 is indicated by the ``fixLoad'' flag in the sidTuneInfo
	// structure.
	//
	// The ``force'' flag here can be used to remove the first load address
	// and set new INIT/PLAY addresses regardless of whether a duplicate
	// load address has been detected and indicated by ``fixLoad''.
	// For instance, some position independent sidtunes contain a load address
	// of 0xE000, but are loaded to 0x0FFE and call the player code at 0x1000.
	//
	// Don't forget to save the sidtune file.
	void fixLoadAddress(bool force = false, uword initAddr = 0, uword playAddr = 0);
	
	ubyte* cachePtr;
	udword fileOffset;  // for files with header: offset to real data

 protected:  // -------------------------------------------------------------

	bool status;
	sidTuneInfo info;

	ubyte songSpeed[classMaxSongs];
	ubyte clockSpeed[classMaxSongs];  // not fully used by file formats
	uword songLength[classMaxSongs];  // reserved

	// holds text info from the format headers etc.
	char infoString[infoStringNum][infoStringLen];

	bool isCached;
	
	udword cacheLen;

    bool isSlashedFileName;

	// Using external buffers for loading files instead of the interpreter
	// memory. This separates the sidtune objects from the interpreter.
	ubyte* fileBuf;
	ubyte* fileBuf2;


	// Filename extensions to append for various file types.
	const char **fileNameExtensions;

	// --- protected member functions ---

	// Convert 32-bit PSID-style speed word to internal tables.
	void convertOldStyleSpeedToTables(udword oldStyleSpeed);

	// Copy C64 data from internal cache to C64 memory.
	bool placeSidTuneInC64mem( ubyte* c64buf );

	udword loadFile(const char* fileName, ubyte** bufferRef);
	bool saveToOpenFile( ofstream& toFile, const ubyte* buffer, udword bufLen );

    bool fileExists(const char* fileName);
   
	// Data caching.
	bool cacheRawData(const void* sourceBuffer, udword sourceBufLen);
	bool getCachedRawData(void* destBuffer, udword destBufLen);

	// Support for various file formats.

	virtual bool PSID_fileSupport(const void* buffer, udword bufLen);
	virtual bool PSID_fileSupportSave(ofstream& toFile, const ubyte* dataBuffer);

        virtual bool MUS_fileSupport(const void* buffer, udword bufLen);
        virtual void MUS_installPlayer(ubyte *c64buf);

	virtual bool INFO_fileSupport(const void* dataBuffer, udword dataBufLen,
                                      const void* infoBuffer, udword infoBufLen);

	virtual bool SID_fileSupport(const void* dataBuffer, udword dataBufLen,
                                     const void* sidBuffer, udword sidBufLen);
	virtual bool SID_fileSupportSave(ofstream& toFile);

	
 private:  // ---------------------------------------------------------------
	
	void safeConstructor();
	void safeDestructor();
#if !defined(SID_NO_STDIN_LOADER)
	void stdinConstructor();
#endif
	void filesConstructor(const char* name);
        void bufferConstructor(const ubyte* data, udword dataLen);
	
	uword selectSong(uword selectedSong);
	void setIRQaddress(uword address);
	
        void clearCache();

	void deleteFileBuffers();
        void deleteFileNameCopies();
	// Try to retrieve single-file sidtune from specified buffer.
	bool getSidtuneFromFileBuffer(const ubyte* buffer, udword bufferLen);
	// Cache the data of a single-file or two-file sidtune and its
	// corresponding file names.
	void acceptSidTune(const char* dataFileName, const char* infoFileName,
                           const ubyte* dataFileBuf, udword dataLen );
	bool createNewFileName(char** destStringPtr,
                               const char* sourceName, const char* sourceExt);
};
	

#endif  /* SIDPLAY1_SIDTUNE_H */
