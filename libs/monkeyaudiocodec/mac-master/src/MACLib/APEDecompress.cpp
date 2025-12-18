#include "All.h"
#include "APEDecompress.h"

#include "APEInfo.h"
#include "Prepare.h"
#include "UnBitArray.h"
#include "NewPredictor.h"
#include <algorithm>

#define DECODE_BLOCK_SIZE        4096

CAPEDecompress::CAPEDecompress(int * pErrorCode, CAPEInfo * pAPEInfo, int nStartBlock, int nFinishBlock)
{
    *pErrorCode = ERROR_SUCCESS;

    // open / analyze the file
    m_spAPEInfo.Assign(pAPEInfo);

    // version check (this implementation only works with 3.93 and later files)
    if (GetInfo(APE_INFO_FILE_VERSION) < 3930)
    {
        *pErrorCode = ERROR_UNDEFINED;
        return;
    }

    // get format information
    GetInfo(APE_INFO_WAVEFORMATEX, (intptr_t) &m_wfeInput);
    m_nBlockAlign = GetInfo(APE_INFO_BLOCK_ALIGN);

    // initialize other stuff
    m_bDecompressorInitialized = FALSE;
    m_nCurrentFrame = 0;
    m_nCurrentBlock = 0;
    m_nCurrentFrameBufferBlock = 0;
    m_nFrameBufferFinishedBlocks = 0;
    m_bErrorDecodingCurrentFrame = FALSE;
    m_nErrorDecodingCurrentFrameOutputSilenceBlocks = 0;

    // set the "real" start and finish blocks
    m_nStartBlock = (nStartBlock < 0) ? 0 : (std::min<int>)(nStartBlock, GetInfo(APE_INFO_TOTAL_BLOCKS));
    m_nFinishBlock = (nFinishBlock < 0) ? GetInfo(APE_INFO_TOTAL_BLOCKS) : (std::min<int>)(nFinishBlock, GetInfo(APE_INFO_TOTAL_BLOCKS));
    m_bIsRanged = (m_nStartBlock != 0) || (m_nFinishBlock != GetInfo(APE_INFO_TOTAL_BLOCKS));
}

CAPEDecompress::~CAPEDecompress()
{
}

int CAPEDecompress::InitializeDecompressor()
{
    // check if we have anything to do
    if (m_bDecompressorInitialized)
        return ERROR_SUCCESS;

    // update the initialized flag
    m_bDecompressorInitialized = TRUE;

    // create a frame buffer
    m_cbFrameBuffer.CreateBuffer((GetInfo(APE_INFO_BLOCKS_PER_FRAME) + DECODE_BLOCK_SIZE) * m_nBlockAlign, m_nBlockAlign * 64);
    
    // create decoding components
    m_spUnBitArray.Assign((CUnBitArrayBase *) CreateUnBitArray(this, GetInfo(APE_INFO_FILE_VERSION)));

    if (GetInfo(APE_INFO_FILE_VERSION) >= 3950)
    {
        m_spNewPredictorX.Assign(new CPredictorDecompress3950toCurrent(GetInfo(APE_INFO_COMPRESSION_LEVEL), GetInfo(APE_INFO_FILE_VERSION)));
        m_spNewPredictorY.Assign(new CPredictorDecompress3950toCurrent(GetInfo(APE_INFO_COMPRESSION_LEVEL), GetInfo(APE_INFO_FILE_VERSION)));
    }
    else
    {
        m_spNewPredictorX.Assign(new CPredictorDecompressNormal3930to3950(GetInfo(APE_INFO_COMPRESSION_LEVEL), GetInfo(APE_INFO_FILE_VERSION)));
        m_spNewPredictorY.Assign(new CPredictorDecompressNormal3930to3950(GetInfo(APE_INFO_COMPRESSION_LEVEL), GetInfo(APE_INFO_FILE_VERSION)));
    }
    
    // seek to the beginning
    return Seek(0);
}

