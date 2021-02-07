//
// /home/ms/source/sidplay/libsidplay/RCS/sidtune.cpp,v
//
// Information on usage of this class in "include/sidtune.h".
//

#include "compconf.h"
#ifdef SID_HAVE_EXCEPTIONS
#include <new>
#endif
#include <iostream>
#include <iomanip>
#include <fstream>
using namespace std;
#include <cstring>
#include <climits>

#undef SID_USE_STAT
#ifdef SID_HAVE_UNISTD_H
#ifdef SID_HAVE_SYS_TYPES_H
#ifdef SID_HAVE_SYS_STAT_H
#define SID_USE_STAT
# include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#endif
#endif
#ifndef SID_USE_STAT
#include <stdio.h>
#endif

#include "sidtune.h"
#include "fformat.h"
#include "myendian.h"
#include "pp.h"


const char text_songNumberExceed[] = "WARNING: Selected song number was too high";
const char text_emptyFile[] = "ERROR: File is empty";
const char text_unrecognizedFormat[] = "ERROR: Could not determine file format";
const char text_noDataFile[] = "ERROR: Did not find the corresponding data file";
const char text_notEnoughMemory[] = "ERROR: Not enough free memory";
const char text_cantLoadFile[] = "ERROR: Could not load input file";
const char text_cantOpenFile[] = "ERROR: Could not open file for binary input";
const char text_fileTooLong[] = "ERROR: Input data too long";
const char text_dataTooLong[] = "ERROR: Music data size exceeds C64 memory";
const char text_cantCreateFile[] = "ERROR: Could not create output file";
const char text_fileIoError[] = "ERROR: File I/O error";
const char text_fatalInternal[] = "FATAL: Internal error - contact the developers";
const char text_VBI[] = "VBI";
const char text_CIA[] = "CIA 1 Timer A";
const char text_PAL_VBI[] = "50 Hz VBI (PAL)";
const char text_PAL_CIA[] = "CIA 1 Timer A (PAL)";
const char text_NTSC_VBI[] = "60 Hz VBI (NTSC)";
const char text_NTSC_CIA[] = "CIA 1 Timer A (NTSC)";
const char text_noErrors[] = "No errors";
const char text_na[] = "N/A";

// Default sidtune file name extensions. This selection can be overriden
// by specifying a custom list in the constructor.
const char *defaultFileNameExtSid1[] =
{
	// Preferred default file extension for single-file sidtunes
	// or sidtune description files in SIDPLAY INFOFILE format.
	".sid",
	// Common file extension for single-file sidtunes due to SIDPLAY/DOS
	// displaying files *.DAT in its file selector by default.
	// Originally this was intended to be the extension of the raw data file
	// of two-file sidtunes in SIDPLAY INFOFILE format.
	".dat",
	// Extension of Amiga Workbench tooltype icon info files, which
	// have been cut to MS-DOS file name length (8.3).
	".inf",
	// No extension for the raw data file of two-file sidtunes in
	// PlaySID Amiga Workbench tooltype icon info format.
	"",
	// Common upper-case file extensions from MS-DOS (unconverted).
	".DAT", ".SID", ".INF",
	// File extensions used (and created) by various C64 emulators and
	// related utilities. These extensions are recommended to be used as
	// a replacement for ".dat" in conjunction with two-file sidtunes.
	".c64", ".prg", ".C64", ".PRG",
	// Uncut extensions from Amiga.
	".info", ".INFO", ".data", ".DATA",
	// End.
	0
};

// ------------------------------------------------- constructors, destructor

sidTune::sidTune( const char* fileName, const char **fileNameExt )
{
	safeConstructor();
	isSlashedFileName = false;
	setFileNameExtensions(fileNameExt);
	if (fileName != 0)
	{
#if !defined(SID_NO_STDIN_LOADER)
		// Filename ``-'' is used as a synonym for standard input.
		if ( strcmp( fileName, "-" ) == 0 )
		{
			stdinConstructor();
		}
		else
		{
#endif
			filesConstructor( fileName );
#if !defined(SID_NO_STDIN_LOADER)
		}
#endif
		deleteFileBuffers();
	}
}

