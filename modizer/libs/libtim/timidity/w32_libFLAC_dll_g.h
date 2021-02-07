

#ifndef __libFLAC_dll_g_h__
#define __libFLAC_dll_g_h__

#if defined(LEGACY_FLAC)

#include "w32_libFLAC_dll_i.h"

/***************************************************************
   for header file of global definition
 ***************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif


extern int g_load_libFLAC_dll ( char *path );
extern void g_free_libFLAC_dll ( void );
#ifndef IGNORE_libFLAC_FLAC__StreamEncoderStateString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__StreamEncoderStateString;
#define FLAC__StreamEncoderStateString (g_libFLAC_FLAC__StreamEncoderStateString)
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamEncoderWriteStatusString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__StreamEncoderWriteStatusString;
#define FLAC__StreamEncoderWriteStatusString (g_libFLAC_FLAC__StreamEncoderWriteStatusString)
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderStateString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__StreamDecoderStateString;
#define FLAC__StreamDecoderStateString (g_libFLAC_FLAC__StreamDecoderStateString)
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderReadStatusString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__StreamDecoderReadStatusString;
#define FLAC__StreamDecoderReadStatusString (g_libFLAC_FLAC__StreamDecoderReadStatusString)
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderWriteStatusString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__StreamDecoderWriteStatusString;
#define FLAC__StreamDecoderWriteStatusString (g_libFLAC_FLAC__StreamDecoderWriteStatusString)
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderErrorStatusString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__StreamDecoderErrorStatusString;
#define FLAC__StreamDecoderErrorStatusString (g_libFLAC_FLAC__StreamDecoderErrorStatusString)
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamEncoderStateString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__SeekableStreamEncoderStateString;
#define FLAC__SeekableStreamEncoderStateString (g_libFLAC_FLAC__SeekableStreamEncoderStateString)
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamEncoderSeekStatusString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__SeekableStreamEncoderSeekStatusString;
#define FLAC__SeekableStreamEncoderSeekStatusString (g_libFLAC_FLAC__SeekableStreamEncoderSeekStatusString)
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderStateString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__SeekableStreamDecoderStateString;
#define FLAC__SeekableStreamDecoderStateString (g_libFLAC_FLAC__SeekableStreamDecoderStateString)
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderReadStatusString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__SeekableStreamDecoderReadStatusString;
#define FLAC__SeekableStreamDecoderReadStatusString (g_libFLAC_FLAC__SeekableStreamDecoderReadStatusString)
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderSeekStatusString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__SeekableStreamDecoderSeekStatusString;
#define FLAC__SeekableStreamDecoderSeekStatusString (g_libFLAC_FLAC__SeekableStreamDecoderSeekStatusString)
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderTellStatusString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__SeekableStreamDecoderTellStatusString;
#define FLAC__SeekableStreamDecoderTellStatusString (g_libFLAC_FLAC__SeekableStreamDecoderTellStatusString)
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderLengthStatusString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__SeekableStreamDecoderLengthStatusString;
#define FLAC__SeekableStreamDecoderLengthStatusString (g_libFLAC_FLAC__SeekableStreamDecoderLengthStatusString)
#endif
#ifndef IGNORE_libFLAC_FLAC__Metadata_SimpleIteratorStatusString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__Metadata_SimpleIteratorStatusString;
#define FLAC__Metadata_SimpleIteratorStatusString (g_libFLAC_FLAC__Metadata_SimpleIteratorStatusString)
#endif
#ifndef IGNORE_libFLAC_FLAC__Metadata_ChainStatusString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__Metadata_ChainStatusString;
#define FLAC__Metadata_ChainStatusString (g_libFLAC_FLAC__Metadata_ChainStatusString)
#endif
#ifndef IGNORE_libFLAC_FLAC__VERSION_STRING
  extern FLAC_API const char * * g_libFLAC_FLAC__VERSION_STRING;
#define FLAC__VERSION_STRING (*g_libFLAC_FLAC__VERSION_STRING)
#endif
#ifndef IGNORE_libFLAC_FLAC__VENDOR_STRING
  extern FLAC_API const char * * g_libFLAC_FLAC__VENDOR_STRING;
#define FLAC__VENDOR_STRING (*g_libFLAC_FLAC__VENDOR_STRING)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC_STRING
  extern FLAC_API const FLAC__byte*  g_libFLAC_FLAC__STREAM_SYNC_STRING;
#define FLAC__STREAM_SYNC_STRING (g_libFLAC_FLAC__STREAM_SYNC_STRING)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_SYNC;
#define FLAC__STREAM_SYNC (*g_libFLAC_FLAC__STREAM_SYNC)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_SYNC_LEN;
#define FLAC__STREAM_SYNC_LEN (*g_libFLAC_FLAC__STREAM_SYNC_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__EntropyCodingMethodTypeString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__EntropyCodingMethodTypeString;
#define FLAC__EntropyCodingMethodTypeString (g_libFLAC_FLAC__EntropyCodingMethodTypeString)
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN;
#define FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN (*g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN;
#define FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN (*g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN;
#define FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN (*g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER
  extern FLAC_API const unsigned * g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER;
#define FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER (*g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER)
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_TYPE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__ENTROPY_CODING_METHOD_TYPE_LEN;
#define FLAC__ENTROPY_CODING_METHOD_TYPE_LEN (*g_libFLAC_FLAC__ENTROPY_CODING_METHOD_TYPE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__SubframeTypeString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__SubframeTypeString;
#define FLAC__SubframeTypeString (g_libFLAC_FLAC__SubframeTypeString)
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN;
#define FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN (*g_libFLAC_FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN;
#define FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN (*g_libFLAC_FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_ZERO_PAD_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_ZERO_PAD_LEN;
#define FLAC__SUBFRAME_ZERO_PAD_LEN (*g_libFLAC_FLAC__SUBFRAME_ZERO_PAD_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_TYPE_LEN;
#define FLAC__SUBFRAME_TYPE_LEN (*g_libFLAC_FLAC__SUBFRAME_TYPE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN;
#define FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN (*g_libFLAC_FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK
  extern FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK;
#define FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK (*g_libFLAC_FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK)
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK
  extern FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK;
#define FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK (*g_libFLAC_FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK)
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK
  extern FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK;
#define FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK (*g_libFLAC_FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK)
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK
  extern FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK;
#define FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK (*g_libFLAC_FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK)
#endif
#ifndef IGNORE_libFLAC_FLAC__ChannelAssignmentString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__ChannelAssignmentString;
#define FLAC__ChannelAssignmentString (g_libFLAC_FLAC__ChannelAssignmentString)
#endif
#ifndef IGNORE_libFLAC_FLAC__FrameNumberTypeString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__FrameNumberTypeString;
#define FLAC__FrameNumberTypeString (g_libFLAC_FLAC__FrameNumberTypeString)
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC
  extern FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_SYNC;
#define FLAC__FRAME_HEADER_SYNC (*g_libFLAC_FLAC__FRAME_HEADER_SYNC)
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_SYNC_LEN;
#define FLAC__FRAME_HEADER_SYNC_LEN (*g_libFLAC_FLAC__FRAME_HEADER_SYNC_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_RESERVED_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_RESERVED_LEN;
#define FLAC__FRAME_HEADER_RESERVED_LEN (*g_libFLAC_FLAC__FRAME_HEADER_RESERVED_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_BLOCK_SIZE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_BLOCK_SIZE_LEN;
#define FLAC__FRAME_HEADER_BLOCK_SIZE_LEN (*g_libFLAC_FLAC__FRAME_HEADER_BLOCK_SIZE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SAMPLE_RATE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_SAMPLE_RATE_LEN;
#define FLAC__FRAME_HEADER_SAMPLE_RATE_LEN (*g_libFLAC_FLAC__FRAME_HEADER_SAMPLE_RATE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN;
#define FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN (*g_libFLAC_FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN;
#define FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN (*g_libFLAC_FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_ZERO_PAD_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_ZERO_PAD_LEN;
#define FLAC__FRAME_HEADER_ZERO_PAD_LEN (*g_libFLAC_FLAC__FRAME_HEADER_ZERO_PAD_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_CRC_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_CRC_LEN;
#define FLAC__FRAME_HEADER_CRC_LEN (*g_libFLAC_FLAC__FRAME_HEADER_CRC_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_FOOTER_CRC_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_FOOTER_CRC_LEN;
#define FLAC__FRAME_FOOTER_CRC_LEN (*g_libFLAC_FLAC__FRAME_FOOTER_CRC_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__MetadataTypeString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__MetadataTypeString;
#define FLAC__MetadataTypeString (g_libFLAC_FLAC__MetadataTypeString)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN;
#define FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN (*g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN;
#define FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN (*g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN;
#define FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN (*g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN;
#define FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN (*g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN;
#define FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN (*g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN;
#define FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN (*g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN;
#define FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN (*g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN;
#define FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN (*g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN;
#define FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN (*g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_APPLICATION_ID_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_APPLICATION_ID_LEN;
#define FLAC__STREAM_METADATA_APPLICATION_ID_LEN (*g_libFLAC_FLAC__STREAM_METADATA_APPLICATION_ID_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN;
#define FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN (*g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN;
#define FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN (*g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN;
#define FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN (*g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER
  extern FLAC_API const FLAC__uint64 * g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
#define FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER (*g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN;
#define FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN (*g_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN;
#define FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN (*g_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN;
#define FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN (*g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_IS_LAST_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_IS_LAST_LEN;
#define FLAC__STREAM_METADATA_IS_LAST_LEN (*g_libFLAC_FLAC__STREAM_METADATA_IS_LAST_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_TYPE_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_TYPE_LEN;
#define FLAC__STREAM_METADATA_TYPE_LEN (*g_libFLAC_FLAC__STREAM_METADATA_TYPE_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_LENGTH_LEN
  extern FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_LENGTH_LEN;
#define FLAC__STREAM_METADATA_LENGTH_LEN (*g_libFLAC_FLAC__STREAM_METADATA_LENGTH_LEN)
#endif
#ifndef IGNORE_libFLAC_FLAC__FileEncoderStateString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__FileEncoderStateString;
#define FLAC__FileEncoderStateString (g_libFLAC_FLAC__FileEncoderStateString)
#endif
#ifndef IGNORE_libFLAC_FLAC__FileDecoderStateString
  extern FLAC_API const char * const*  g_libFLAC_FLAC__FileDecoderStateString;
#define FLAC__FileDecoderStateString (g_libFLAC_FLAC__FileDecoderStateString)
#endif

#if defined(__cplusplus)
}  /* extern "C" { */
#endif
/***************************************************************/

	
#else  /* defined(LEGACY_FLAC) */
	
	
	 extern const char * const *  *g_FLAC__StreamEncoderInitStatusString;
	 extern const char * const *  *g_FLAC__StreamEncoderStateString;
	 extern const char * const *  *g_FLAC__StreamDecoderStateString;

	 extern const char * const *  *g_FLAC__StreamDecoderErrorStatusString;
	 extern const char * const *  *g_FLAC__StreamDecoderInitStatusString;
	 extern const char * const *  *g_FLAC__StreamDecoderLengthStatusString;
	 extern const char * const *  *g_FLAC__StreamDecoderReadStatusString;
	 extern const char * const *  *g_FLAC__StreamDecoderSeekStatusString;
	 extern const char * const *  *g_FLAC__StreamDecoderTellStatusString;
	 extern const char * const *  *g_FLAC__StreamDecoderWriteStatusString;
	 extern const char * const *  *g_FLAC__StreamEncoderSeekStatusString;
	 extern const char * const *  *g_FLAC__StreamEncoderTellStatusString;
	 extern const char * const *  *g_FLAC__StreamEncoderWriteStatusString;
	 extern const char * const *  *g_FLAC__StreamEncoderReadStatusString;

	 extern const char * const *  *g_FLAC__ChannelAssignmentString;
	 extern const char * const *  *g_FLAC__EntropyCodingMethodTypeString;
	 extern const char * const *  *g_FLAC__FrameNumberTypeString;
	 extern const char * const *  *g_FLAC__MetadataTypeString;
	 extern const char * const *  *g_FLAC__Metadata_ChainStatusString;
	 extern const char * const *  *g_FLAC__Metadata_SimpleIteratorStatusString;
	 extern const char * const *  *g_FLAC__StreamMetadata_Picture_TypeString;
	 extern const char * const *  *g_FLAC__SubframeTypeString;