int CAPEDecompress::GetData(char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    int nRetVal = ERROR_SUCCESS;
    if (pBlocksRetrieved) *pBlocksRetrieved = 0;
    
    // make sure we're initialized
    RETURN_ON_ERROR(InitializeDecompressor())

    // cap
    int nBlocksUntilFinish = m_nFinishBlock - m_nCurrentBlock;
    const int nBlocksToRetrieve = (std::min<int>)(nBlocks, nBlocksUntilFinish);
    
    // get the data
    unsigned char * pOutputBuffer = (unsigned char *) pBuffer;
    int nBlocksLeft = nBlocksToRetrieve; int nBlocksThisPass = 1;
    while ((nBlocksLeft > 0) && (nBlocksThisPass > 0))
    {
        // fill up the frame buffer
        int nDecodeRetVal = FillFrameBuffer();
        if (nDecodeRetVal != ERROR_SUCCESS)
            nRetVal = nDecodeRetVal;

        // analyze how much to remove from the buffer
        const int nFrameBufferBlocks = m_nFrameBufferFinishedBlocks;
        nBlocksThisPass = (std::min<int>)(nBlocksLeft, nFrameBufferBlocks);

        // remove as much as possible
        if (nBlocksThisPass > 0)
        {
            m_cbFrameBuffer.Get(pOutputBuffer, nBlocksThisPass * m_nBlockAlign);
            pOutputBuffer += nBlocksThisPass * m_nBlockAlign;
            nBlocksLeft -= nBlocksThisPass;
            m_nFrameBufferFinishedBlocks -= nBlocksThisPass;
        }
    }

    // calculate the blocks retrieved
    int nBlocksRetrieved = nBlocksToRetrieve - nBlocksLeft;

    // update position
    m_nCurrentBlock += nBlocksRetrieved;
    if (pBlocksRetrieved) *pBlocksRetrieved = nBlocksRetrieved;

    return nRetVal;
}

int CAPEDecompress::Seek(int nBlockOffset)
{
    RETURN_ON_ERROR(InitializeDecompressor())

    // use the offset
    nBlockOffset += m_nStartBlock;
    
    // cap (to prevent seeking too far)
    if (nBlockOffset >= m_nFinishBlock)
        nBlockOffset = m_nFinishBlock - 1;
    if (nBlockOffset < m_nStartBlock)
        nBlockOffset = m_nStartBlock;

    // seek to the perfect location
    int nBaseFrame = nBlockOffset / GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    int nBlocksToSkip = nBlockOffset % GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    int nBytesToSkip = nBlocksToSkip * m_nBlockAlign;
        
    m_nCurrentBlock = nBaseFrame * GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    m_nCurrentFrameBufferBlock = nBaseFrame * GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    m_nCurrentFrame = nBaseFrame;
    m_nFrameBufferFinishedBlocks = 0;
    m_cbFrameBuffer.Empty();
    RETURN_ON_ERROR(SeekToFrame(m_nCurrentFrame));

    // skip necessary blocks
    CSmartPtr<char> spTempBuffer(new char [nBytesToSkip], TRUE);
    if (spTempBuffer == NULL) return ERROR_INSUFFICIENT_MEMORY;
    
    int nBlocksRetrieved = 0;
    GetData(spTempBuffer, nBlocksToSkip, &nBlocksRetrieved);
    if (nBlocksRetrieved != nBlocksToSkip)
        return ERROR_UNDEFINED;

    return ERROR_SUCCESS;
}

