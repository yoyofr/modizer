#include "meta.h"
#include "../util.h"

/* CPS (from Eternal Sonata) */
VGMSTREAM * init_vgmstream_ps3_cps(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[PATH_LIMIT];
    off_t start_offset;

    int loop_flag;
   int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("cps",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x43505320) /* "CPS" */
        goto fail;

    loop_flag = read_32bitBE(0x18,streamFile);
    
	channel_count = read_32bitBE(0x8,streamFile);

   /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

   /* fill in the vital statistics */
    start_offset = read_32bitBE(0x4,streamFile);
   vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitBE(0x10,streamFile);
	if (read_32bitBE(0x20,streamFile)==0x00000000){
		vgmstream->coding_type = coding_PCM16BE;
	    vgmstream->num_samples = read_32bitBE(0xc,streamFile)/4;
		vgmstream->interleave_block_size = 2;
	}
	else {
		vgmstream->coding_type = coding_PSX;
	    vgmstream->num_samples = read_32bitBE(0xc,streamFile)*28/32;
		vgmstream->interleave_block_size = 0x10;
	}

    if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitBE(0x14,streamFile)*28/32;
       vgmstream->loop_end_sample = read_32bitBE(0x18,streamFile)*28/32;
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_PS3_CPS;

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