sidTune::sidTune(const char* fileName, const bool separatorIsSlash,
                 const char **fileNameExt)
{
	safeConstructor();
	isSlashedFileName = separatorIsSlash;
	setFileNameExtensions(fileNameExt);
	if (fileName != 0)
	{
#if !defined(SID_NO_STDIN_LOADER)
		// Filename ``-'' is used as a synonym for standard input.
		if ( strcmp( fileName, "-" ) == 0 )
		{
			stdinConstructor();
		}
		else
		{
#endif
			filesConstructor( fileName );
			deleteFileBuffers();
#if !defined(SID_NO_STDIN_LOADER)
		}
#endif
	}
}

sidTune::sidTune(const ubyte* data, udword dataLen)
{
	safeConstructor();
	bufferConstructor(data,dataLen);
}


sidTune::~sidTune()
{
	safeDestructor();
}


// -------------------------------------------------- public member functions

void sidTune::setFileNameExtensions(const char **fileNameExt)
{
	if (fileNameExt != 0)
		fileNameExtensions = fileNameExt;
	else
		fileNameExtensions = defaultFileNameExtSid1;
}

bool sidTune::load(const ubyte* data, udword dataLen)
{
	safeDestructor();
	safeConstructor();
	bufferConstructor(data,dataLen);
	return status;
}

bool sidTune::open(const char* fileName)
{
	safeDestructor();
	safeConstructor();
    isSlashedFileName = false;
	filesConstructor(fileName);
	deleteFileBuffers();
	return status;
}

bool sidTune::open(const char* fileName, const bool separatorIsSlash)
{
	safeDestructor();
	safeConstructor();
	isSlashedFileName = separatorIsSlash;
	filesConstructor(fileName);
	deleteFileBuffers();
	return status;
}

bool sidTune::setInfo( sidTuneInfo & inInfo )
{
    // dummy
    return true;
}

bool sidTune::getInfo( sidTuneInfo& outInfo )
{
	outInfo = info;
	return true;
}

// First check, whether a song is valid. Then copy any song-specific
// variable information such a speed/clock setting to the info structure.
//
// This is a private member function. It is used only by player.cpp.
uword sidTune::selectSong(uword selectedSong)
{
	// Determine and set starting song number.
	if (selectedSong == 0)
	{
		selectedSong = info.startSong;
	}
	else if ((selectedSong > info.songs) || (selectedSong > classMaxSongs))
	{
		info.statusString = text_songNumberExceed;
		selectedSong = info.startSong;
	}
	info.lengthInSeconds = songLength[selectedSong-1];
	// Retrieve song speed definition.
	info.songSpeed = songSpeed[selectedSong-1];
    // NB! Prior to song initialization, the following is the same
    // than info.clock.
	info.clockSpeed = clockSpeed[selectedSong-1];
	// Assign song speed description string depending on clock speed.
    // Final speed description is available only after song init.
    if (info.songSpeed == SIDTUNE_SPEED_VBI)
    {
        info.speedString = text_VBI;
    }
    else
    {
        info.speedString = text_CIA;
    }
	return (info.currentSong=selectedSong);
}

void sidTune::fixLoadAddress(bool force, uword init, uword play)
{
    if (info.fixLoad || force)
    {
        info.fixLoad = false;
        info.loadAddr += 2;
        fileOffset += 2;

        if (force)
        {
            info.initAddr = init;
            info.playAddr = play;
        }
    }
}

// --------------------------------------------------------------------------

void sidTune::setIRQaddress(uword address)
{
	info.irqAddr = address;
}

