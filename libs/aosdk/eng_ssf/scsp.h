/*

	SCSP (YMF292-F) header
*/

#ifndef _SCSPAOAO_H_
#define _SCSPAOAO_H_

#define MAX_SCSPAO	(2)

#define COMBINE_DATA(varptr)	(*(varptr) = (*(varptr) & mem_mask) | (data & ~mem_mask))

// convert AO types
typedef int8 data8_t;
typedef int16 data16_t;
typedef int32 data32_t;
typedef int offs_t;

struct SCSPinterface 
{
	int num;
	void *region[MAX_SCSPAO];
	int mixing_level[MAX_SCSPAO];			/* volume */
	void (*irq_callback[MAX_SCSPAO])(int state);	/* irq callback */
};

void *scsp_start(const void *config);
void SCSPAO_Update(void *param, INT16 **inputs, INT16 **buf, int samples);

#define READ16_HANDLER(name)	data16_t name(offs_t offset, data16_t mem_mask)
#define WRITE16_HANDLER(name)	void     name(offs_t offset, data16_t data, data16_t mem_mask)

// SCSP register access
READ16_HANDLER( SCSPAO_0_r );
WRITE16_HANDLER( SCSPAO_0_w );
READ16_HANDLER( SCSPAO_1_r );
WRITE16_HANDLER( SCSPAO_1_w );

// MIDI I/O access (used for comms on Model 2/3)
WRITE16_HANDLER( SCSPAO_MidiIn );
READ16_HANDLER( SCSPAO_MidiOutR );

#endif
