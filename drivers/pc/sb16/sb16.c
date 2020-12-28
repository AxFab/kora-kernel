/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#include <kernel/arch.h>
// https://wiki.osdev.org/Sound_Blaster_16

#define SB16_DSP_MIXER_PORT  0x224
#define SB16_DSP_MIXERDATA_PORT  0x225
#define SB16_DSP_RESET_PORT  0x226
#define SB16_DSP_READ_PORT  0x22A
#define SB16_DSP_WRITE_PORT  0x22C
#define SB16_DSP_STATUS_PORT  0x22E
#define SB16_DSP_IRQ_ACK_PORT  0x22F


#define SB16_CMD_TIMING 0x40
#define SB16_CMD_SAMPLERATE 0x41
#define SB16_CMD_SPEAKERON 0xD1
#define SB16_CMD_SPEAKEROFF 0xD3
#define SB16_CMD_STOP_8BIT 0xD0
#define SB16_CMD_START_8BIT 0xD4
#define SB16_CMD_STOP_16BIT 0xD5
#define SB16_CMD_START_16BIT 0xD6
#define SB16_CMD_VERSION 0xE1

#define DSP_TRSFT_8BIT  0xC0
#define DSP_TRSFT_16BIT  0xB0
#define DSP_TRSFT_RECORD  0x08
#define DSP_TRSFT_FIFO  0x02

#define DSP_TDATA_MONO  0x00
#define DSP_TDATA_STEREO  0x20
#define DSP_TDATA_UNSIGNED  0x00
#define DSP_TDATA_SIGNED  0x10

void sb16_speaker_on()
{
    outb(SB16_DSP_WRITE_PORT, SB16_CMD_SPEAKERON);
}

void sb16_play(void *buf, size_t len)
{
    if (1) {
        // 00001010

        // Program ISA DMA to transfer
        // DMA channel 1
        outb(0x0A, 1 | 4); // Disable channel 1
        outb(0x0C, 1); // Flip-flop (any value e.g. 1)
        outb(0x0B, 0x49); // Transfert mode (0x48 for single mode/0x58 for auto mode + channel number)
        size_t phys = mmu_read(buf); // DMA !
        outb(0x83, (phys >> 16) & 0xFF);
        outb(0x02, (phys >> 8) & 0xFF);
        outb(0x02, phys & 0xFF);
        outb(0x03, (len >> 8) & 0xFF);
        outb(0x03, len & 0xFF);
        outb(0x0A, 1); // Ènable channel 1
    } else {
        // 11010100

        outb(0xD4, 1 | 4); // Disable channel 5
        outb(0xD8, 1); // Flip-flop (any value e.g. 1)
        outb(0xD6, 0x49); // Transfert mode (0x48 for single mode/0x58 for auto mode + channel number)
        size_t phys = mmu_read(buf); // DMA !
        outb(0x8B, (phys >> 16) & 0xFF);
        outb(0xC4, (phys >> 8) & 0xFF);
        outb(0xC4, phys & 0xFF);
        outb(0xC6, (len >> 8) & 0xFF);
        outb(0xC6, len & 0xFF);
        outb(0xD4, 1); // Ènable channel 5
    }

    // Program sound blaster 16
    outb(SB16_DSP_WRITE_PORT, 0x40); // Set time constant
    outb(SB16_DSP_WRITE_PORT, 165); // Sample rate 10989 Hz
    // Transfer mode
    outb(SB16_DSP_WRITE_PORT, DSP_TRSFT_8BIT);
    // Type of sound data
    outb(SB16_DSP_WRITE_PORT, DSP_TDATA_MONO | DSP_TDATA_UNSIGNED);
    outb(SB16_DSP_WRITE_PORT, ((len - 1) >> 8) & 0xFF);
    outb(SB16_DSP_WRITE_PORT, (len - 1) & 0xFF);
}

void sb16_setup()
{
    // Reset DSP
    outb(SB16_DSP_RESET_PORT, 1);
    sleep_timer(3); // 3 µs
    outb(SB16_DSP_RESET_PORT, 0);
    int8_t value = inb(SB16_DSP_READ_PORT);
    if (value != 0xAA)
        return;

    // PLAYING SOUND
    // Load sound data to memory (DMA memory)
    // Set master volume
    // sb16_speaker_on(); // Turn speaker on
    // sb16_play(NULL, 0); // Play sound data
    // now transfer start - dont forget to handle irq
}


void sb16_teardown()
{
}

EXPORT_MODULE(sb16, sb16_setup, sb16_teardown);