bool sidTune::placeSidTuneInC64mem( ubyte* c64buf )
{
	if (isCached && status)
	{
		// Check the size of the data.
		if ( info.c64dataLen > 65536 )
		{
			info.statusString = text_dataTooLong;
			return (status = false);
		}
		else
		{
			udword endPos = info.loadAddr + info.c64dataLen;
			if (endPos <= 65536)
			{
				// Copy data from cache to the correct destination.
				memcpy(c64buf+info.loadAddr,cachePtr+fileOffset,info.c64dataLen);
			}
			else
			{
				// Security - split data which would exceed the end of the C64 memory.
				// Memcpy could not detect this.
				memcpy(c64buf+info.loadAddr,cachePtr+fileOffset,info.c64dataLen-(endPos-65536));
				// Wrap the remaining data to the start address of the C64 memory.
				memcpy(c64buf,cachePtr+fileOffset+info.c64dataLen-(endPos-65536),(endPos-65536));
			}
			return (status = true);
		}
	}
	else
	{
		return (status = false);
	}
}


udword sidTune::loadFile(const char* fileName, ubyte** bufferRef)
{
	udword fileLen = 0;
	status = false;
    // Work-around for bugs in Standard library.
    if ( !fileExists( fileName )  )
    {
		info.statusString = text_cantOpenFile;
        return 0;
    }
    // Open binary input file stream at end of file.
#if defined(SID_HAVE_IOS_BIN)
	ifstream myIn( fileName, ios::in|ios::bin|ios::ate );
#else
	ifstream myIn( fileName, ios::in|ios::binary|ios::ate );
#endif
	// As a replacement for !is_open(), bad() and the NOT-operator
    // don't seem to work on all systems.
#if defined(SID_DONT_HAVE_IS_OPEN)
    if ( !myIn )
#else
	if ( !myIn.is_open() )
#endif
	{
		info.statusString = text_cantOpenFile;
        return 0;
	}
    // Check for PowerPacker compression: load and decompress, if PP20 file.
    if (depp(myIn,bufferRef))
    {
        // Decompression successful, use uncompressed datafilelen.
        fileLen = ppUncompressedLen();
        info.statusString = ppErrorString;
        status = true;
        return fileLen;
    }
    else if (ppIsCompressed())
    {
        // An error occured while decompressing.
        info.statusString = ppErrorString;
        return 0;
    }
    // Should be uncompressed file.
#if defined(SID_HAVE_SEEKG_OFFSET)
    fileLen = (myIn.seekg(0,ios::end)).offset();
#else
    myIn.seekg(0, ios::end);
    fileLen = (udword)myIn.tellg();
#endif
    if ( *bufferRef != 0 )
    {
        delete[] *bufferRef;  // free previously allocated memory
    }
#ifdef SID_HAVE_EXCEPTIONS
    *bufferRef = new(std::nothrow) ubyte[fileLen+1];
#else
    *bufferRef = new ubyte[fileLen+1];
#endif
    if ( *bufferRef == 0 )
    {
        info.statusString = text_notEnoughMemory;
        fileLen = 0;  // returning 0 = error condition.
    }
    else
    {
        *(*bufferRef+fileLen) = 0;
    }
    // Load uncompressed file.
    myIn.seekg(0, ios::beg);
    udword restFileLen = fileLen;
    while ( restFileLen > INT_MAX )
    {
        myIn.read( (char*)*bufferRef + (fileLen - restFileLen), INT_MAX );
        restFileLen -= INT_MAX;
    }
    if ( restFileLen > 0 )
    {
        myIn.read( (char*)*bufferRef + (fileLen - restFileLen), restFileLen );
    }
    if ( myIn.bad() )
    {
        info.statusString = text_cantLoadFile;
    }
    else
    {
        info.statusString = text_noErrors;
        status = true;
    }
    myIn.close();
    if (fileLen == 0)
    {
        info.statusString = text_emptyFile;
        status = false;
    }
	return fileLen;
}


// protected
bool sidTune::fileExists( const char* fileName )
{
#ifdef SID_USE_STAT
    struct stat fileStat;
    return ( stat( fileName, &fileStat ) == 0  &&
         S_ISREG( fileStat.st_mode )  );
#else
    FILE* f = fopen( fileName, "r+b" );
    bool exists = (f!=NULL);
    if ( exists )
        fclose( f );
    return exists;
#endif
}
    

