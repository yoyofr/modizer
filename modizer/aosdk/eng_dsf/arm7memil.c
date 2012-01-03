// memory inline functions shared by ARM and THUMB modes

static INLINE void arm7_write_32(UINT32 addr, UINT32 data )
{
	addr &= ~3;
	dc_write32(addr,data);
}


static INLINE void arm7_write_16(UINT32 addr, UINT16 data)
{
	addr &= ~1;
	dc_write16(addr,data);
}

static INLINE void arm7_write_8(UINT32 addr, UINT8 data)
{
	dc_write8(addr,data);
}

static INLINE UINT32 arm7_read_32(UINT32 addr)
{
    UINT32 result;
    int k = (addr & 3) << 3;

    if (k)
    {
        result = dc_read32 (addr & ~3);
        result = (result >> k) | (result << (32 - k));
    }
    else
    {
    	result = dc_read32(addr);
    }
    return result;
}

static INLINE UINT16 arm7_read_16(UINT32 addr)
{
	UINT16 result;

	result = dc_read16(addr & ~1);

	if (addr & 1)
	{
		result = ((result>>8) & 0xff) | ((result&0xff)<<8);
	}

	return result;
}

static INLINE UINT8 arm7_read_8(UINT32 addr)
{
    return dc_read8(addr);
}

