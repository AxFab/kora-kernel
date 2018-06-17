;      This file is part of the KoraOS project.
;  Copyright (C) 2015  <Fabien Bavent>
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
; S T A R T -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
use32

global _start, start
extern code, bss, end
extern grub_init, cpu_early_init, kernel_start
extern apic_regs, cpu_table, ap_setup

; Define some constant values
%define GRUB_MAGIC1     0x1BADB002
%define GRUB_MAGIC2     0x2BADB002
%define GRUB_FLAGS      0x00010007 ;0x00010003; 0x00010007

%define KSTACK0         0x4000
%define KSTACK_LEN      0x1000

; Segments values
%define KCS  0x08
%define KDS  0x10
%define KSS  0x18
%define UCS  0x23
%define UDS  0x2d
%define USS  0x33
%define TSS0  0x38

%define SMP_GDT_REG  0x7e8
%define SMP_CPU_LOCK  0x7f0
%define SMP_CPU_COUNT  0x7f8

%define KERNEL_PAGE_DIR 0x2000

%macro PUTC 3
    mov byte [0xB8000 + 154], %2
    mov byte [0xB8001 + 154], %1
    mov byte [0xB8002 + 154], %3
    mov byte [0xB8003 + 154], %1
%endmacro

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
section .text.boot
_start:
    jmp start

; Multiboot header, used by Grub
align 4
mboot:
    dd GRUB_MAGIC1
    dd GRUB_FLAGS
    dd - GRUB_FLAGS - GRUB_MAGIC1

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
start:
    cli ; Block interruption
    mov esp, KSTACK0 + KSTACK_LEN - 4 ; Set stack
    cmp eax, GRUB_MAGIC2 ; Check we used grub as loader
    jmp .grubLoader

.unknowLoader:
    PUTC 0x47, 'U', 'n'
    jmp .failed
.errorLoader:
    PUTC 0x47, 'E', 'r'
    jmp .failed
.failed:
    hlt
    jmp $

.grubLoader:
    PUTC 0x17, 'G', 'r'
    push ebx
    call grub_init
    pop ebx
    test eax, eax
    jnz .errorLoader

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
startup:
    ; Clear the first 4 Kb
    xor eax, eax
    mov edi, eax
    mov ecx, 2048
    rep stosd

    PUTC 0x17, 'S', 't'
    cli
    call cpu_early_init

    lgdt [.gdt_regs] ; GDT
    jmp KCS:.reloadCS

  .reloadCS:
    mov ax, KDS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax


    lidt [.idt_regs] ; IDT
    ;mov ax, TSS0 ; TSS
    ;ltr ax

    PUTC 0x27, 'G', 'o'
    cli
    call kernel_start
    PUTC 0x27, 'E', 'd'
    ;sti
    jmp $

align 4
  .gdt_regs:
    dw 0xF8, 0, 0, 0
  .idt_regs:
    dw 0x7F8, 0x800, 0, 0




; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
section .text

global ap_start

use16
align 4096
ap_start:
    cli
    ; Reset data segments
    mov ax, 0x0
    mov ds, ax
    mov es, ax

    ; Activate GDT
    mov di, SMP_GDT_REG
    mov word [di], 0xF8
    lgdt [di] ; GDT

    ; Mode protected
    mov eax, cr0
    or ax, 1
    mov cr0, eax ; PE flags set

    lock inc word [SMP_CPU_COUNT]
    jmp .next

.next:
    ; Reset segments
    mov ax, KDS
    mov ds, ax
    mov fs, ax
    mov gs, ax
    mov es, ax
    mov ss, ax

    jmp dword KCS:ap_boot    ; code segment

use32
align 32
ap_boot:
    cli
    PUTC 0x17, 'O', 'n'

    lidt [startup.idt_regs] ; IDT

    ; Lock CPU Initialization Spin-Lock
;.spinlock:
;    cmp dword [SMP_CPU_LOCK], 0    ; Check if lock is free
;    je .getlock
;    pause
;    jmp .spinlock
;.getlock:
;    mov eax, 1
;    xchg eax, [SMP_CPU_LOCK]  ; Try to get lock
;    cmp eax, 0            ; Test is successful
;    jne .spinlock
;.criticalsection:

    ; Active paging
    mov eax, KERNEL_PAGE_DIR
    mov cr3, eax
    mov eax, cr0
    or eax, (1 << 31) ; CR0 31b to activate mmu
    mov cr0, eax

    ; Get APIC Id
    mov edi, [apic_regs]
    mov ecx, [edi + 0x20] ; Read APIC ID
    shr ecx, 24
    and ecx, 0xf

    ; Compute stack
    mov ebx, [cpu_table]
    mov edx, ecx
    shl edx, 5
    add ebx, edx
    add ebx, 12
    mov eax, [ebx]
    add eax, 0xFFC
    mov esp, eax

    ; ...

    ; Unlock the spinlock
;    xor eax, eax
;    mov [SMP_CPU_LOCK], eax

    ; ...
    call ap_setup

    jmp $
