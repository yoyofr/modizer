//
// /home/ms/source/sidplay/libsidplay/fformat/RCS/info_.cpp,v
//
// Amiga PlaySID icon tooltype file format (.INFO) support.
//
// This is a derived work, courtesy of Peter Kunath, who has provided an
// examplary source code to examine an Amiga icon file.
//
// It has been ported and heavily modified to suit certain requirements.
//
// This replaces the old code, which was simply scanning input data for
// a first, presumedly constant, Id string. This code does not require the
// default tool to serve as a constant Id by containing "SID:PlaySID".

#include "info_.h"


const char text_format[] = "Raw plus PlaySID icon tooltype file (INFO)";

const char keyword_id[] = "SID:PLAYSID";
const char keyword_address[] = "ADDRESS=";
const char keyword_songs[] = "SONGS=";
const char keyword_speed[] = "SPEED=";
const char keyword_name[] = "NAME=";
const char keyword_author[] = "AUTHOR=";
const char keyword_copyright[] = "COPYRIGHT=";
const char keyword_musPlayer[] = "SIDSONG=YES";

const char text_noMemError[] = "ERROR: Not enough free memory";
const char text_corruptError[] = "ERROR: Info file is incomplete or corrupt";
const char text_noStringsError[] = "ERROR: Info file does not contain required strings";
const char text_dataCorruptError[] = "ERROR: C64 data file is corrupt";
#if defined(SIDTUNE_REJECT_UNKNOWN_FIELDS)
const char text_chunkError[] = "ERROR: Invalid tooltype information in icon file";
#endif

const uint safeBufferSize = 64;  // for string comparison, stream parsing


bool copyItem(smartPtr<const char>& spIn, smartPtr<char>& spCmpBuf, udword itemLen)
{
    for ( uword i = 0; i < itemLen; i++ )
    {
        spCmpBuf[i] = spIn[i];
    }
    return (spIn && spCmpBuf);
}