void sidTune::deleteFileBuffers()
{
	// This function does not affect status and statusstring.
	// The filebuffers are global to the object.
	if ( fileBuf != 0 )
	{
		delete[] fileBuf;
		fileBuf = 0;
	}
	if ( fileBuf2 != 0 )
	{
		delete[] fileBuf2;
		fileBuf2 = 0;
	}
}

void sidTune::deleteFileNameCopies()
{
	// When will it be fully safe to call delete[] (0)?
	if ( info.dataFileName != 0 )
		delete[] info.dataFileName;
	if ( info.infoFileName != 0 )
		delete[] info.infoFileName;
	if ( info.path != 0 )
		delete[] info.path;
	info.dataFileName = 0;
	info.infoFileName = 0;
	info.path = 0;
}


bool sidTune::cacheRawData(const void* sourceBuf, udword sourceBufLen)
{
	clearCache();
#ifdef SID_HAVE_EXCEPTIONS
	if ( (cachePtr = new(std::nothrow) ubyte[sourceBufLen]) == 0 )
#else
	if ( (cachePtr = new ubyte[sourceBufLen]) == 0 )
#endif
	{
		info.statusString = text_notEnoughMemory;
		return (status = false);
	}
	else
        {
            if (sourceBufLen >= 2)
            {
                // We only detect an offset of two. Some position independent
                // sidtunes contain a load address of 0xE000, but are loaded
                // to 0x0FFE and call player at 0x1000.
                info.fixLoad = (readLEword((const ubyte*)sourceBuf+fileOffset)==(info.loadAddr+2));
            }
		memcpy(cachePtr, (const ubyte*)sourceBuf, sourceBufLen);
		cacheLen = sourceBufLen;
		isCached = true;
		info.statusString = text_noErrors;
		return (status = true);
	}
}

void sidTune::clearCache()
{
	if ( cachePtr != 0 )
	{
		delete[] cachePtr;
		cachePtr = 0;
	}
	cacheLen = 0;
	isCached = false;
}

bool sidTune::getCachedRawData( void* destBuf, udword destBufLen )
{
	if (( cachePtr == 0 ) || ( cacheLen > destBufLen ))
	{
		info.statusString = text_fatalInternal;
		return (status = false);
	}
	memcpy( (ubyte*)destBuf, cachePtr, cacheLen );
	info.dataFileLen = cacheLen;
	info.statusString = text_noErrors;
	return (status = true);
}


void sidTune::safeConstructor()
{
	// Initialize the object with some safe defaults.
	status = false;

	info.statusString = text_na;
	info.path = info.infoFileName = info.dataFileName = 0;
	info.dataFileLen = info.c64dataLen = 0;
	info.formatString = text_na;
	info.speedString = text_na;
	info.loadAddr = ( info.initAddr = ( info.playAddr = 0 ));
	info.songs = ( info.startSong = ( info.currentSong = 0 ));
	info.musPlayer = false;
	info.psidSpecific = false;
	info.fixLoad = false;
    info.clock = SIDTUNE_CLOCK_UNKNOWN;
    info.sidModel = SIDTUNE_SIDMODEL_UNKNOWN;
	info.songSpeed = SIDTUNE_SPEED_VBI;
	info.clockSpeed = SIDTUNE_CLOCK_UNKNOWN;
	info.lengthInSeconds = 0;
    info.relocStartPage = 0;
    info.relocPages = 0;
    info.reserved = 0;
	
	for ( uint si = 0; si < classMaxSongs; si++ )
	{
		songSpeed[si] = SIDTUNE_SPEED_VBI;
		clockSpeed[si] = SIDTUNE_CLOCK_UNKNOWN;
		songLength[si] = 0;
	}

	cachePtr = 0;
	cacheLen = 0;

	fileBuf = ( fileBuf2 = 0 );
	fileOffset = 0;
	fileNameExtensions = defaultFileNameExtSid1;

	for ( uint sNum = 0; sNum < infoStringNum; sNum++ )
	{
		for ( uint sPos = 0; sPos < infoStringLen; sPos++ )
		{
			infoString[sNum][sPos] = 0;
		}
	}
	info.numberOfInfoStrings = 0;

	// Not used!!!
	info.numberOfCommentStrings = 1;
#ifdef SID_HAVE_EXCEPTIONS
	info.commentString = new(std::nothrow) char* [info.numberOfCommentStrings];
#else
	info.commentString = new char* [info.numberOfCommentStrings];
#endif
    if (info.commentString != 0)
        info.commentString[0] = myStrDup("--- SAVED WITH SIDPLAY ---");
    else
        info.commentString[0] = 0;
}


