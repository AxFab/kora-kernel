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
#ifndef _KERNEL_CPU_H
#define _KERNEL_CPU_H 1

#include <kernel/types.h>

#define IRQ_ON   ((void)0)
#define IRQ_OFF  ((void)0)


struct regs
{
    uint32_t ss;
};

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


static inline void outl(uint16_t port, uint32_t val)
{
}

static inline uint32_t inl(uint16_t port)
{
}



#endif /* _KERNEL_CPU_H */
