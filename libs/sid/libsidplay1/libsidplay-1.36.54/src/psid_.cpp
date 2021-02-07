//
// /home/ms/source/sidplay/libsidplay/fformat/RCS/psid_.cpp,v
//

#include "psid_.h"


const char text_format[] = "PlaySID one-file format (PSID)";
const char text_psidTruncated[] = "ERROR: PSID file is most likely truncated";

const int maxStringLength = 31;


bool sidTune::PSID_fileSupport(const void* buffer, udword bufLen)
{
	// Remove any format description or format error string.
	info.formatString = 0;

	// Require a first minimum size.
	if (bufLen < 6)
	{
		return false;
	}
	// Now it is safe to access the first bytes.
	// Require a valid ID and version number.
	const psidHeader* pHeader = (const psidHeader*)buffer;
	if ( (readBEdword((const ubyte*)pHeader->id) != 0x50534944)  // "PSID" ?
		|| (readBEword(pHeader->version) >= 3) )
	{
		return false;
	}
	// Due to security concerns, input must be at least as long as version 1
	// plus C64 load address data. That is the area which will be accessed.
	if ( bufLen < (sizeof(psidHeader)+2) )
	{
		info.formatString = text_psidTruncated;
		return false;
	}

	fileOffset = readBEword(pHeader->data);
	info.loadAddr = readBEword(pHeader->load);
	info.initAddr = readBEword(pHeader->init);
	info.playAddr = readBEword(pHeader->play);
	info.songs = readBEword(pHeader->songs);
	info.startSong = readBEword(pHeader->start);

	if (info.songs > classMaxSongs)
	{
		info.songs = classMaxSongs;
	}
	
	info.musPlayer = false;
    info.psidSpecific = false;
	if ( readBEword(pHeader->version) >= 2 )
	{
		if ( readBEword(pHeader->flags) & 0x01 )
		{
			info.musPlayer = true;
		}
		if ( readBEword(pHeader->flags) & 0x02 )
		{
			info.psidSpecific = true;
		}
        info.clock = ( readBEword(pHeader->flags)&0x0c ) >> 2;
        info.sidModel = ( readBEword(pHeader->flags)&0x30 ) >> 4;
        info.relocStartPage = pHeader->relocStartPage;
        info.relocPages = pHeader->relocPages;
        info.reserved = readBEword(pHeader->reserved);
	}
    else
    {
        info.clock = SIDTUNE_CLOCK_UNKNOWN;
        info.sidModel = SIDTUNE_SIDMODEL_UNKNOWN;
        info.relocStartPage = info.relocPages = 0;
        info.reserved = 0;
    }

    // Create the speed/clock setting table.
	convertOldStyleSpeedToTables(readBEdword(pHeader->speed));

	if ( info.loadAddr == 0 )
	{
		ubyte* pData = (ubyte*)buffer + fileOffset;
		info.loadAddr = readEndian( *(pData+1), *pData );
		fileOffset += 2;
	}
	if ( info.initAddr == 0 )
	{
		info.initAddr = info.loadAddr;
	}
	
	// Copy info strings, so they will not get lost.
	strncpy(&infoString[0][0],pHeader->name,maxStringLength);
	info.nameString = &infoString[0][0];
	info.infoString[0] = &infoString[0][0];
	strncpy(&infoString[1][0],pHeader->author,maxStringLength);
	info.authorString = &infoString[1][0];
	info.infoString[1] = &infoString[1][0];
	strncpy(&infoString[2][0],pHeader->copyright,maxStringLength);
	info.copyrightString = &infoString[2][0];
	info.infoString[2] = &infoString[2][0];
	info.numberOfInfoStrings = 3;
	
	info.formatString = text_format;
	return true;
}


bool sidTune::PSID_fileSupportSave(ofstream& fMyOut, const ubyte* dataBuffer)
{
	psidHeader myHeader;
	writeBEdword((ubyte*)myHeader.id,0x50534944);  // 'PSID'
	writeBEword(myHeader.version,2);
	writeBEword(myHeader.data,0x7C);
	writeBEword(myHeader.load,0);
	writeBEword(myHeader.init,info.initAddr);
	writeBEword(myHeader.play,info.playAddr);
	writeBEword(myHeader.songs,info.songs);
	writeBEword(myHeader.start,info.startSong);

	udword speed = 0;
	int maxBugSongs = ((info.songs <= 32) ? info.songs : 32);
	for (int s = 0; s < maxBugSongs; s++)
	{
		if (songSpeed[s] == SIDTUNE_SPEED_CIA_1A)
		{
			speed |= (1<<s);
		}
	}
	writeBEdword(myHeader.speed,speed);

	uword tmpFlags = (info.clock<<2)|(info.sidModel<<4);
	if ( info.musPlayer )
	{
		tmpFlags |= 0x01;
	}
	if ( info.psidSpecific )
	{
		tmpFlags |= 0x02;
	}
	writeBEword(myHeader.flags,tmpFlags);
    myHeader.relocStartPage = info.relocStartPage;
    myHeader.relocPages = info.relocPages;
	writeBEword(myHeader.reserved,info.reserved);
	for ( int i = 0; i < 32; i++ )
	{
		myHeader.name[i] = 0;
		myHeader.author[i] = 0;
		myHeader.copyright[i] = 0;
	}
	strncpy( myHeader.name, info.nameString, 31 );
	strncpy( myHeader.author, info.authorString, 31 );
	strncpy( myHeader.copyright, info.copyrightString, 31 );
	fMyOut.write( (char*)&myHeader, sizeof(psidHeader) );
	
	// Save C64 lo/hi load address (little-endian).
	ubyte saveAddr[2];
	saveAddr[0] = info.loadAddr & 255;
	saveAddr[1] = info.loadAddr >> 8;
	fMyOut.write( (char*)saveAddr, 2 );
	// Data starts at: bufferaddr + fileoffset
	// Data length: datafilelen - fileoffset
	fMyOut.write( (const char*)dataBuffer + fileOffset, info.dataFileLen - fileOffset );
	if ( !fMyOut )
	{
		return false;
	}
	else
	{
		return true;
	}
}