/*****************************************************************************************
Decodes blocks of data
*****************************************************************************************/
int CAPEDecompress::FillFrameBuffer()
{
    int nRetVal = ERROR_SUCCESS;

    // determine the maximum blocks we can decode
    // note that we won't do end capping because we can't use data
    // until EndFrame(...) successfully handles the frame
    // that means we may decode a little extra in end capping cases
    // but this allows robust error handling of bad frames

    // loop and decode data
    int nBlocksLeft = m_cbFrameBuffer.MaxAdd() / m_nBlockAlign;
    while (nBlocksLeft > 0)
    {
        // output silence from previous error
        if (m_nErrorDecodingCurrentFrameOutputSilenceBlocks > 0)
        {
            // output silence
            int nOutputSilenceBlocks = (std::min<int>)(m_nErrorDecodingCurrentFrameOutputSilenceBlocks, nBlocksLeft);
            unsigned char cSilence = (GetInfo(APE_INFO_BITS_PER_SAMPLE) == 8) ? 127 : 0;
            for (int z = 0; z < nOutputSilenceBlocks * m_nBlockAlign; z++)
            {
                *m_cbFrameBuffer.GetDirectWritePointer() = cSilence;
                m_cbFrameBuffer.UpdateAfterDirectWrite(1);
            }

            // decrement
            m_nErrorDecodingCurrentFrameOutputSilenceBlocks -= nOutputSilenceBlocks;
            nBlocksLeft -= nOutputSilenceBlocks;
            m_nFrameBufferFinishedBlocks += nOutputSilenceBlocks;
            m_nCurrentFrameBufferBlock += nOutputSilenceBlocks;
            if (nBlocksLeft <= 0)
                break;
        }

        // get frame size
        int nFrameBlocks = GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame);
        if (nFrameBlocks < 0)
            break;

        // analyze
        int nFrameOffsetBlocks = m_nCurrentFrameBufferBlock % GetInfo(APE_INFO_BLOCKS_PER_FRAME);
        int nFrameBlocksLeft = nFrameBlocks - nFrameOffsetBlocks;
        int nBlocksThisPass = (std::min<int>)(nFrameBlocksLeft, nBlocksLeft);

        // start the frame if we need to
        if (nFrameOffsetBlocks == 0)
            StartFrame();

        // decode data
        DecodeBlocksToFrameBuffer(nBlocksThisPass);
            
        // end the frame if we decoded all the blocks from the current frame
        BOOL bEndedFrame = FALSE;
        if ((nFrameOffsetBlocks + nBlocksThisPass) >= nFrameBlocks)
        {
            EndFrame();
            bEndedFrame = TRUE;
        }

        // handle errors (either mid-frame or from a CRC at the end of the frame)
        if (m_bErrorDecodingCurrentFrame)
        {
            int nFrameBlocksDecoded = 0;
            if (bEndedFrame)
            {   
                // remove the frame buffer blocks that have been marked as good
                m_nFrameBufferFinishedBlocks -= GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame - 1);
                
                // assume that the frame buffer contains the correct number of blocks for the entire frame
                nFrameBlocksDecoded = GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame - 1);
            }
            else
            {
                // move to the next frame
                m_nCurrentFrame++;

                // calculate how many blocks were output before we errored
                nFrameBlocksDecoded = m_nCurrentFrameBufferBlock - (GetInfo(APE_INFO_BLOCKS_PER_FRAME) * (m_nCurrentFrame - 1));
            }

            // remove any decoded data for this frame from the buffer
            int nFrameBytesDecoded = nFrameBlocksDecoded * m_nBlockAlign;
            m_cbFrameBuffer.RemoveTail(nFrameBytesDecoded);

            // seek to try to synchronize after an error
            if (m_nCurrentFrame < GetInfo(APE_INFO_TOTAL_FRAMES))
                SeekToFrame(m_nCurrentFrame);

            // reset our frame buffer position to the beginning of the frame
            m_nCurrentFrameBufferBlock = (m_nCurrentFrame - 1) * GetInfo(APE_INFO_BLOCKS_PER_FRAME);

            // output silence for the duration of the error frame (we can't just dump it to the
            // frame buffer here since the frame buffer may not be large enough to hold the
            // duration of the entire frame)
            m_nErrorDecodingCurrentFrameOutputSilenceBlocks += nFrameBlocks;

            // save the return value
            nRetVal = ERROR_INVALID_CHECKSUM;
        }

        // update the number of blocks that still fit in the buffer
        nBlocksLeft = m_cbFrameBuffer.MaxAdd() / m_nBlockAlign;
    }

    return nRetVal;
}

void CAPEDecompress::DecodeBlocksToFrameBuffer(int nBlocks)
{
    // decode the samples
    int nBlocksProcessed = 0;
    int nFrameBufferBytes = m_cbFrameBuffer.MaxGet();

    try
    {
        if (m_wfeInput.nChannels == 2)
        {
            if ((m_nSpecialCodes & SPECIAL_FRAME_LEFT_SILENCE) && 
                (m_nSpecialCodes & SPECIAL_FRAME_RIGHT_SILENCE)) 
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    m_Prepare.Unprepare(0, 0, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer(), &m_nCRC);
                    m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                }
            }
            else if (m_nSpecialCodes & SPECIAL_FRAME_PSEUDO_STEREO)
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    int X = m_spNewPredictorX->DecompressValue(m_spUnBitArray->DecodeValueRange(m_BitArrayStateX));
                    m_Prepare.Unprepare(X, 0, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer(), &m_nCRC);
                    m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                }
            }    
            else
            {
                if (m_spAPEInfo->GetInfo(APE_INFO_FILE_VERSION) >= 3950)
                {
                    for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                    {
                        int nY = m_spUnBitArray->DecodeValueRange(m_BitArrayStateY);
                        int nX = m_spUnBitArray->DecodeValueRange(m_BitArrayStateX);
                        int Y = m_spNewPredictorY->DecompressValue(nY, m_nLastX);
                        int X = m_spNewPredictorX->DecompressValue(nX, Y);
                        m_nLastX = X;

                        m_Prepare.Unprepare(X, Y, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer(), &m_nCRC);
                        m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                    }
                }
                else
                {
                    for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                    {
                        int X = m_spNewPredictorX->DecompressValue(m_spUnBitArray->DecodeValueRange(m_BitArrayStateX));
                        int Y = m_spNewPredictorY->DecompressValue(m_spUnBitArray->DecodeValueRange(m_BitArrayStateY));
                        
                        m_Prepare.Unprepare(X, Y, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer(), &m_nCRC);
                        m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                    }
                }
            }
        }
        else
        {
            if (m_nSpecialCodes & SPECIAL_FRAME_MONO_SILENCE)
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    m_Prepare.Unprepare(0, 0, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer(), &m_nCRC);
                    m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                }
            }
            else
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    int X = m_spNewPredictorX->DecompressValue(m_spUnBitArray->DecodeValueRange(m_BitArrayStateX));
                    m_Prepare.Unprepare(X, 0, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer(), &m_nCRC);
                    m_cbFrameBuffer.UpdateAfterDirectWrite(m_nBlockAlign);
                }
            }
        }
    }
    catch(...)
    {
        m_bErrorDecodingCurrentFrame = TRUE;
    }

    // get actual blocks that have been decoded and added to the frame buffer
    int nActualBlocks = (m_cbFrameBuffer.MaxGet() - nFrameBufferBytes) / m_nBlockAlign;
    if (nBlocks != nActualBlocks)
        m_bErrorDecodingCurrentFrame = TRUE;

    // bump frame decode position
    m_nCurrentFrameBufferBlock += nActualBlocks;
}

