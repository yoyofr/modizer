#include "All.h"
#include "APEIO.h"
#include "APECompressCreate.h"
#include "APECompressCore.h"

CAPECompressCreate::CAPECompressCreate()
{
    m_nMaxFrames = 0;
}

CAPECompressCreate::~CAPECompressCreate()
{
}

int CAPECompressCreate::Start(CIO * pioOutput, const WAVEFORMATEX * pwfeInput, int nMaxAudioBytes, int nCompressionLevel, const void * pHeaderData, int nHeaderBytes)
{
    // verify the parameters
    if (pioOutput == NULL || pwfeInput == NULL)
        return ERROR_BAD_PARAMETER;

    // verify the wave format
    if ((pwfeInput->nChannels != 1) && (pwfeInput->nChannels != 2)) 
    {
        return ERROR_INPUT_FILE_UNSUPPORTED_CHANNEL_COUNT;
    }
    if ((pwfeInput->wBitsPerSample != 8) && (pwfeInput->wBitsPerSample != 16) && (pwfeInput->wBitsPerSample != 24)) 
    {
        return ERROR_INPUT_FILE_UNSUPPORTED_BIT_DEPTH;
    }

    // initialize (creates the base classes)
    m_nSamplesPerFrame = 73728;
    if (nCompressionLevel == COMPRESSION_LEVEL_EXTRA_HIGH)
        m_nSamplesPerFrame *= 4;
    else if (nCompressionLevel == COMPRESSION_LEVEL_INSANE)
        m_nSamplesPerFrame *= 16;

    m_spIO.Assign(pioOutput, FALSE, FALSE);
    m_spAPECompressCore.Assign(new CAPECompressCore(m_spIO, pwfeInput, m_nSamplesPerFrame, nCompressionLevel));
    
    // copy the format
    memcpy(&m_wfeInput, pwfeInput, sizeof(WAVEFORMATEX));
    
    // the compression level
    m_nCompressionLevel = nCompressionLevel;
    m_nFrameIndex = 0;
    m_nLastFrameBlocks = m_nSamplesPerFrame;
    
    // initialize the file
    if (nMaxAudioBytes < 0)
        nMaxAudioBytes = 2147483647;

    uint32 nMaxAudioBlocks = nMaxAudioBytes / pwfeInput->nBlockAlign;
    int nMaxFrames = nMaxAudioBlocks / m_nSamplesPerFrame;
    if ((nMaxAudioBlocks % m_nSamplesPerFrame) != 0) nMaxFrames++;
        
    InitializeFile(m_spIO, &m_wfeInput, nMaxFrames,
        m_nCompressionLevel, pHeaderData, nHeaderBytes);
    
    return ERROR_SUCCESS;
}

int CAPECompressCreate::GetFullFrameBytes()
{
    return m_nSamplesPerFrame * m_wfeInput.nBlockAlign;
}

int CAPECompressCreate::EncodeFrame(const void * pInputData, int nInputBytes)
{
    int nInputBlocks = nInputBytes / m_wfeInput.nBlockAlign;
    
    if ((nInputBlocks < m_nSamplesPerFrame) && (m_nLastFrameBlocks < m_nSamplesPerFrame))
    {
        return -1; // can only pass a smaller frame for the very last time
    }

    // update the seek table
    m_spAPECompressCore->GetBitArray()->AdvanceToByteBoundary();
    int nRetVal = SetSeekByte(m_nFrameIndex, m_spIO->GetPosition() + (m_spAPECompressCore->GetBitArray()->GetCurrentBitIndex() / 8));
    if (nRetVal != ERROR_SUCCESS)
        return nRetVal;

    // compress
    nRetVal = m_spAPECompressCore->EncodeFrame(pInputData, nInputBytes);
    
    // update stats
    m_nLastFrameBlocks = nInputBlocks;
    m_nFrameIndex++;

    return nRetVal;
}

#ifdef SHNTOOL
int CAPECompressCreate::Finish(const void * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes, const void * pHeaderData, int nHeaderBytes)
#else
int CAPECompressCreate::Finish(const void * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes)
#endif
{
    // clear the bit array
    RETURN_ON_ERROR(m_spAPECompressCore->GetBitArray()->OutputBitArray(TRUE));
    
    // finalize the file
#ifdef SHNTOOL
    RETURN_ON_ERROR(FinalizeFile(m_spIO, m_nFrameIndex, m_nLastFrameBlocks, 
        pTerminatingData, nTerminatingBytes, nWAVTerminatingBytes, m_spAPECompressCore->GetPeakLevel(), pHeaderData, nHeaderBytes));
#else
    RETURN_ON_ERROR(FinalizeFile(m_spIO, m_nFrameIndex, m_nLastFrameBlocks, 
        pTerminatingData, nTerminatingBytes, nWAVTerminatingBytes, m_spAPECompressCore->GetPeakLevel()));
#endif
    
    return ERROR_SUCCESS;
}

int CAPECompressCreate::SetSeekByte(int nFrame, int nByteOffset)
{
    if (nFrame >= m_nMaxFrames) return ERROR_APE_COMPRESS_TOO_MUCH_DATA;
    m_spSeekTable[nFrame] = nByteOffset;
    return ERROR_SUCCESS;
}

