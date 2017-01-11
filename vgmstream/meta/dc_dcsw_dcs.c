#include "meta.h"
#include "../util.h"

/* WAV+DCS (DCSW+DCS)
2008-12-06 - manakoAT : Evil Twin - Cypriens Chronicles...
2008-12-07 - manakoAT : Added a function to read the Header file and for 
                        retrieving the channels/frequency, Frequency starts
                        always at a "data" chunk - 0x0C bytes, Channels
                        always - 0x0E bytes...
2010-01-13 - manakoAT : Changed the 'Helper' extension from .wav to .dcws, to prevent conflicts */

VGMSTREAM * init_vgmstream_dc_dcsw_dcs(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    STREAMFILE * streamFileDCSW = NULL;
    char filename[260];
    char filenameDCSW[260];
    int i;
    int channel_count;
    int loop_flag;
    int frequency;
    int dataBuffer = 0;
    int Founddata = 0;
    size_t file_size;
    off_t current_chunk;
    
    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("dcs",filename_extension(filename))) goto fail;

    /* Getting the Header file name... */
    strcpy(filenameDCSW,filename);
    strcpy(filenameDCSW+strlen(filenameDCSW)-3,"dcsw");
    
    /* Look if the Header file is present, else cancel vgmstream */
    streamFileDCSW = streamFile->open(streamFile,filenameDCSW,STREAMFILE_DEFAULT_BUFFER_SIZE);
    if (!streamFileDCSW) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFileDCSW) != 0x52494646 || /* "RIFF" */
        read_32bitBE(0x08,streamFileDCSW) != 0x57415645 || /* "WAVE" */
        read_32bitBE(0x0C,streamFileDCSW) != 0x34582E76 || /* 0x34582E76 */
        read_32bitBE(0x3C,streamFileDCSW) != 0x406E616D) /* "@nam" */
    goto fail;

    /* scan file until we find a "data" string */
    file_size = get_streamfile_size(streamFileDCSW);
    {
        current_chunk = 0;
        /* Start at 0 and loop until we reached the
        file size, or until we found a "data string */
        while (!Founddata && current_chunk < file_size) {
        dataBuffer = (read_32bitBE(current_chunk,streamFileDCSW));
            if (dataBuffer == 0x64617461) { /* "data" */
                /* if "data" string found, retrieve the needed infos */
                Founddata = 1;
                /* We will cancel the search here if we have a match */
            break;
            }
            /* else we will increase the search offset by 1 */
            current_chunk = current_chunk + 1;
        }
    }
    
    if (Founddata == 0) {
        goto fail;
    } else if (Founddata == 1) {
        channel_count = (uint16_t)read_16bitLE(current_chunk-0x0E,streamFileDCSW);
        frequency = read_32bitLE(current_chunk-0x0C,streamFileDCSW);
    }
    
    loop_flag = 0;

    /* Seems we're dealing with a vaild file+header,
    now we can finally build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->channels = channel_count;
    vgmstream->sample_rate = frequency;
    vgmstream->num_samples=(get_streamfile_size(streamFile))*2/channel_count;
    
    if(loop_flag) {
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = (get_streamfile_size(streamFile))*2/channel_count;
    }	

    if (channel_count == 1) {
        vgmstream->layout_type = layout_none;
    } else if (channel_count > 1) {
        vgmstream->layout_type = layout_interleave;
        vgmstream->interleave_block_size = 0x4000;
    }

    vgmstream->coding_type = coding_AICA;
    vgmstream->meta_type = meta_DC_DCSW_DCS;
    
    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,vgmstream->interleave_block_size);
            
        if (!vgmstream->ch[i].streamfile) goto fail;
			vgmstream->ch[i].channel_start_offset=
            vgmstream->ch[i].offset=i*vgmstream->interleave_block_size;
            vgmstream->ch[i].adpcm_step_index = 0x7f;   /* AICA */
		}
    }

    close_streamfile(streamFileDCSW); streamFileDCSW=NULL;
    
    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (streamFileDCSW) close_streamfile(streamFileDCSW);
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