void CAPEDecompress::StartFrame()
{
    m_nCRC = 0xFFFFFFFF;
    
    // get the frame header
    m_nStoredCRC = m_spUnBitArray->DecodeValue(DECODE_VALUE_METHOD_UNSIGNED_INT);
    m_bErrorDecodingCurrentFrame = FALSE;
    m_nErrorDecodingCurrentFrameOutputSilenceBlocks = 0;

    // get any 'special' codes if the file uses them (for silence, FALSE stereo, etc.)
    m_nSpecialCodes = 0;
    if (GET_USES_SPECIAL_FRAMES(m_spAPEInfo))
    {
        if (m_nStoredCRC & 0x80000000) 
        {
            m_nSpecialCodes = m_spUnBitArray->DecodeValue(DECODE_VALUE_METHOD_UNSIGNED_INT);
        }
        m_nStoredCRC &= 0x7FFFFFFF;
    }

    m_spNewPredictorX->Flush();
    m_spNewPredictorY->Flush();

    m_spUnBitArray->FlushState(m_BitArrayStateX);
    m_spUnBitArray->FlushState(m_BitArrayStateY);

    m_spUnBitArray->FlushBitArray();

    m_nLastX = 0;
}

void CAPEDecompress::EndFrame()
{
    m_nFrameBufferFinishedBlocks += GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame);
    m_nCurrentFrame++;

    // finalize
    m_spUnBitArray->Finalize();

    // check the CRC
    m_nCRC = m_nCRC ^ 0xFFFFFFFF;
    m_nCRC >>= 1;
    if (m_nCRC != m_nStoredCRC)
    {
        // error
        m_bErrorDecodingCurrentFrame = TRUE;

        // We didn't use to check the CRC of the last frame in MAC 3.98 and earlier.  This caused some confusion for one
        // user that had a lot of 3.97 Extra High files that have CRC errors on the last frame.  They would verify
        // with old versions, but not with newer versions.  It's still unknown what corrupted the user's files but since
        // only the last frame was bad, it's likely to have been caused by a buggy tagger.
        //if ((m_nCurrentFrame >= GetInfo(APE_INFO_TOTAL_FRAMES)) && (GetInfo(APE_INFO_FILE_VERSION) < 3990))
        //    m_bErrorDecodingCurrentFrame = FALSE;
    }
}

/*****************************************************************************************
Seek to the proper frame (if necessary) and do any alignment of the bit array
*****************************************************************************************/
int CAPEDecompress::SeekToFrame(int nFrameIndex)
{
    int nSeekRemainder = (GetInfo(APE_INFO_SEEK_BYTE, nFrameIndex) - GetInfo(APE_INFO_SEEK_BYTE, 0)) % 4;
    return m_spUnBitArray->FillAndResetBitArray(GetInfo(APE_INFO_SEEK_BYTE, nFrameIndex) - nSeekRemainder, nSeekRemainder * 8);
}

/*****************************************************************************************
Get information from the decompressor
*****************************************************************************************/
intptr_t CAPEDecompress::GetInfo(APE_DECOMPRESS_FIELDS Field, intptr_t nParam1, intptr_t nParam2)
{
    intptr_t nRetVal = 0;
    BOOL bHandled = TRUE;

    switch (Field)
    {
    case APE_DECOMPRESS_CURRENT_BLOCK:
        nRetVal = m_nCurrentBlock - m_nStartBlock;
        break;
    case APE_DECOMPRESS_CURRENT_MS:
    {
        int nSampleRate = m_spAPEInfo->GetInfo(APE_INFO_SAMPLE_RATE, 0, 0);
        if (nSampleRate > 0)
            nRetVal = intptr_t((double(m_nCurrentBlock) * double(1000)) / double(nSampleRate));
        break;
    }
    case APE_DECOMPRESS_TOTAL_BLOCKS:
        nRetVal = m_nFinishBlock - m_nStartBlock;
        break;
    case APE_DECOMPRESS_LENGTH_MS:
    {
        int nSampleRate = m_spAPEInfo->GetInfo(APE_INFO_SAMPLE_RATE, 0, 0);
        if (nSampleRate > 0)
            nRetVal = intptr_t((double(m_nFinishBlock - m_nStartBlock) * double(1000)) / double(nSampleRate));
        break;
    }
    case APE_DECOMPRESS_CURRENT_BITRATE:
        nRetVal = GetInfo(APE_INFO_FRAME_BITRATE, m_nCurrentFrame);
        break;
    case APE_DECOMPRESS_AVERAGE_BITRATE:
    {
        if (m_bIsRanged)
        {
            // figure the frame range
            const int nBlocksPerFrame = GetInfo(APE_INFO_BLOCKS_PER_FRAME);
            int nStartFrame = m_nStartBlock / nBlocksPerFrame;
            int nFinishFrame = (m_nFinishBlock + nBlocksPerFrame - 1) / nBlocksPerFrame;

            // get the number of bytes in the first and last frame
            int nTotalBytes = (GetInfo(APE_INFO_FRAME_BYTES, nStartFrame) * (m_nStartBlock % nBlocksPerFrame)) / nBlocksPerFrame;
            if (nFinishFrame != nStartFrame)
                nTotalBytes += (GetInfo(APE_INFO_FRAME_BYTES, nFinishFrame) * (m_nFinishBlock % nBlocksPerFrame)) / nBlocksPerFrame;

            // get the number of bytes in between
            const int nTotalFrames = GetInfo(APE_INFO_TOTAL_FRAMES);
            for (int nFrame = nStartFrame + 1; (nFrame < nFinishFrame) && (nFrame < nTotalFrames); nFrame++)
                nTotalBytes += GetInfo(APE_INFO_FRAME_BYTES, nFrame);

            // figure the bitrate
            int nTotalMS = intptr_t((double(m_nFinishBlock - m_nStartBlock) * double(1000)) / double(GetInfo(APE_INFO_SAMPLE_RATE)));
            if (nTotalMS != 0)
                nRetVal = (nTotalBytes * 8) / nTotalMS;
        }
        else
        {
            nRetVal = GetInfo(APE_INFO_AVERAGE_BITRATE);
        }

        break;
    }
    default:
        bHandled = FALSE;
    }

    if (!bHandled && m_bIsRanged)
    {
        bHandled = TRUE;

        switch (Field)
        {
        case APE_INFO_WAV_HEADER_BYTES:
            nRetVal = sizeof(WAVE_HEADER);
            break;
        case APE_INFO_WAV_HEADER_DATA:
        {
            char * pBuffer = (char *) nParam1;
            int nMaxBytes = nParam2;
            
            if (sizeof(WAVE_HEADER) > (unsigned int)nMaxBytes)
            {
                nRetVal = -1;
            }
            else
            {
                WAVEFORMATEX wfeFormat; GetInfo(APE_INFO_WAVEFORMATEX, (intptr_t) &wfeFormat, 0);
                WAVE_HEADER WAVHeader; FillWaveHeader(&WAVHeader, 
                    (m_nFinishBlock - m_nStartBlock) * GetInfo(APE_INFO_BLOCK_ALIGN), 
                    &wfeFormat,    0);
                memcpy(pBuffer, &WAVHeader, sizeof(WAVE_HEADER));
                nRetVal = 0;
            }
            break;
        }
        case APE_INFO_WAV_TERMINATING_BYTES:
            nRetVal = 0;
            break;
        case APE_INFO_WAV_TERMINATING_DATA:
            nRetVal = 0;
            break;
        default:
            bHandled = FALSE;
        }
    }

    if (bHandled == FALSE)
        nRetVal = m_spAPEInfo->GetInfo(Field, nParam1, nParam2);

    return nRetVal;
}
