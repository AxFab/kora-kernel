;      This file is part of the KoraOS project.
;  Copyright (C) 2015-2021  <Fabien Bavent>
;
;  This program is free software: you can redistribute it and/or modify
;  it under the terms of the GNU Affero General Public License as
;  published by the Free Software Foundation, either version 3 of the
;  License, or (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU Affero General Public License for more details.
;
;  You should have received a copy of the GNU Affero General Public License
;  along with this program.  If not, see <http://www.gnu.org/licenses/>.
; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
use32
; S T A R T -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

global _start, start
extern code, bss, end

; Define some constant values
%define MBOOT_MAGIC1     0x1BADB002
%define MBOOT_MAGIC2     0x2BADB002
%define MBOOT_FLAGS      0x00010007 ;0x00010003; 0x00010007

%define KMM_EARLY_PAGE_PT 0x2000
%define KMM_EARLY_PAGE_PD 0x3000
%define KMM_EARLY_PAGE_PDP 0x4000
%define KMM_EARLY_PAGE_PML4 0x5000

%macro PUTC 3
    mov byte [0xB8000 + %1 * 2], %3
    mov byte [0xB8001 + %1 * 2], %2
%endmacro

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
section .text.boot
_start:
    jmp start

; Multiboot header
align 4
mboot:
    dd MBOOT_MAGIC1
    dd MBOOT_FLAGS
    dd - MBOOT_FLAGS - MBOOT_MAGIC1

    dd mboot
    dd code
    dd bss
    dd end
    dd start

    dd 1
    dd 80
    dd 25
    dd 0

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
use32
align 16
start:
    cli ; Block interruption
    PUTC 1, 0x17, 'S'
    mov esp, 0x4FF0 ; Initial stack is one page long!
    cmp eax, MBOOT_MAGIC2

    ; Choose paging mechanisms
    mov [multiboot_table], ebx
    PUTC 2, 0x15, 'm'


    ; Prepare and load pagination
    mov edi, KMM_EARLY_PAGE_PT
    mov cr3, edi
    mov ecx, 1024 * 4
    xor eax, eax
  .gd_loop:
    mov [edi], eax
    add edi, 4
    loop .gd_loop

    mov edi, KMM_EARLY_PAGE_PT
    mov esi, KMM_EARLY_PAGE_PD
    mov [edi], eax


    ; Enable PAE (cr4 |= 32)
    mov eax, cr4
    or eax, 32
    mov cr4, eax

    ; EFER !?
    mov ecx, 0xC0000080
    rdmsr
    or eax, 256
    wrmsr


    ; Set PG (Active paging)
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax


    ; Set early_LGDT !?

    ; Long jump to 32/64 bit code

    ; Segments

    ; Fill GDT ? IDT
    ; Early init()
    ; kernel_start()

    jmp 0x8:bsp_start64


align 4
multiboot_table:
    dd 0, 0


use64
align 4
extern kernel_start
bsp_start64:
    call kernel_start


;align 8
;ap_start64:

;use16
;align 4096
;ap_start:
