#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "gbcpu.h"
#include "mapper.h"

#define MAPPER_ROMBANK_SIZE 0x4000
#define MAPPER_ROMBANK_MASK (MAPPER_ROMBANK_SIZE - 1)
#define MAPPER_MAX_EXTRAM_SIZE 0x8000
#define MAPPER_RAMBANK_SIZE 0x2000
#define MAPPER_RAMBANK_MASK (MAPPER_RAMBANK_SIZE - 1)

struct bank {
	uint8_t *data;
	size_t size;
	uint32_t mask;
	uint32_t banksize;
	long enable;
	struct mapper *mapper;
};

struct mbc1_regs {
	uint8_t reg[4];
};

struct mapper {
	const uint8_t *rom;
	size_t rom_size;
	size_t ram_size;

	struct bank rom_lower;
	struct bank rom_upper;
	struct bank extram;

	struct mbc1_regs mbc1;

	uint8_t ram[MAPPER_MAX_EXTRAM_SIZE];
};

static void bank_init(struct bank *b, struct mapper *m, uint32_t banksize)
{
	b->mapper = m;
	b->banksize = banksize;
	b->mask = banksize - 1;
	b->enable = 1;
}

static struct mapper *mapper_new(const uint8_t *rom, size_t rom_size, size_t ram_size)
{
	struct mapper *m = calloc(sizeof(*m), 1);
	m->rom = rom;
	m->rom_size = rom_size;
	m->ram_size = ram_size;
	bank_init(&m->rom_lower, m, MAPPER_ROMBANK_SIZE);
	bank_init(&m->rom_upper, m, MAPPER_ROMBANK_SIZE);
	bank_init(&m->extram, m, MAPPER_RAMBANK_SIZE);
	m->extram.enable = 0;
	return m;
}

static void mapper_map(struct bank *b, uint8_t *data, size_t size, long bank)
{
	size_t ofs = bank * b->banksize;
	size_t len = size - ofs;
	if (len <= 0) {
		WARN_ONCE("Bank %ld out of range (0-%ld)!\n", bank, size / b->banksize);
		b->data = NULL;
		b->size = 0;
	}
	b->data = data + ofs;
	b->size = size - ofs;
}

static void mapper_map_rom(struct bank *b, long bank)
{
	struct mapper *m = b->mapper;
	mapper_map(b, (uint8_t*)m->rom, m->rom_size, bank);
}

static void mapper_map_ram(struct bank *b, long bank)
{
	struct mapper *m = b->mapper;
	mapper_map(b, m->ram, m->ram_size, bank);
}

static uint32_t bank_get(void *priv, uint32_t addr)
{
	struct bank *b = priv;
	uint32_t maddr = addr & b->mask;
	if (maddr > b->size || b->enable == 0) {
		return 0xff;
	}
	return b->data[maddr];
}

static void bank_put(void *priv, uint32_t addr, uint8_t val)
{
	struct bank *b = priv;
	uint32_t maddr = addr & b->mask;
	if (maddr > b->size || b->enable == 0) {
		return;
	}
	b->data[maddr] = val;
}

static void gbs_rom_put(void *priv, uint32_t addr, uint8_t val)
{
	if (addr >= 0x2000 && addr <= 0x3fff) {
		struct bank *b = priv;
		struct mapper *m = b->mapper;
		mapper_map_rom(&m->rom_upper, val + (val == 0));
	} else {
		WARN_ONCE("rom write of %02x to %04x ignored\n", val, addr);
	}
}

static void mbc1_rom_put(void *priv, uint32_t addr, uint8_t val)
{
	struct bank *b = priv;
	struct mapper *m = b->mapper;
	uint8_t rombank = 0;
	uint8_t rambank = 0;

	/* store reg value */
	m->mbc1.reg[addr / 0x2000] = val;

	/* update mbc1 state */
	m->extram.enable = m->mbc1.reg[0] == 0x0a;
	rombank = m->mbc1.reg[1] & 0x1f;
	rombank += rombank == 0;
	rambank = m->mbc1.reg[2] & 0x03;

	if (m->mbc1.reg[3] == 1) {
		if (m->ram_size > 0x2000) {
			/* RAM banking mode */
			mapper_map_rom(&m->rom_lower, 0);
			mapper_map_rom(&m->rom_upper, rombank);
			mapper_map_ram(&m->extram, rambank);
		} else {
			/* Advanced ROM banking mode */
			rombank |= rambank << 5;
			mapper_map_rom(&m->rom_lower, rambank << 5);
			mapper_map_rom(&m->rom_upper, rombank);
			mapper_map_ram(&m->extram, 0);
		}
	} else {
		/* Simple ROM banking mode */
		rombank |= rambank << 5;
		mapper_map_rom(&m->rom_lower, 0);
		mapper_map_rom(&m->rom_upper, rombank);
		mapper_map_ram(&m->extram, 0);
	}
}