void sidTune::safeDestructor()
{
	// Remove copy of comment field.
	udword strNum = 0;
	// Check and remove every available line.
	while (info.numberOfCommentStrings-- > 0)
	{
		if (info.commentString[strNum] != 0)
		{
			delete[] info.commentString[strNum];
			info.commentString[strNum] = 0;
		}
		strNum++;  // next string
	};
	delete[] info.commentString;  // free the array pointer

	clearCache();
	deleteFileNameCopies();
	deleteFileBuffers();

	status = false;
}


#if !defined(SID_NO_STDIN_LOADER)

void sidTune::stdinConstructor()
{
	// Assume a failure, so we can simply return.
	status = false;
	// Assume the memory allocation to fail.
	info.statusString = text_notEnoughMemory;
#ifdef SID_HAVE_EXCEPTIONS
	if (( fileBuf = new(std::nothrow) ubyte[maxSidtuneFileLen] ) == 0 )
#else
	if (( fileBuf = new ubyte[maxSidtuneFileLen] ) == 0 )
#endif
    {
		return;
    }
	udword i = 0;
	char datb;
	// We only read as much as fits in the buffer.
	// This way we avoid choking on huge data.
	while (cin.get(datb) && i<maxSidtuneFileLen)
		fileBuf[i++] = datb;
	info.dataFileLen = i;
    getSidtuneFromFileBuffer(fileBuf,info.dataFileLen);
}

#endif


void sidTune::bufferConstructor(const ubyte* data, udword dataLen)
{
	// Assume a failure, so we can simply return.
	status = false;
	if (data != 0)
	{
		if (dataLen > maxSidtuneFileLen)
		{
			info.statusString = text_fileTooLong;
		}
		else
		{
			info.dataFileLen = dataLen;
			getSidtuneFromFileBuffer(data, dataLen);
		}
	}
}

bool sidTune::getSidtuneFromFileBuffer(const ubyte* buffer, udword bufferLen)
{
	bool foundFormat = false;
	// Here test for the possible single file formats. ------------------
	if ( PSID_fileSupport( buffer, bufferLen ))
	{
		foundFormat = true;
	}
	else if ( MUS_fileSupport( buffer, bufferLen ))
	{
		foundFormat = true;
	}
	else
	{
		// No further single-file-formats available. --------------------
		info.formatString = text_na;
		info.statusString = text_unrecognizedFormat;
		status = false;
	}
	if ( foundFormat )
	{
		status = true;
		info.statusString = text_noErrors;
		acceptSidTune("-","-",buffer,bufferLen);
	}
	return foundFormat;
}


void sidTune::acceptSidTune(const char* dataFileName, const char* infoFileName,
                            const ubyte* dataBuf, udword dataLen )
{
	deleteFileNameCopies();
	// Make a copy of the data file name and path, if available.
	if ( dataFileName != 0 )
	{
		info.path = myStrDup(dataFileName);
		if (isSlashedFileName)
		{
			info.dataFileName = myStrDup(slashedFileNameWithoutPath(info.path));
			*slashedFileNameWithoutPath(info.path) = 0;  // path only
		}
		else
		{
            info.dataFileName = myStrDup(fileNameWithoutPath(info.path));
            *fileNameWithoutPath(info.path) = 0;  // path only
        }
		if ((info.path==0) || (info.dataFileName==0))
		{
			info.statusString = text_notEnoughMemory;
			status = false;
			return;
		}
	}
	// Make a copy of the info file name, if available.
	if ( infoFileName != 0 )
	{
		char* tmp = myStrDup(infoFileName);
		if (isSlashedFileName)
			info.infoFileName = myStrDup(slashedFileNameWithoutPath(tmp));
		else
			info.infoFileName = myStrDup(fileNameWithoutPath(tmp));
		if ((tmp==0) || (info.infoFileName==0))
		{
			info.statusString = text_notEnoughMemory;
			status = false;
			return;
		}
		delete[] tmp;
	}
	// Fix bad sidtune set up.
	if (info.songs > classMaxSongs)
		info.songs = classMaxSongs;
	else if (info.songs == 0)
		info.songs++;
	if (info.startSong > info.songs)
		info.startSong = 1;
	else if (info.startSong == 0)
		info.startSong++;
	info.dataFileLen = dataLen;
	info.c64dataLen = dataLen - fileOffset;
	cacheRawData( dataBuf, dataLen );
}


