/**
*
* @file
*
* @brief  Vorbis subsystem API gate interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//platform-specific includes
#include <vorbis/codec.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Sound
{
  namespace Vorbis
  {
    class Api
    {
    public:
      typedef boost::shared_ptr<Api> Ptr;
      virtual ~Api() {}

      
      virtual int vorbis_block_clear(vorbis_block *vb) = 0;
      virtual int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb) = 0;
      virtual void vorbis_dsp_clear(vorbis_dsp_state *v) = 0;
      virtual void vorbis_info_clear(vorbis_info *vi) = 0;
      virtual void vorbis_info_init(vorbis_info *vi) = 0;
      virtual const char *vorbis_version_string(void) = 0;
      virtual int vorbis_analysis(vorbis_block *vb, ogg_packet *op) = 0;
      virtual int vorbis_analysis_blockout(vorbis_dsp_state *v, vorbis_block *vb) = 0;
      virtual float** vorbis_analysis_buffer(vorbis_dsp_state *v, int vals) = 0;
      virtual int vorbis_analysis_headerout(vorbis_dsp_state *v, vorbis_comment *vc, ogg_packet *op, ogg_packet *op_comm, ogg_packet *op_code) = 0;
      virtual int vorbis_analysis_init(vorbis_dsp_state *v, vorbis_info *vi) = 0;
      virtual int vorbis_analysis_wrote(vorbis_dsp_state *v,int vals) = 0;
      virtual int vorbis_bitrate_addblock(vorbis_block *vb) = 0;
      virtual int vorbis_bitrate_flushpacket(vorbis_dsp_state *vd, ogg_packet *op) = 0;
      virtual void vorbis_comment_add_tag(vorbis_comment *vc, const char *tag, const char *contents) = 0;
      virtual void vorbis_comment_clear(vorbis_comment *vc) = 0;
      virtual void vorbis_comment_init(vorbis_comment *vc) = 0;
    };

    //throw exception in case of error
    Api::Ptr LoadDynamicApi();

  }
}