int CAPECompressCreate::InitializeFile(CIO * pIO, const WAVEFORMATEX * pwfeInput, int nMaxFrames, int nCompressionLevel, const void * pHeaderData, int nHeaderBytes)
{
    // error check the parameters
    if (pIO == NULL || pwfeInput == NULL || nMaxFrames <= 0)
        return ERROR_BAD_PARAMETER;
    
    APE_DESCRIPTOR APEDescriptor; memset(&APEDescriptor, 0, sizeof(APEDescriptor));
    APE_HEADER APEHeader; memset(&APEHeader, 0, sizeof(APEHeader));

    // create the descriptor (only fill what we know)
    APEDescriptor.cID[0] = 'M';
    APEDescriptor.cID[1] = 'A';
    APEDescriptor.cID[2] = 'C';
    APEDescriptor.cID[3] = ' ';
    APEDescriptor.nVersion = MAC_FILE_VERSION_NUMBER;
    
    APEDescriptor.nDescriptorBytes = sizeof(APEDescriptor);
    APEDescriptor.nHeaderBytes = sizeof(APEHeader);
    APEDescriptor.nSeekTableBytes = nMaxFrames * sizeof(unsigned int);
    APEDescriptor.nHeaderDataBytes = (nHeaderBytes == CREATE_WAV_HEADER_ON_DECOMPRESSION) ? 0 : nHeaderBytes;

    swap_ape_descriptor(&APEDescriptor);

    // create the header (only fill what we know now)
    APEHeader.nBitsPerSample = pwfeInput->wBitsPerSample;
    APEHeader.nChannels = pwfeInput->nChannels;
    APEHeader.nSampleRate = pwfeInput->nSamplesPerSec;
    
    APEHeader.nCompressionLevel = (uint16) nCompressionLevel;
    APEHeader.nFormatFlags = (nHeaderBytes == CREATE_WAV_HEADER_ON_DECOMPRESSION) ? MAC_FORMAT_FLAG_CREATE_WAV_HEADER : 0;

    APEHeader.nBlocksPerFrame = m_nSamplesPerFrame;

    swap_ape_header(&APEHeader);

    // write the data to the file
    unsigned int nBytesWritten = 0;
    RETURN_ON_ERROR(pIO->Write(&APEDescriptor, sizeof(APEDescriptor), &nBytesWritten))
    RETURN_ON_ERROR(pIO->Write(&APEHeader, sizeof(APEHeader), &nBytesWritten))
        
    // write an empty seek table
    m_spSeekTable.Assign(new uint32 [nMaxFrames], TRUE);
    if (m_spSeekTable == NULL) { return ERROR_INSUFFICIENT_MEMORY; }
    ZeroMemory(m_spSeekTable, nMaxFrames * 4);
    RETURN_ON_ERROR(pIO->Write(m_spSeekTable, (nMaxFrames * 4), &nBytesWritten))
    m_nMaxFrames = nMaxFrames;

    // write the WAV data
    if ((pHeaderData != NULL) && (nHeaderBytes > 0) && (nHeaderBytes != CREATE_WAV_HEADER_ON_DECOMPRESSION))
    {
        m_spAPECompressCore->GetBitArray()->GetMD5Helper().AddData(pHeaderData, nHeaderBytes);
        RETURN_ON_ERROR(pIO->Write((void *) pHeaderData, nHeaderBytes, &nBytesWritten))
    }

    return ERROR_SUCCESS;
}

#ifdef SHNTOOL
void ULONG_TO_UCHAR_LE(unsigned char * buf,unsigned long num)
/* converts an unsigned long to 4 bytes stored in little-endian format */
{
  buf[0] = (unsigned char)(num);
  buf[1] = (unsigned char)(num >> 8);
  buf[2] = (unsigned char)(num >> 16);
  buf[3] = (unsigned char)(num >> 24);
}
#endif
 