bool sidTune::createNewFileName( char** destStringPtr,
                                 const char* sourceName,
                                 const char* sourceExt)
{
	// Free any previously allocated object.
	if ( *destStringPtr != 0 )
	{
		delete[] *destStringPtr;
	}
	// Get enough memory, so we can appended the extension.
#ifdef SID_HAVE_EXCEPTIONS
	*destStringPtr = new(std::nothrow) char[strlen(sourceName) + strlen(sourceExt) +1];
#else
	*destStringPtr = new char[strlen(sourceName) + strlen(sourceExt) +1];
#endif
	if ( *destStringPtr == 0 )
	{
		info.statusString = text_notEnoughMemory;
		return (status = false);
	}
	strcpy( *destStringPtr, sourceName );
	char* extPtr = fileExtOfPath(*destStringPtr);
	strcpy( extPtr, sourceExt );
	return true;
}


// Initializing the object based upon what we find in the specified file.

void sidTune::filesConstructor( const char* fileName )
{
	fileBuf = 0;  // will later point to the buffered file
	// Try to load the single specified file.
	if (( info.dataFileLen = loadFile(fileName,&fileBuf)) != 0 )
	{
		// File loaded. Now check if it is in a valid single-file-format.
		if ( PSID_fileSupport(fileBuf,info.dataFileLen ))
		{
			acceptSidTune(fileName,0,fileBuf,info.dataFileLen);
			return;
		}
		else if ( MUS_fileSupport(fileBuf,info.dataFileLen) )
		{
			acceptSidTune(fileName,0,fileBuf,info.dataFileLen);
			return;
		}


// -------------------------------------- Support for multiple-files formats.
		else
		{
// We cannot simply try to load additional files, if a description file was
// specified. It would work, but is error-prone. Imagine a filename mismatch
// or more than one description file (in another) format. Any other file
// with an appropriate file name can be the C64 data file.
// First we see if ``fileName'' could be a raw data file. In that case we
// have to find the corresponding description file.

			// Right now we do not have a second file. Hence the (0, 0, ...)
			// parameters are set for the data buffer. This will not hurt the
			// file support procedures.
			udword fileLen2;
			char* fileName2 = 0;

			// Make sure that ``fileBuf'' does not contain a description file.
			if ( !SID_fileSupport(0,0,fileBuf,info.dataFileLen) &&
				!INFO_fileSupport(0,0,fileBuf,info.dataFileLen) )
			{
				// Assuming ``fileName'' to hold the name of the raw data file,
				// we now create the name of a description file (=fileName2) by
				// appending various filename extensions.

// ------------------------------------------ Looking for a description file.

				const char **tmpFileNameExt = fileNameExtensions;
				while (*tmpFileNameExt != 0)
				{
					if ( !createNewFileName(&fileName2,fileName,*tmpFileNameExt) )
						return;
					// Do not load the first file again if names are equal.
					if ( myStrCaseCmp(fileName,fileName2) != 0 )
					{
						// 1st data file was loaded into ``fileBuf'',
						// so we load the 2nd one into ``fileBuf2''.
						if (( fileLen2 = loadFile(fileName2,&fileBuf2)) != 0 )
						{
							if ( SID_fileSupport(fileBuf,info.dataFileLen,fileBuf2,fileLen2) )
							{
								acceptSidTune(fileName,fileName2,fileBuf,info.dataFileLen);
								delete[] fileName2;
								return;
							}
							else if ( INFO_fileSupport(fileBuf,info.dataFileLen,fileBuf2,fileLen2) )
							{
								acceptSidTune(fileName,fileName2,fileBuf,info.dataFileLen);
								delete[] fileName2;
								return;
							}
						}
					}
					tmpFileNameExt++;
				};

// --------------------------------------- Could not find a description file.

				delete[] fileName2;
				info.formatString = text_na;
				info.statusString = text_unrecognizedFormat;
				status = false;
				return;
			}


// -------------------------------------------------------------------------
// Still unsuccessful ? Probably one put a description file name into
// ``fileName''. Assuming ``fileName'' to hold the name of a description
// file, we now create the name of the data file and swap both used memory
// buffers - fileBuf and fileBuf2 - when calling the format support.
// If it works, the second file is the data file ! If it is not, but does
// exist, we are out of luck, since we cannot detect data files.

			// Make sure ``fileBuf'' contains a description file.
			else if ( SID_fileSupport(0,0,fileBuf,info.dataFileLen) ||
					 INFO_fileSupport(0,0,fileBuf,info.dataFileLen))
			{

// --------------------- Description file found. --- Looking for a data file.

				const char **tmpFileNameExt = fileNameExtensions;
				while (*tmpFileNameExt != 0)
				{
					if ( !createNewFileName(&fileName2,fileName,*tmpFileNameExt) )
						return;
					// Do not load the first file again if names are equal.
					if ( myStrCaseCmp(fileName,fileName2) != 0 )
					{
						// 1st info file was loaded into ``fileBuf'',
						// so we load the 2nd one into ``fileBuf2''.
						if (( fileLen2 = loadFile(fileName2,&fileBuf2)) != 0 )
						{
// -------------- Some data file found, now identifying the description file.

							if ( SID_fileSupport(fileBuf2,fileLen2,fileBuf,info.dataFileLen) )
							{
								acceptSidTune(fileName2,fileName,fileBuf2,fileLen2);
								delete[] fileName2;
								return;
							}
							else if ( INFO_fileSupport(fileBuf2,fileLen2,fileBuf,info.dataFileLen) )
							{
								acceptSidTune(fileName2,fileName,fileBuf2,fileLen2);
								delete[] fileName2;
								return;
							}
						}
					}
					tmpFileNameExt++;
				};
				
// ---------------------------------------- No corresponding data file found.

				delete[] fileName2;
				info.formatString = text_na;
				info.statusString = text_noDataFile;
				status = false;
				return;
			} // end else if ( = is description file )

// --------------------------------- Neither description nor data file found.

			else
			{
				info.formatString = text_na;
				info.statusString = text_unrecognizedFormat;
				status = false;
				return;
			}
		} // end else ( = is no singlefile )

// ---------------------------------------------------------- File I/O error.

	} // if loaddatafile
	else
	{
		// returned fileLen was 0 = error. The info.statusString is
		// already set then.
		info.formatString = text_na;
		status = false;
		return;
	}
} 


