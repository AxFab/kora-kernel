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
#ifndef _KERNEL_SYS_ELF_H
#define _KERNEL_SYS_ELF_H 1


enum ELF_Class {
    ELFCLASSNONE = 0,     /// Invalid class
    ELFCLASS32 = 1,       /// 32-bit objects
    ELFCLASS64 = 2,       /// 64-bit objects
};

#define ET_NONE         0       /// No file type
#define ET_REL          1       /// Relocatable file
#define ET_EXEC         2       /// Executable file
#define ET_DYN          3       /// Shared object file
#define ET_CORE         4       /// Core file
#define ET_LOPROC       0xff00  /// Processor-specific
#define ET_HIPROC       0xffff  /// Processor-specific

enum ELF_Machine {
    EM_NONE = 0,       /// No machine
    EM_M32 = 1,        /// AT&T WE 32100
    EM_SPARC = 2,      /// SPARC
    EM_386 = 3,        /// Intel 80386
    EM_68K = 4,        /// Motorola 68000
    EM_88K = 5,        /// Motorola 88000
    EM_860 = 7,        /// Intel 80860
    EM_MIPS = 8,       /// MIPS RS3000
};

/* Sh Type */
#define PT_NULL             0
#define PT_LOAD             1
#define PT_DYNAMIC          2
#define PT_INTERP           3
#define PT_NOTE             4
#define PT_SHLIB            5
#define PT_PHDR             6
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7fffffff

PACK(struct ELF_header {
    uint8_t     ident_[16];
    uint16_t    type_;
    uint16_t    machine_;
    uint32_t    version_;
    uint32_t    entry_;
    uint32_t    phOff_;
    uint32_t    shOff_;
    uint32_t    flags_;
    uint16_t    ehSize_;
    uint16_t    phSize_;
    uint16_t    phCount_;
    uint16_t    shSize_;
    uint16_t    shCount_;
    uint16_t    shstRndx_;
});

PACK(struct ELF_phEntry {
    uint32_t    type_;
    uint32_t    fileAddr_;
    uint32_t    virtAddr_;
    uint32_t    physAddr_;
    uint32_t    fileSize_;
    uint32_t    memSize_;
    uint32_t    flags_;
    uint32_t    align_;
});

static uint8_t ELFident[] = {
    0x7f, 0x45, 0x4c, 0x46, ELFCLASS32, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};




#endif  /* _KERNEL_SYS_ELF_H */
