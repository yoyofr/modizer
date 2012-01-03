//
// /home/ms/source/sidplay/libsidplay/fformat/RCS/sid_.cpp,v
// 

#include "sid_.h"


const char text_format[] = "Raw plus SIDPLAY ASCII text file (SID)";

const char text_truncatedError[] = "ERROR: SID file is truncated";
const char text_noMemError[] = "ERROR: Not enough free memory";

const char keyword_id[] = "SIDPLAY INFOFILE";

const char keyword_name[] = "NAME=";            // No white-space characters 
const char keyword_author[] = "AUTHOR=";        // in these keywords, because
const char keyword_copyright[] = "COPYRIGHT=";  // we want to use a white-space
const char keyword_address[] = "ADDRESS=";      // eating string stream to
const char keyword_songs[] = "SONGS=";          // parse most of the header.
const char keyword_speed[] = "SPEED=";
const char keyword_musPlayer[] = "SIDSONG=YES";

const uint sidMinFileSize = 1+sizeof(keyword_id);  // Just to avoid a first segm.fault.
const uint parseChunkLen = 80;                     // Enough for all keywords incl. their values.


bool sidTune::SID_fileSupport(const void* dataBuffer, udword dataBufLen,
                              const void* sidBuffer, udword sidBufLen)
{
	// Remove any format description or error string.
	info.formatString = 0;
	
	// Make sure SID buffer pointer is not zero.
	// Check for a minimum file size. If it is smaller, we will not proceed.
	if ((sidBuffer==0) || (sidBufLen<sidMinFileSize))
	{
		return false;
	}

	const char* pParseBuf = (const char*)sidBuffer;
	// First line has to contain the exact identification string.
	if ( myStrNcaseCmp( pParseBuf, keyword_id ) != 0 )
	{
		return false;
	}
	else
	{
		// At least the ID was found, so set a default error message.
		info.formatString = text_truncatedError;
		
		// Defaults.
		fileOffset = 0;                // no header in separate data file
		info.musPlayer = false;
		info.numberOfInfoStrings = 0;
		udword oldStyleSpeed = 0;

		// Flags for required entries.
		bool hasAddress = false,
		    hasName = false,
		    hasAuthor = false,
		    hasCopyright = false,
		    hasSongs = false,
		    hasSpeed = false;
	
		// Using a temporary instance of an input string chunk.
#ifdef SID_HAVE_EXCEPTIONS
		char* pParseChunk = new(std::nothrow) char[parseChunkLen+1];
#else
		char* pParseChunk = new char[parseChunkLen+1];
#endif
		if ( pParseChunk == 0 )
		{
			info.formatString = text_noMemError;
			return false;
		}
		
		// Parse as long we have not collected all ``required'' entries.
		while ( !hasAddress || !hasName || !hasAuthor || !hasCopyright
			    || !hasSongs || !hasSpeed )
		{
			// Skip to next line. Leave loop, if none.
			if (( pParseBuf = returnNextLine( pParseBuf )) == 0 )
			{
				break;
			}
			// And get a second pointer to the following line.
			const char* pNextLine = returnNextLine( pParseBuf );
			udword restLen;
			if ( pNextLine != 0 )
			{
				// Calculate number of chars between current pos and next line.
				restLen = (udword)(pNextLine - pParseBuf);
			}
			else
			{
				// Calculate number of chars between current pos and end of buf.
				restLen = sidBufLen - (udword)(pParseBuf - (char*)sidBuffer);
			}
			// Create whitespace eating (!) input string stream.
#ifdef SID_HAVE_SSTREAM
			string sParse( pParseBuf, restLen );
            istringstream parseStream( sParse );
			// A second one just for copying.
			istringstream parseCopyStream( sParse );
#else
			istrstream parseStream( pParseBuf, restLen );
			istrstream parseCopyStream( pParseBuf, restLen );
#endif
			if ( !parseStream || !parseCopyStream )
			{
				break;
			}
			// Now copy the next X characters except white-spaces.
			for ( uint i = 0; i < parseChunkLen; i++ )
			{
				char c;
				parseCopyStream >> c;
				pParseChunk[i] = c;
			}
			pParseChunk[parseChunkLen]=0;
			// Now check for the possible keywords.
			// ADDRESS
			if ( myStrNcaseCmp( pParseChunk, keyword_address ) == 0 )
			{
				skipToEqu( parseStream );
				info.loadAddr = (uword)readHex( parseStream );
				if ( !parseStream )
				    break;
				info.initAddr = (uword)readHex( parseStream );
				if ( !parseStream )
				    break;
				info.playAddr = (uword)readHex( parseStream );
				hasAddress = true;
			}
			// NAME
			else if ( myStrNcaseCmp( pParseChunk, keyword_name ) == 0 )
			{
				copyStringValueToEOL( pParseBuf, &infoString[0][0], infoStringLen );
				info.nameString = &infoString[0][0];
				info.infoString[0] = &infoString[0][0];
				hasName = true;
			}
			// AUTHOR
			else if ( myStrNcaseCmp( pParseChunk, keyword_author ) == 0 )
			{
				copyStringValueToEOL( pParseBuf, &infoString[1][0], infoStringLen );
				info.authorString = &infoString[1][0];
				info.infoString[1] = &infoString[1][0];
				hasAuthor = true;
			}
			// COPYRIGHT
			else if ( myStrNcaseCmp( pParseChunk, keyword_copyright ) == 0 )
			{
				copyStringValueToEOL( pParseBuf, &infoString[2][0], infoStringLen );
				info.copyrightString = &infoString[2][0];
				info.infoString[2] = &infoString[2][0];
				hasCopyright = true;
			}
			// SONGS
			else if ( myStrNcaseCmp( pParseChunk, keyword_songs ) == 0 )
			{
				skipToEqu( parseStream );
				info.songs = (uword)readDec( parseStream );
				info.startSong = (uword)readDec( parseStream );
				hasSongs = true;
			}
			// SPEED
			else if ( myStrNcaseCmp( pParseChunk, keyword_speed ) == 0 )
			{
				skipToEqu( parseStream );
				oldStyleSpeed = readHex(parseStream);
				hasSpeed = true;
			}
			// SIDSONG
			else if ( myStrNcaseCmp( pParseChunk, keyword_musPlayer ) == 0 )
			{
				info.musPlayer = true;
			}
        };
		
        delete[] pParseChunk;
		
		// Again check for the ``required'' values.
		if ( hasAddress || hasName || hasAuthor || hasCopyright || hasSongs || hasSpeed )
		{
			// Create the speed/clock setting table.
			convertOldStyleSpeedToTables(oldStyleSpeed);
			// loadAddr = 0 means, the address is stored in front of the C64 data.
			// We cannot verify whether the dataBuffer contains valid data.
		    // All we want to know is whether the SID buffer is valid.
			// If data is present, we access it (here to get the C64 load address).
			if (info.loadAddr==0 && (dataBufLen>=(fileOffset+2)) && dataBuffer!=0)
			{
				const ubyte* pDataBufCp = (const ubyte*)dataBuffer + fileOffset;
				info.loadAddr = readEndian( *(pDataBufCp + 1), *pDataBufCp );
				fileOffset += 2;  // begin of data
			}
			// Keep compatibility to PSID/SID.
			if ( info.initAddr == 0 )
			{
				info.initAddr = info.loadAddr;
			}
			info.numberOfInfoStrings = 3;
			// We finally accept the input data.
			info.formatString = text_format;
			return true;
		}
		else
		{
			// Something is missing (or damaged ?).
			// Error string set above.
			return false;
		}
	}
}


bool sidTune::SID_fileSupportSave( ofstream& fMyOut )
{
	fMyOut << keyword_id << endl
		<< keyword_address << hex << setw(4) << setfill('0') << 0 << ','
		<< hex << setw(4) << info.initAddr << ","
		<< hex << setw(4) << info.playAddr << endl
		<< keyword_songs << dec << (int)info.songs << "," << (int)info.startSong << endl;

	udword oldStyleSpeed = 0;
	int maxBugSongs = ((info.songs <= 32) ? info.songs : 32);
	for (int s = 0; s < maxBugSongs; s++)
	{
		if (songSpeed[s] == SIDTUNE_SPEED_CIA_1A)
		{
			oldStyleSpeed |= (1<<s);
		}
	}

	fMyOut
		<< keyword_speed << hex << setw(8) << oldStyleSpeed << endl
		<< keyword_name << info.nameString << endl
		<< keyword_author << info.authorString << endl
		<< keyword_copyright << info.copyrightString << endl;
	if ( info.musPlayer )
	{
		fMyOut << keyword_musPlayer << endl;
	}
	if ( !fMyOut )
	{
		return false;
	}
	else
	{
		return true;
	}
}
