/*
 * gbsplay is a Gameboy sound player
 *
 * 2020-2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _GBMAPPER_H_
#define _GBMAPPER_H_

#include <inttypes.h>

struct gbcpu;
struct mapper;

struct mapper *mapper_gbs(struct gbcpu *gbcpu, const uint8_t *rom, size_t size);
struct mapper *mapper_gbr(struct gbcpu *gbcpu, const uint8_t *rom, size_t size, uint8_t bank_lower, uint8_t bank_upper);
struct mapper *mapper_gb(struct gbcpu *gbcpu, const uint8_t *rom, size_t size, uint8_t cart_type, uint8_t rom_type, uint8_t ram_type);
void mapper_lockout(struct mapper *m);
void mapper_free(struct mapper *m);

#endif
