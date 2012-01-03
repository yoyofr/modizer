// dc_hw.c - Hardware found on the ARM7/AICA side of the Dreamcast

#include "ao.h"
#include "dc_hw.h"
#include "aica.h"

#define DK_CORE	(1)

#if DK_CORE
#include "arm7.h"
#else
#include "arm7core.h"
#endif

uint8 dc_ram[8*1024*1024];

static void aica_irq(int irq)
{
	if (irq > 0)
	{
		#if DK_CORE
		ARM7_SetFIQ(TRUE);
		#else
		set_irq_line(ARM7_FIRQ_LINE, 1);
		#endif
	}
	else
	{
		#if DK_CORE
		ARM7_SetFIQ(FALSE);
		#else
		set_irq_line(ARM7_FIRQ_LINE, 0);
		#endif
	}
}

#define MIXER_PAN_LEFT    1
#define MIXER_PAN_RIGHT   2
#define MIXER(level,pan) ((level & 0xff) | ((pan & 0x03) << 8))
#define YM3012_VOL(LVol,LPan,RVol,RPan) (MIXER(LVol,LPan)|(MIXER(RVol,RPan) << 16))

static struct AICAinterface aica_interface =
{
	1,
	{ dc_ram, },
	{ YM3012_VOL(100, MIXER_PAN_LEFT, 100, MIXER_PAN_RIGHT) },
	{ aica_irq, },
};

uint8 dc_read8(int addr)
{
	if (addr < 0x800000)
	{
		return dc_ram[addr];
	}

	if ((addr >= 0x800000) && (addr <= 0x807fff))
	{
		int foo = AICA_0_r((addr-0x800000)/2, 0);

		if (addr & 1)
		{
			return foo>>8;
		}
		else
		{
			return foo & 0xff;
		}
	}

	printf("R8 @ %x\n", addr);
	return -1;
}

uint16 dc_read16(int addr)
{
	if (addr < 0x800000)
	{
		return dc_ram[addr] | (dc_ram[addr+1]<<8);
	}

	if ((addr >= 0x800000) && (addr <= 0x807fff))
	{
		return AICA_0_r((addr-0x800000)/2, 0);
	}

	printf("R16 @ %x\n", addr);
	return -1;
}

uint32 dc_read32(int addr)
{
	if (addr < 0x800000)
	{
		return dc_ram[addr] | (dc_ram[addr+1]<<8) | (dc_ram[addr+2]<<16) | (dc_ram[addr+3]<<24);
	}

	if ((addr >= 0x800000) && (addr <= 0x807fff))
	{
		addr &= 0x7fff;
		return AICA_0_r(addr/2, 0) & 0xffff;
	}

//	printf("R32 @ %x\n", addr);
	return 0;
}

void dc_write8(int addr, uint8 data)
{
	if (addr < 0x800000)
	{
		dc_ram[addr] = data;
		return;
	}

	if ((addr >= 0x800000) && (addr <= 0x807fff))
	{
		addr -= 0x800000;
		if ((addr & 1))
			AICA_0_w(addr>>1, data<<8, 0x00ff);
		else
			AICA_0_w(addr>>1, data, 0xff00);
		return;
	}

	printf("W8 %x @ %x\n", data, addr);
}

void dc_write16(int addr, uint16 data)
{
	if (addr < 0x800000)
	{
		dc_ram[addr] = data&0xff;
		dc_ram[addr+1] = (data>>8) & 0xff;
		return;
	}

	if ((addr >= 0x800000) && (addr <= 0x807fff))
	{
		AICA_0_w((addr-0x800000)/2, data, 0);
		return;
	}

	printf("W16 %x @ %x\n", data, addr);
}

void dc_write32(int addr, uint32 data)
{
	if (addr < 0x800000)
	{
		dc_ram[addr] = data&0xff;
		dc_ram[addr+1] = (data>>8) & 0xff;
		dc_ram[addr+2] = (data>>16) & 0xff;
		dc_ram[addr+3] = (data>>24) & 0xff;
		return;
	}

	if ((addr >= 0x800000) && (addr <= 0x807fff))
	{
		addr -= 0x800000;
		AICA_0_w((addr>>1), data&0xffff, 0x0000);
		AICA_0_w((addr>>1)+1, data>>16, 0x0000);
		return;
	}

	printf("W32 %x @ %x\n", data, addr);
}

void dc_hw_init(void)
{
	aica_interface.region[0] = dc_ram;
	aica_start(&aica_interface);
}