static void mbc3_rom_put(void *priv, uint32_t addr, uint8_t val)
{
	/* TODO: RTC registers not supported */
	struct bank *b = priv;
	struct mapper *m = b->mapper;
	uint8_t rombank = 0;
	uint8_t rambank = 0;

	/* store reg value */
	m->mbc1.reg[addr / 0x2000] = val;

	/* update MBC3 state */
	m->extram.enable = m->mbc1.reg[0] == 0x0a;
	rombank = m->mbc1.reg[1] & 0x7f;
	rombank += rombank == 0;
	rambank = m->mbc1.reg[2] & 0x03;

	mapper_map_rom(&m->rom_lower, 0);
	mapper_map_rom(&m->rom_upper, rombank);
	mapper_map_ram(&m->extram, rambank);
}

struct mapper *mapper_gbs(struct gbcpu *gbcpu, const uint8_t *rom, size_t size) {
	struct mapper *m = mapper_new(rom, size, MAPPER_RAMBANK_SIZE);
	m->extram.enable = 1;
	mapper_map_rom(&m->rom_lower, 0);
	mapper_map_rom(&m->rom_upper, 1);
	mapper_map_ram(&m->extram, 0);
	gbcpu_add_mem(gbcpu, 0x00, 0x3f, gbs_rom_put, bank_get, &m->rom_lower);
	gbcpu_add_mem(gbcpu, 0x40, 0x7f, gbs_rom_put, bank_get, &m->rom_upper);
	gbcpu_add_mem(gbcpu, 0xa0, 0xbf, bank_put, bank_get, &m->extram);
	return m;
}

struct mapper *mapper_gbr(struct gbcpu *gbcpu, const uint8_t *rom, size_t size, uint8_t bank_lower, uint8_t bank_upper) {
	struct mapper *m = mapper_new(rom, size, MAPPER_RAMBANK_SIZE);
	mapper_map_rom(&m->rom_lower, bank_lower);
	mapper_map_rom(&m->rom_upper, bank_upper);
	mapper_map_ram(&m->extram, 0);
	gbcpu_add_mem(gbcpu, 0x00, 0x3f, gbs_rom_put, bank_get, &m->rom_lower);
	gbcpu_add_mem(gbcpu, 0x40, 0x7f, gbs_rom_put, bank_get, &m->rom_upper);
	gbcpu_add_mem(gbcpu, 0xa0, 0xbf, bank_put, bank_get, &m->extram);
	return m;
}

struct mapper *mapper_gb(struct gbcpu *gbcpu, const uint8_t *rom, size_t size, uint8_t cart_type, uint8_t rom_type, uint8_t ram_type) {
	struct mapper *m;
	size_t ram_size = 0;
	gbcpu_put_fn rom_put = mbc1_rom_put;

	switch (cart_type) {
	case 0x00:  /* ROM only */
	case 0x01:  /* MBC1 */
	case 0x02:  /* MBC1+RAM */
	case 0x03:  /* MBC1+RAM+BAT */
	case 0x08:  /* ROM+RAM */
	case 0x09:  /* ROM+RAM+BAT */
		break;
	case 0x11:  /* MBC3 */
	case 0x12:  /* MBC3+RAM */
	case 0x13:  /* MBC3+RAM+BAT (e.g. Pokemon) */
		rom_put = mbc3_rom_put;
		break;
	/* TODO: Implement more mappers */
	default: return NULL;
	}

	switch (ram_type) {
	default: break;  /* No RAM */
	case 0x01: ram_size = 0x00800; break;  /*   2 KiB */
	case 0x02: ram_size = 0x02000; break;  /*   8 KiB */
	case 0x03: ram_size = 0x08000; break;  /*  32 KiB */
	}

	assert(ram_size <= MAPPER_MAX_EXTRAM_SIZE);
	m = mapper_new(rom, size, ram_size);

	mapper_map_rom(&m->rom_lower, 0);
	mapper_map_rom(&m->rom_upper, 1);
	gbcpu_add_mem(gbcpu, 0x00, 0x3f, rom_put, bank_get, &m->rom_lower);
	gbcpu_add_mem(gbcpu, 0x40, 0x7f, rom_put, bank_get, &m->rom_upper);

	if (ram_size > 0) {
		mapper_map_ram(&m->extram, 0);
		gbcpu_add_mem(gbcpu, 0xa0, 0xbf, bank_put, bank_get, &m->extram);
	}

	return m;
}

void mapper_free(struct mapper *m) {
	free(m);
}