#define FLAC__StreamEncoderInitStatusString *g_FLAC__StreamEncoderInitStatusString
#define FLAC__StreamEncoderStateString *g_FLAC__StreamEncoderStateString
#define FLAC__StreamDecoderStateString *g_FLAC__StreamDecoderStateString

#define FLAC__StreamDecoderErrorStatusString *g_FLAC__StreamDecoderErrorStatusString
#define FLAC__StreamDecoderInitStatusString *g_FLAC__StreamDecoderInitStatusString
#define FLAC__StreamDecoderLengthStatusString *g_FLAC__StreamDecoderLengthStatusString
#define FLAC__StreamDecoderReadStatusString *g_FLAC__StreamDecoderReadStatusString
#define FLAC__StreamDecoderSeekStatusString *g_FLAC__StreamDecoderSeekStatusString
#define FLAC__StreamDecoderTellStatusString  *g_FLAC__StreamDecoderTellStatusString 
#define FLAC__StreamDecoderWriteStatusString  *g_FLAC__StreamDecoderWriteStatusString 
#define FLAC__StreamEncoderSeekStatusString *g_FLAC__StreamEncoderSeekStatusString
#define FLAC__StreamEncoderTellStatusString *g_FLAC__StreamEncoderTellStatusString
#define FLAC__StreamEncoderWriteStatusString *g_FLAC__StreamEncoderWriteStatusString
#define FLAC__StreamEncoderReadStatusString *g_FLAC__StreamEncoderReadStatusString

#define FLAC__ChannelAssignmentString *g_FLAC__ChannelAssignmentString
#define FLAC__EntropyCodingMethodTypeString *g_FLAC__EntropyCodingMethodTypeString
#define FLAC__FrameNumberTypeString *g_FLAC__FrameNumberTypeString
#define FLAC__MetadataTypeString *g_FLAC__MetadataTypeString
#define FLAC__Metadata_ChainStatusString  *g_FLAC__Metadata_ChainStatusString 
#define FLAC__Metadata_SimpleIteratorStatusString  *g_FLAC__Metadata_SimpleIteratorStatusString 
#define FLAC__StreamMetadata_Picture_TypeString *g_FLAC__StreamMetadata_Picture_TypeString
#define FLAC__SubframeTypeString *g_FLAC__SubframeTypeString
	
#endif  /* defined(LEGACY_FLAC) */
#endif  /* __libFLAC_dll_g_h__ */

