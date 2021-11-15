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
#include <kernel/core.h>
#include <kernel/stdc.h>

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800


const char *x86_features[] = {
    "FPU", "VME", "PE", "PSE", "TSC", "MSR", "PAE", "MCE",
    "CX8", "APIC", NULL, "SEP", "MTRR", "PGE", "MCA", "CMOV",
    "PAT", "PSE36", "PSN", "CLF", NULL, "DTES", "ACPI", "MMX",
    "FXSR", "SSE", "SSE2", "SS", "HTT", "TM1", "IA64", "PBE",
    "SSE3", "PCLMUL", "DTES64", "MONITOR", "DS_CPL", "VMX", "SMX", "EST",
    "TM2", "SSSE3", "CID", NULL, "FMA", "CX16", "ETPRD", "PDCM",
    NULL, NULL, "DCA", "SSE4_1", "SSE4_2", "x2APIC", "MOVBE", "POPCNT",
    NULL, "AES", "XSAVE", "OSXSAVE", "AVX", NULL, NULL, NULL,
};

// Read 64bits data from MSR registers
static void rdmsr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

int cpu_feature(x86_cpu_t *cpu, const char *feature, int n)
{
    if (x86_features[n] != NULL && strcmp(x86_features[n], feature) == 0)
        return cpu->features[n / 32] & (1 << (n % 32)) ? 1 : 0;

    for (int i = 0; i < 64; ++i) {
        if (x86_features[i] == NULL || strcmp(x86_features[i], feature) != 0)
            continue;
        return cpu->features[i / 32] & (1 << (i % 32)) ? 1 : 0;
    }
    return 0;
}

void x86_cpuid(int, int, int*);

void cpuid_setup(sys_info_t *sysinfo)
{
    int no = cpu_no();
    x86_cpu_t *cpu = sysinfo->cpu_table[no].arch;

    // Read CPU vendor
    int cpu_res[5];
    x86_cpuid(0, 0, cpu_res);
    cpu_res[4] = 0;
    cpu->vendor = kstrdup((char *)&cpu_res[1]);
    cpu->max_cpuid = cpu_res[0];

    x86_cpuid(1, 0, cpu_res);
    cpu->model = (cpu_res[0] >> 4) & 0xf;
    cpu->family = (cpu_res[0] >> 8) & 0xf;
    // type / family / model / stepping / brand
    cpu->cache_size = (cpu_res[1] >> 5) & 0x7F8;
    cpu->features[0] = cpu_res[2];
    cpu->features[1] = cpu_res[3];

    // Display CPU info
    int lg = 0;
    char tmp[512];
    tmp[0] = '\0';
    for (int i = 0; i < 64; ++i) {
        if (x86_features[i] == NULL)
            continue;
        if (cpu->features[i / 32] & (1 << (i % 32))) {
            if (lg != 0) {
                strcpy(&tmp[lg], ", ");
                lg += 2;
            }
            strcpy(&tmp[lg], x86_features[i]);
            lg += strlen(x86_features[i]);
        }
    }
    kprintf(-1, "CPU(%d) :: %s :: Model: %d, Family: %d\n", no, cpu->vendor, cpu->model, cpu->family);
    kprintf(-1, "CPU(%d) :: %s\n", no, tmp);
    if (no != 0)
        return;

    if (!cpu_feature(cpu, "MSR", 5)) {
        kprintf(-1, "CPU: No MSR capability\n");
        return;
    } else if (!cpu_feature(cpu, "APIC", 9)) {
        kprintf(-1, "CPU: No APIC capability\n");
        return;
    }

    // Find IO map for APIC
    uint32_t regA, regB;
    rdmsr(IA32_APIC_BASE_MSR, &regA, &regB);
    if ((regA & (1 << 11)) == 0) {
        kprintf(-1, "CPU: APIC disabled\n");
        return;
    } else if ((regA & (1 << 8)) == 0) {
        kprintf(-1, "CPU: Unfound BSP\n");
        return;
    }

    // Set from ACPI/MADT or from MSR here
    sysinfo->arch->apic = regA & ~(PAGE_SIZE - 1);
}