void sidTune::convertOldStyleSpeedToTables(udword oldStyleSpeed)
{
	// Create the speed/clock setting tables.
	//
	// This does not take into account the PlaySID bug upon evaluating the
	// SPEED field. It would most likely break compatibility to lots of
	// sidtunes, which have been converted from .SID format and vice versa.
	// The .SID format does the bit-wise/song-wise evaluation of the SPEED
	// value correctly, like it is described in the PlaySID documentation.

	int toDo = ((info.songs <= classMaxSongs) ? info.songs : classMaxSongs);
	for (int s = 0; s < toDo; s++)
	{
		if (( (oldStyleSpeed>>(s&31)) & 1 ) == 0 )
		{
			songSpeed[s] = SIDTUNE_SPEED_VBI;
		}
		else
		{
			songSpeed[s] = SIDTUNE_SPEED_CIA_1A;
		}
        clockSpeed[s] = info.clock;
	}
}


//
// File format conversion ---------------------------------------------------
//
				
bool sidTune::saveToOpenFile( ofstream& toFile, const ubyte* buffer, udword bufLen )
{
	udword lenToWrite = bufLen;
	while ( lenToWrite > INT_MAX )
	{
		toFile.write( (const char*)buffer + (bufLen - lenToWrite), INT_MAX );
		lenToWrite -= INT_MAX;
	}
	if ( lenToWrite > 0 )
		toFile.write( (const char*)buffer + (bufLen - lenToWrite), lenToWrite );
	if ( toFile.bad() )
	{
		info.statusString = text_fileIoError;
		return false;
	}
	else
	{
		info.statusString = text_noErrors;
		return true;
	}
}
		