bool sidTune::INFO_fileSupport(const void* dataBuffer, udword dataLength,
                               const void* infoBuffer, udword infoLength)
{
    // Remove any format description or format error string.
    info.formatString = 0;

    // Require a first minimum safety size.
    ulong minSize = 1+sizeof(struct DiskObject);
    if (infoLength < minSize)
        return( false );

    const DiskObject *dobject = (const DiskObject *)infoBuffer;

    // Require Magic_Id in the first two bytes of the file.
    if ( readEndian(dobject->Magic[0],dobject->Magic[1]) != WB_DISKMAGIC )
        return false;

    // Only version 1.x supported.
    if ( readEndian(dobject->Version[0],dobject->Version[1]) != WB_DISKVERSION )
        return false;

    // A PlaySID icon must be of type project.
    if ( dobject->Type != WB_PROJECT )
        return false;

    int i;  // general purpose index variable

    // We want to skip a possible Gadget Image item.
    const char *icon = (const char*)infoBuffer + sizeof(DiskObject);

    if ( (readEndian(dobject->Gadget.Flags[0],dobject->Gadget.Flags[1]) & GFLG_GADGIMAGE) == 0)
    {
        // Calculate size of gadget borders (vector image).
		
        if (dobject->Gadget.pGadgetRender[0] |
            dobject->Gadget.pGadgetRender[1] |
            dobject->Gadget.pGadgetRender[2] |
            dobject->Gadget.pGadgetRender[3])  // border present?
        {
            // Require another minimum safety size.
            minSize += sizeof(struct Border);
            if (infoLength < minSize)
                return( false );

            const Border *brd = (const Border *)icon;
            icon += sizeof(Border);
            icon += brd->Count * (2+2);	       // pair of uword
        }

        if (dobject->Gadget.pSelectRender[0] |
            dobject->Gadget.pSelectRender[1] |
            dobject->Gadget.pSelectRender[2] |
            dobject->Gadget.pSelectRender[3])  // alternate border present?
        {
            // Require another minimum safety size.
            minSize += sizeof(Border);
            if (infoLength < minSize)
                return( false );

            const Border *brd = (const Border *)icon;
            icon += sizeof(Border);
            icon += brd->Count * (2+2);	       // pair of uword
        }
    }
    else
    {
        // Calculate size of gadget images (bitmap image).

        if (dobject->Gadget.pGadgetRender[0] |
            dobject->Gadget.pGadgetRender[1] |
            dobject->Gadget.pGadgetRender[2] |
            dobject->Gadget.pGadgetRender[3])  // image present?
        {
            // Require another minimum safety size.
            minSize += sizeof(Image);
            if (infoLength < minSize)
                return( false );

            const Image *img = (const Image *)icon;
            icon += sizeof(Image);

            udword imgsize = 0;
            for(i=0;i<readEndian(img->Depth[0],img->Depth[1]);i++)
            {
                if ( (img->PlanePick & (1<<i)) != 0)
                {
                    // NOTE: Intuition relies on PlanePick to know how many planes
                    // of data are found in ImageData. There should be no more
                    // '1'-bits in PlanePick than there are planes in ImageData.
                    imgsize++;
                }
            }

            imgsize *= ((readEndian(img->Width[0],img->Width[1])+15)/16)*2;  // bytes per line
            imgsize *= readEndian(img->Height[0],img->Height[1]);            // bytes per plane

            icon += imgsize;
        }
      
        if (dobject->Gadget.pSelectRender[0] |
            dobject->Gadget.pSelectRender[1] |
            dobject->Gadget.pSelectRender[2] |
            dobject->Gadget.pSelectRender[3])  // alternate image present?
        {
            // Require another minimum safety size.
            minSize += sizeof(Image);
            if (infoLength < minSize)
                return( false );

            const Image *img = (const Image *)icon;
            icon += sizeof(Image);

            udword imgsize = 0;
            for(i=0;i<readEndian(img->Depth[0],img->Depth[1]);i++)
            {
                if ( (img->PlanePick & (1<<i)) != 0)
                {
                    // NOTE: Intuition relies on PlanePick to know how many planes
                    // of data are found in ImageData. There should be no more
                    // '1'-bits in PlanePick than there are planes in ImageData.
                    imgsize++;
                }
            }

            imgsize *= ((readEndian(img->Width[0],img->Width[1])+15)/16)*2;  // bytes per line
            imgsize *= readEndian(img->Height[0],img->Height[1]);            // bytes per plane
            icon += imgsize;
        }
    }

    // Here use a smart pointer to prevent access violation errors.
    smartPtr<const char> spTool((const char*)icon,infoLength-(ulong)(icon-(const char*)infoBuffer));
    if ( !spTool )
    {
        info.formatString = text_corruptError;
        return false;
    }

    // A separate safe buffer is used for each tooltype string.
#ifdef SID_HAVE_EXCEPTIONS
    smartPtr<char> spCmpBuf(new(std::nothrow) char[safeBufferSize],safeBufferSize,true);
#else
    smartPtr<char> spCmpBuf(new char[safeBufferSize],safeBufferSize,true);
#endif
    if ( !spCmpBuf )
    {
        info.formatString = text_noMemError;
        return false;
    }
    char* cmpBuf = spCmpBuf.tellBegin();

    // Skip default tool.
    spTool += readEndian(spTool[0],spTool[1],spTool[2],spTool[3]) + 4;

    // Defaults.
    fileOffset = 0;	               // no header in separate data file
    info.musPlayer = false;
    info.numberOfInfoStrings = 0;
    udword oldStyleSpeed = 0;

    // Flags for required entries.
    bool hasAddress = false,
        hasName = false,
        hasAuthor = false,
        hasCopyright = false,
        hasSongs = false,
        hasSpeed = false,
        hasUnknownChunk = false;

    // Calculate number of tooltype strings.
    i = (readEndian(spTool[0],spTool[1],spTool[2],spTool[3])/4) - 1;
    spTool += 4;  // skip size info

    while( i-- > 0 )
    {
        // Get length of this tool.
        udword toolLen = readEndian(spTool[0],spTool[1],spTool[2],spTool[3]);
        spTool += 4;  // skip tool length
        // Copy item to safe buffer.
        if ( !copyItem(spTool,spCmpBuf,toolLen) )
        {
            return false;
        }

        // Now check all possible keywords.
        if ( myStrNcaseCmp(cmpBuf,keyword_address) == 0 )
        {
#ifdef SID_HAVE_SSTREAM
            string sAddrIn(cmpBuf + strlen(keyword_address),
                           toolLen - strlen(keyword_address));
            istringstream addrIn( sAddrIn );
#else
            istrstream addrIn(cmpBuf + strlen(keyword_address),
                              toolLen - strlen(keyword_address));
#endif
            info.loadAddr = (uword)readHex( addrIn );
            info.initAddr = (uword)readHex( addrIn );
            info.playAddr = (uword)readHex( addrIn );
            if ( !addrIn )
            {
                return false;
            }
            hasAddress = true;
        }
        else if ( myStrNcaseCmp(cmpBuf,keyword_songs) == 0 )
        {
#ifdef SID_HAVE_SSTREAM
            string sNumIn( cmpBuf + strlen(keyword_songs),
                           toolLen - strlen(keyword_songs) );
            istringstream numIn( sNumIn );
#else
            istrstream numIn( cmpBuf + strlen(keyword_songs),
                              toolLen - strlen(keyword_songs) );
#endif
            if ( !numIn )
            {
                return false;
            }
            info.songs = (uword)readDec( numIn );
            info.startSong = (uword)readDec( numIn );
            hasSongs = true;
        }
        else if ( myStrNcaseCmp(cmpBuf,keyword_speed) == 0 )
        {
#ifdef SID_HAVE_SSTREAM
            string sSpeedIn( cmpBuf + strlen(keyword_speed),
                            toolLen - strlen(keyword_speed) );
            istringstream speedIn( sSpeedIn );
#else
            istrstream speedIn( cmpBuf + strlen(keyword_speed),
                                toolLen - strlen(keyword_speed) );
#endif
            if ( !speedIn )
            {
                return false;
            }
            oldStyleSpeed = readHex(speedIn);
            hasSpeed = true;
        }
        else if ( myStrNcaseCmp(cmpBuf,keyword_name) == 0 )
        {
            strncpy( &infoString[0][0], cmpBuf + strlen(keyword_name), 31 );
            info.nameString = &infoString[0][0];
            info.infoString[0] = &infoString[0][0];
            hasName = true;
        }
        else if ( myStrNcaseCmp(cmpBuf,keyword_author) == 0 )
        {
            strncpy( &infoString[1][0], cmpBuf + strlen(keyword_author), 31 );
            info.authorString = &infoString[1][0];
            info.infoString[1] = &infoString[1][0];
            hasAuthor = true;
        }
        else if ( myStrNcaseCmp(cmpBuf,keyword_copyright) == 0 )
        {
            strncpy( &infoString[2][0], cmpBuf + strlen(keyword_copyright), 31 );
            info.copyrightString = &infoString[2][0];
            info.infoString[2] = &infoString[2][0];
            hasCopyright = true;
        }
        else if ( myStrNcaseCmp(cmpBuf,keyword_musPlayer) == 0 )
        {
            info.musPlayer = true;
        }
        else
        {
            hasUnknownChunk = true;
#if defined(SIDTUNE_REJECT_UNKNOWN_FIELDS)
            info.formatString = text_chunkError;
            return false;
#endif
        }
        // Skip to next tool.
        spTool += toolLen;
    }

    // Collected ``required'' information complete ?
    if ( hasAddress && hasName && hasAuthor && hasCopyright && hasSongs && hasSpeed )
    {
        // Create the speed/clock setting table.
        convertOldStyleSpeedToTables(oldStyleSpeed);
        if (( info.loadAddr == 0 ) && ( dataLength != 0 ))
        {
            smartPtr<const ubyte> spDataBuf((const ubyte*)dataBuffer,dataLength);
            spDataBuf += fileOffset;
            info.loadAddr = readEndian(spDataBuf[1],spDataBuf[0]);
            if ( !spDataBuf )
            {
                info.formatString = text_dataCorruptError;
                return false;
            }
            fileOffset += 2;
        }
        if ( info.initAddr == 0 )
        {
            info.initAddr = info.loadAddr;
        }
        info.numberOfInfoStrings = 3;
        // We finally accept the input data.
        info.formatString = text_format;
        return true;
    }
    else if ( hasAddress || hasName || hasAuthor || hasCopyright || hasSongs || hasSpeed )
    {
        // Something is missing (or damaged?).
        info.formatString = text_corruptError;
        return false;
    }
    else
    {
        // No PlaySID conform info strings.
        info.formatString = text_noStringsError;
        return false;
    }
}
