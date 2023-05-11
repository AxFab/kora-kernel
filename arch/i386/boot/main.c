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
#include <stddef.h>
#include <bits/atomic.h>
#include <kernel/core.h>
#include <kernel/stdc.h>
#include <kernel/irq.h>
#include <kernel/mods.h>
#include <string.h>
#include <stdbool.h>

int cpu_no();

void mmu_setup(sys_info_t *sysinfo);
void acpi_setup(sys_info_t *sysinfo);
void cpuid_setup(sys_info_t *sysinfo);
void apic_setup(sys_info_t *sysinfo);
void tss_setup(sys_info_t *sysinfo);

void mboot_modules();
void pci_setup();
// void csl_setup();
void serial_setup();
void pic_setup();

void clock_handler(void *);


void clwrite(int p, const char *str);
void serial_send(int com, const char *buf, size_t len);


xtime_t rtc_time();

x86_info_t x86info;


void cpu_setup(sys_info_t *sysinfo)
{
    int no = cpu_no();
    if (no != 0) {
        // acpi ?
        cpuid_setup(sysinfo);
        tss_setup(sysinfo);
        return;
    }

    mmu_setup(sysinfo);
    memset(&x86info, 0, sizeof(x86_info_t));
    sysinfo->arch = &x86info;
    acpi_setup(sysinfo);
    cpuid_setup(sysinfo);
    apic_setup(sysinfo);
    pic_setup();
    if (1) {
        // If SMP setup HPET
    } else {
        // Setup PIT/RTC
    }

    tss_setup(sysinfo);
    irq_register(0, (irq_handler_t)clock_handler, NULL);
    sysinfo->uptime = rtc_time();
}

void arch_init()
{
    mboot_modules();
    pci_setup();
    // csl_setup();
    serial_setup();

    // Clock timer
    // pit_init();
}


uint16_t cls_pen = 0x700;
uint16_t *cls = (void*)0xB8000;
int cls_row = 0;
int cls_col = 0;
char cls_clr[] = { 0, 4, 2, 6, 1, 5, 3, 7};

void cls_write(const char *buf)
{
    while (*buf) {
        if (buf[0] == '\033' && buf[1] == '[') {
            const char *rt;
            int val = strtol(&buf[2], &rt, 10);
            buf = rt;
            if (*rt == 'm') {
                if (val == 0)
                    cls_pen = 0x700;
                else if (val >= 31 && val <= 37)
                    cls_pen = (cls_pen & 0xF000) | (cls_clr[val - 30] << 8);
                else if (val >= 91 && val <= 97)
                    cls_pen = (cls_pen & 0xF000) | (cls_clr[val - 90] << 8);
                else if (val >= 40 && val <= 47)
                    cls_pen = (cls_pen & 0x0F00) | (cls_clr[val - 40] << 12);
                buf++;
            }
            continue;
        } else if (buf[0] == '\n') {
            for (; cls_col < 80; ++cls_col)
                cls[cls_row * 80 + cls_col] = cls_pen;
        } else if (buf[0] == '\t') {
            for (int n = ALIGN_UP(cls_col + 1, 8); cls_col < n; ++cls_col)
                cls[cls_row * 80 + cls_col] = cls_pen;
        } else if (buf[0] < ' ') {
            // ...
        } else {
            cls[cls_row * 80 + cls_col] = cls_pen | buf[0];
            cls_col++;
        }

        if (cls_col >= 80) {
            cls_col = 0;
            cls_row++;
            for (int i = 0; i < 80; ++i)
                cls[cls_row * 80 + i] = cls_pen;
            if (cls_row >= 24) {
                cls_row--;
                for (int i = 0; i < 24; ++i)
                    memcpy(&cls[i*80], &cls[(i+1)*80], 80 * 2);
                // cls_row = 0;
            }
        }
        buf++;
    }
}


void kwrite(const char *buf)
{
    // Console
    cls_write(buf);
    // Serial
    size_t len = strlen(buf);
    serial_send(0, buf, len);
}


// libgcc symbols, need to be exported

void __muldi3();
void __divdi3();
void __moddi3();
void __udivdi3();
void __umoddi3();

EXPORT_SYMBOL(__muldi3, 0);
EXPORT_SYMBOL(__divdi3, 0);
EXPORT_SYMBOL(__moddi3, 0);
EXPORT_SYMBOL(__udivdi3, 0);
EXPORT_SYMBOL(__umoddi3, 0);

