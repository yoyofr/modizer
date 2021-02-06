#include "layout.h"
#include "../vgmstream.h"

/* set up for the block at the given offset */
void ast_block_update(off_t block_offset, VGMSTREAM * vgmstream) {
    int i;
    vgmstream->current_block_offset = block_offset;
    vgmstream->current_block_size = read_32bitBE(
            vgmstream->current_block_offset+4,
            vgmstream->ch[0].streamfile);
    vgmstream->next_block_offset = vgmstream->current_block_offset +
        vgmstream->current_block_size*vgmstream->channels + 0x20;

    for (i=0;i<vgmstream->channels;i++) {
        vgmstream->ch[i].offset = vgmstream->current_block_offset +
            0x20 + vgmstream->current_block_size*i;
    }
}