bool sidTune::saveC64dataFile( const char* fileName, bool overWriteFlag )
{
	bool success = false;  // assume error
	// This prevents saving from a bad object.
	if ( status )
	{
		ofstream fMyOut;
        if ( !overWriteFlag && fileExists( fileName ) )
        {
			info.statusString = text_cantCreateFile;
            return success;
        }
		// Open binary output file stream.
        else
#if defined(SID_HAVE_IOS_BIN)
            fMyOut.open( fileName, ios::out|ios::bin|ios::trunc );
#else
            fMyOut.open( fileName, ios::out|ios::binary|ios::trunc );
#endif
		if ( !fMyOut )
		{ 
			info.statusString = text_cantCreateFile;
		}
		else
		{  
			// Save c64 lo/hi load address.
			ubyte saveAddr[2];
			saveAddr[0] = info.loadAddr & 255;
			saveAddr[1] = info.loadAddr >> 8;
			fMyOut.write( (const char*)saveAddr, 2 );
			// Data starts at: bufferaddr + fileOffset
			// Data length: info.dataFileLen - fileOffset
			if ( !saveToOpenFile( fMyOut, cachePtr + fileOffset, info.dataFileLen - fileOffset ) )
			{
				info.statusString = text_fileIoError;
			}
			else
			{
				info.statusString = text_noErrors;
				success = true;
			}
			fMyOut.close();
		}
	}
	return success;
}


bool sidTune::saveSIDfile( const char* fileName, bool overWriteFlag )
{
	bool success = false;  // assume error
	// This prevents saving from a bad object.
	if ( status )
	{
		ofstream fMyOut;
        if ( !overWriteFlag && fileExists( fileName ) )
        {
			info.statusString = text_cantCreateFile;
            return success;
        }
		// Open ASCII output file stream.
        else
#if defined(SID_HAVE_IOS_BIN)
            fMyOut.open( fileName, ios::out|ios::trunc );
#else
            fMyOut.open( fileName, ios::out|ios::trunc );
#endif
		if ( !fMyOut )
		{ 
			info.statusString = text_cantCreateFile;
		}
		else
		{  
			if ( !SID_fileSupportSave( fMyOut ) )
			{
				info.statusString = text_fileIoError;
			}
			else
			{
				info.statusString = text_noErrors;
				success = true;
			}
			fMyOut.close();
		}
	}
	return success;
}
		

bool sidTune::savePSIDfile( const char* fileName, bool overWriteFlag )
{
	bool success = false;  // assume error
	// This prevents saving from a bad object.
	if ( status )
	{
		ofstream fMyOut;
        if ( !overWriteFlag && fileExists( fileName ) )
        {
			info.statusString = text_cantCreateFile;
            return success;
        }
		// Open binary output file stream.
        else
#if defined(SID_HAVE_IOS_BIN)
            fMyOut.open( fileName, ios::out|ios::bin|ios::trunc );
#else
            fMyOut.open( fileName, ios::out|ios::binary|ios::trunc );
#endif
		if ( !fMyOut )
		{
			info.statusString = text_cantCreateFile;
		}
		else
		{  
			if ( !PSID_fileSupportSave( fMyOut, cachePtr ) )
			{
				info.statusString = text_fileIoError;
			}
			else
			{
				info.statusString = text_noErrors;
				success = true;
			}
			fMyOut.close();
		}
	}
	return success;
}
