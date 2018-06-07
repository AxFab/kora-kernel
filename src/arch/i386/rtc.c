/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#include <kernel/core.h>
#include <kernel/cpu.h>

#define CMOS_CMD 0x70
#define CMOS_DATA 0x71

#define BCD2DEC(v) (((v)&0xf)+(((v)>>4)&0xf)*10))

static inline uint8_t cmos_rd(uint8_t idx)
{
	outb(CMOS_CMD, idx);
	return inb(CMOS_DATA);
}

static inline void cmos_wr(uint8_t idx, uint8_t val)
{
	outb(CMOS_CMD, idx);
	outb(CMOS_DATA, val);
}

time_t rtc_time()
{

}


