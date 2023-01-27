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


int s = 4;
void kwrite(const char *buf)
{
    size_t len = strlen(buf);
    clwrite((s % 25) * 80, buf);
    serial_send(0, buf, len);
    s++;
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

