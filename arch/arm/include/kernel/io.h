/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#ifndef _KERNEL_IO_H
#define _KERNEL_IO_H 1

#if defined __RASPI
#  define PBASE  0x20000000
#elif defined __RASPI2 || defined __RASPI3
#  define PBASE  0x3F000000
#else
#  error Unable to determine the target platform
#endif

// Auxiliaires: UART1, SPI1 & SPI2
#define AUX_BASE  (PBASE + 0x215000)

#define AUX_IRQ  (AUX_BASE + 0x00)
#define AUX_ENABLES  (AUX_BASE + 0x04)
#define AUX_MU_IO_REG  (AUX_BASE + 0x40)
#define AUX_MU_IER_REG  (AUX_BASE + 0x44)
#define AUX_MU_IIR_REG  (AUX_BASE + 0x48)
#define AUX_MU_LCR_REG  (AUX_BASE + 0x4c)
#define AUX_MU_MCR_REG  (AUX_BASE + 0x50)
#define AUX_MU_LSR_REG  (AUX_BASE + 0x54)
#define AUX_MU_MSR_REG  (AUX_BASE + 0x58)
#define AUX_MU_SCRATCH  (AUX_BASE + 0x5c)
#define AUX_MU_CNTL_REG  (AUX_BASE + 0x60)
#define AUX_MU_STAT_REG  (AUX_BASE + 0x64)
#define AUX_MU_BAUD_REG  (AUX_BASE + 0x68)
#define AUX_SPI0_CNTL0_REG  (AUX_BASE + 0x80)
#define AUX_SPI0_CNTL1_REG  (AUX_BASE + 0x84)
#define AUX_SPI0_STAT_REG  (AUX_BASE + 0x88)
#define AUX_SPI0_IO_REG  (AUX_BASE + 0x90)
#define AUX_SPI0_PEEK_REG  (AUX_BASE + 0x94)
#define AUX_SPI1_CNTL0_REG  (AUX_BASE + 0xc0)
#define AUX_SPI1_CNTL1_REG  (AUX_BASE + 0xc4)
#define AUX_SPI1_STAT_REG  (AUX_BASE + 0xc8)
#define AUX_SPI1_IO_REG  (AUX_BASE + 0xd0)
#define AUX_SPI1_PEEK_REG  (AUX_BASE + 0xd4)

// BSC
#define BSC0_BASE  (PBASE + 0x205000)
#define BSC1_BASE  (PBASE + 0x804000)
#define BSC2_BASE  (PBASE + 0x805000)

// DMA controller
#define DMA0_BASE  (PBASE + 0x07000)
#define DMA15_BASE  (PBASE + 0xe05000)
#define EMMC_BASE  (PBASE + 0x300000)


// UART0
#define UART_BASE  (PBASE + 0x201000)

#define UART_DR  (UART_BASE + 0x00)
#define UART_RSRECR  (UART_BASE + 0x04)

// USB
#define USB_BASE  (PBASE + 0x980000)

#endif  /* _KERNEL_IO_H */
