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
global start, _start
extern code, bss, end
extern x86_gdt, x86_idt, x86_paging
extern kstart, serial_early_init
extern apic_ptr, cpu_kstacks, kready

; Define some constant values
%define MBOOT_MAGIC1     0x1BADB002
%define MBOOT_MAGIC2     0x2BADB002
%define MBOOT_FLAGS      0x00010007 ;0x00010003; 0x00010007

%define KSTACK0         0x4000
%define KSTACK_LEN      0x1000

%macro DEBUG_WRITE 2
    mov dword [esp + 4], %2
    mov dword [esp + 0], %1
    call clwrite
%endmacro

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
use32
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
align 16
use32
section .text
start:
    ; Block interruption
    cli
    ; Setup kernel stack
    mov esp, KSTACK0 + KSTACK_LEN - 16
    ; Check we used a multiboot complient loader (grub)
    cmp eax, MBOOT_MAGIC2
    jmp .mbootLoader

.unknowLoader:
    DEBUG_WRITE 0, msg_unknowLoader
    hlt
    jmp $

.mbootLoader:
    ; Store grub table pointer
    mov [mboot_table], ebx

    ; Clear console
    xor eax, eax
    mov edi, 0xB8000
    mov ecx, 80 / 2 * 25
    rep stosd

    DEBUG_WRITE 0, msg_mbootLoader

    ; Clear memory [0-16Kb]
    xor eax, eax
    mov edi, eax
    mov ecx, 4096
    rep stosd

    ; Clear bss
    mov edi, bss
    mov ecx, end
    sub ecx, edi
    add ecx, 3
    shr ecx, 2
    rep stosd

    ; Serial COM init
    call serial_early_init

    ; Check platform support 32/64bit
    mov eax, 0x80000001
    cpuid
    test edx, 0x20000000
    ; mov eax, [mboot_data.max_cpuid]
    ; test eax, 0x80000000
    jnz .support64
    DEBUG_WRITE 80, msg_support32
    jmp .supportDone
.support64:
    DEBUG_WRITE 80, msg_support64
    ; Write flag !?
.supportDone:

    ; Setup GDT
    call x86_gdt
    lgdt [i386_data.gdt]
    jmp 0x08:start32
start32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Setup IDT
    call x86_idt
    lidt [i386_data.idt]

    ; Setup Paging
    call x86_paging
    mov eax, 0x2000
    mov cr3, eax
    mov eax, cr0
    or eax, (1 << 31) ; CR0 31b to activate mmu
    mov cr0, eax

    ; Call the kernel startup
    call kstart

    sti
    hlt
    jmp $


; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
global x86_delay
x86_delay:
    push ecx
    push edx
    mov ecx, [esp + 12]
    mov dx, 0x1fc
.loop:
    in al, dx
    loopnz .loop
    pop edx
    pop ecx
    ret

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
global x86_set_tss
x86_set_tss:
    mov eax, [esp + 4]
    shl ax, 3 ; TSS
    ltr ax
    ret

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
global x86_set_cr3
x86_set_cr3:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
global x86_cpuid
x86_cpuid:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    push edx
    mov eax, [ebp + 8]
    mov ecx, [ebp + 12]
    cpuid
    mov edi, [ebp + 16]
    mov [edi], eax
    mov [edi + 4], ebx
    mov [edi + 8], edx
    mov [edi + 12], ecx
    pop edx
    pop ecx
    pop ebx
    leave
    ret

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
global clwrite
clwrite:
    push ebp
    mov ebp, esp
    mov edi, 0xB8000
    mov eax, [ebp + 8]
    shl eax, 1
    add edi, eax
    mov esi, [ebp + 12]
    mov cl, 0x07
    mov al, [esi]
.lp:
    test al, al
    jz .done
    mov [edi], al
    mov [edi + 1], cl
    inc esi
    add edi, 2
    mov al, [esi]
    jmp .lp
.done:
    leave
    ret

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
section .data
global mboot_table
mboot_table:
    dd 0
msg_unknowLoader:
    db "Unknow Loader ", 0
msg_mbootLoader:
    db "Multiboot Loader ", 0
msg_support32:
    db "Plateform is 32bits", 0
msg_support64:
    db "Plateform is 64bits", 0

align 4
i386_data:
.gdt:
    dw 0x800, 0, 0, 0
.idt:
    dw 0x800, 0x800, 0, 0


; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
%define SMP_GDT_REG  0x7e0
%define SMP_CPU_COUNT  0x7f8

section .text
use16
align 4096
global ap_start
ap_start:
    cli
    mov ax, 0x0
    mov ds, ax
    mov es, ax

    ; Activate GDT (!?)
    mov di, SMP_GDT_REG
    mov word [di], 0xF8
    lgdt [di] ; GDT

    ; Active protected mode
    mov eax, cr0
    or ax, 1
    mov cr0, eax ; PE flags set

    lock inc word [SMP_CPU_COUNT]
    jmp .next
.next:
    mov ax, 0x10
    mov ds, ax
    mov fs, ax
    mov gs, ax
    mov es, ax
    mov ss, ax

    jmp dword 0x08:ap_boot

use32
align 32
ap_boot:
    ; Reactive the right GDT
    lgdt [i386_data.gdt]
    jmp 0x08:.next
.next:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Enable IDT
    lidt [i386_data.idt]

    ; Enable Paging
    mov eax, 0x2000
    mov cr3, eax
    mov eax, cr0
    or eax, (1 << 31) ; CR0 31b to activate mmu
    mov cr0, eax

    ; Get APIC Id
    mov edi, [apic_ptr]
    mov ecx, [edi + 0x20] ; Read APIC ID
    shr ecx, 24
    and ecx, 0xf

    ; Get stack pointer
    mov ebx, [cpu_kstacks]
    mov eax, [ebx + ecx * 4]
    mov esp, eax

    call kready

    sti
    hlt
    jmp $