#ifdef SHNTOOL
int CAPECompressCreate::FinalizeFile(CIO * pIO, int nNumberOfFrames, int nFinalFrameBlocks, const void * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes, int nPeakLevel, const void * pHeaderData, int nHeaderBytes)
#else
int CAPECompressCreate::FinalizeFile(CIO * pIO, int nNumberOfFrames, int nFinalFrameBlocks, const void * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes, int nPeakLevel)
#endif
{
    // store the tail position
    int nTailPosition = pIO->GetPosition();

    // append the terminating data
    unsigned int nBytesWritten = 0;
    unsigned int nBytesRead = 0;
    int nRetVal = 0;
    if ((pTerminatingData != NULL) && (nTerminatingBytes > 0))
    {
        // update the MD5 sum to include the WAV terminating bytes
        m_spAPECompressCore->GetBitArray()->GetMD5Helper().AddData(pTerminatingData, nWAVTerminatingBytes);

        // write the entire chunk to the new file
        if (pIO->Write((void *) pTerminatingData, nTerminatingBytes, &nBytesWritten) != 0) { return ERROR_IO_WRITE; }
    }
    
    // go to the beginning and update the information
    nRetVal = pIO->Seek(0, FILE_BEGIN);
    
    // get the descriptor
    APE_DESCRIPTOR APEDescriptor;
    nRetVal = pIO->Read(&APEDescriptor, sizeof(APEDescriptor), &nBytesRead);
    if ((nRetVal != 0) || (nBytesRead != sizeof(APEDescriptor))) { return ERROR_IO_READ; }

    swap_ape_descriptor(&APEDescriptor);

    // get the header
    APE_HEADER APEHeader;
    nRetVal = pIO->Read(&APEHeader, sizeof(APEHeader), &nBytesRead);
    if (nRetVal != 0 || nBytesRead != sizeof(APEHeader)) { return ERROR_IO_READ; }

    swap_ape_header(&APEHeader);
    
    // update the header
    APEHeader.nFinalFrameBlocks = nFinalFrameBlocks;
    APEHeader.nTotalFrames = nNumberOfFrames;

    // update the descriptor
    APEDescriptor.nAPEFrameDataBytes = nTailPosition - (APEDescriptor.nDescriptorBytes + APEDescriptor.nHeaderBytes + APEDescriptor.nSeekTableBytes + APEDescriptor.nHeaderDataBytes);
    APEDescriptor.nAPEFrameDataBytesHigh = 0;
    APEDescriptor.nTerminatingDataBytes = nWAVTerminatingBytes;
    
    // update the MD5
    m_spAPECompressCore->GetBitArray()->GetMD5Helper().AddData(&APEHeader, sizeof(APEHeader));
    m_spAPECompressCore->GetBitArray()->GetMD5Helper().AddData(m_spSeekTable, m_nMaxFrames * 4);
    m_spAPECompressCore->GetBitArray()->GetMD5Helper().GetResult(APEDescriptor.cFileMD5);

    // set the pointer and re-write the updated header and peak level
    nRetVal = pIO->Seek(0, FILE_BEGIN);

    swap_ape_descriptor(&APEDescriptor);
    swap_ape_header(&APEHeader);

    if (pIO->Write(&APEDescriptor, sizeof(APEDescriptor), &nBytesWritten) != 0) { return ERROR_IO_WRITE; }
    if (pIO->Write(&APEHeader, sizeof(APEHeader), &nBytesWritten) != 0) { return ERROR_IO_WRITE; }

#ifdef WORDS_BIGENDIAN
    int i;
    for (i = 0; i < m_nMaxFrames; i ++)
    {
	m_spSeekTable[i] = swap_int32(m_spSeekTable[i]);
    }
#endif
    // write the updated seek table
    if (pIO->Write(m_spSeekTable, m_nMaxFrames * 4, &nBytesWritten) != 0) { return ERROR_IO_WRITE; }

#ifdef WORDS_BIGENDIAN
    for (i = 0; i < m_nMaxFrames; i ++)
    {
	m_spSeekTable[i] = swap_int32(m_spSeekTable[i]);
    }
#endif

#ifdef SHNTOOL
    char *p;
    int j;
    uint32 nDataSize;
    bool bHasWaveHeader = ((pHeaderData != NULL) && (nHeaderBytes > 0) && (nHeaderBytes != CREATE_WAV_HEADER_ON_DECOMPRESSION));

    // rewrite the WAVE header with known, correct values
    if (bHasWaveHeader)
    {
        CSmartPtr<unsigned char> spOldHeader;
        spOldHeader.Assign(new unsigned char [nHeaderBytes], TRUE);
        memcpy(spOldHeader,pHeaderData,nHeaderBytes);

        nDataSize = (((nNumberOfFrames - 1) * m_nSamplesPerFrame + nFinalFrameBlocks) * m_wfeInput.nBlockAlign);
        for (j = 0; j < nHeaderBytes; j++)
        {
            if ((p = strstr((char *)pHeaderData+j,"RIFF"))) {
                ULONG_TO_UCHAR_LE((unsigned char *)p+4,nDataSize + nHeaderBytes - 8);
                break;
            }
        }
        for (j = 0; j < nHeaderBytes; j++)
        {
            if ((p = strstr((char *)pHeaderData+j,"data"))) {
                ULONG_TO_UCHAR_LE((unsigned char *)p+4,nDataSize);
                break;
            }
        }

        if (pIO->Write((void *) pHeaderData, nHeaderBytes, &nBytesWritten) != 0) { return ERROR_IO_WRITE; }

        if (memcmp(spOldHeader,pHeaderData,nHeaderBytes))
        {
            swap_ape_descriptor(&APEDescriptor);

            GetChecksum(pIO,&APEDescriptor,0,APEDescriptor.cFileMD5);

            swap_ape_descriptor(&APEDescriptor);

            if (pIO->Seek(0, FILE_BEGIN) != ERROR_SUCCESS) { return ERROR_IO_WRITE; }

            if (pIO->Write(&APEDescriptor, sizeof(APEDescriptor), &nBytesWritten) != 0) { return ERROR_IO_WRITE; }
        }

        spOldHeader.Delete();
    }
#endif

    return ERROR_SUCCESS;
}
