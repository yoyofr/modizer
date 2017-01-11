#include "meta.h"
#include "../util.h"

/* KCES (from Dance Dance Revolution) */
VGMSTREAM * init_vgmstream_ps2_kces(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;

    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("kces",filename_extension(filename)) &&
        strcasecmp("vig",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x01006408)
        goto fail;

    loop_flag = (read_32bitLE(0x14,streamFile)!=0); 
    channel_count = read_32bitLE(0x1C,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
    start_offset = read_32bitLE(0x08,streamFile);
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x18,streamFile);
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitLE(0x0C,streamFile)*28/16/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = (read_32bitLE(0x0C,streamFile)-read_32bitLE(0x14,streamFile))*28/16/channel_count;
        vgmstream->loop_end_sample = read_32bitLE(0x0C,streamFile)*28/16/channel_count;
    }


	if(vgmstream->channels==1) {
		vgmstream->layout_type=layout_none;
	} else {
		vgmstream->layout_type = layout_interleave;
		vgmstream->interleave_block_size = read_32bitLE(0x24,streamFile);
	}
    vgmstream->meta_type = meta_PS2_KCES;

    /* open the file for reading */
    {
        int i;
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=start_offset+
                vgmstream->interleave_block_size*i;

        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
