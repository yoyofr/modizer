#include "meta.h"
#include "../util.h"

/* KHV (from Kingdom Hearts 2) */
/* VAG files with custom headers */
VGMSTREAM * init_vgmstream_ps2_khv(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    int loop_flag = 0;
		int channel_count;
    off_t start_offset;
    
    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("khv",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x56414770) /* "VAGp" */
        goto fail;

    loop_flag = (read_32bitBE(0x14,streamFile)!=0);
    channel_count = 2;
    
		/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

		/* fill in the vital statistics */
    start_offset = 0x60;
		vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitBE(0x10,streamFile);
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitBE(0x0C,streamFile);
    if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitBE(0x14,streamFile);
        vgmstream->loop_end_sample = read_32bitBE(0x18,streamFile);
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = 0x10;
    vgmstream->meta_type = meta_PS2_KHV;

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
