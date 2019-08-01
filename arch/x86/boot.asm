
%define MBOOT_MAGIC1     0x1BADB002
%define MBOOT_MAGIC2     0x2BADB002
%define MBOOT_FLAGS      0x00010007 ;0x00010003; 0x00010007

global _start, start

use32
extern code, data, bss, end
section .boot

; Multiboot header
align 4
multiboot:
    dd MBOOT_MAGIC1
    dd MBOOT_FLAGS
    dd - MBOOT_FLAGS - MBOOT_MAGIC1

    dd multiboot
    dd code
    dd bss
    dd end
    dd start

    dd 1
    dd 80
    dd 25
    dd 0

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

%macro PUTC 3
    mov byte [0xB8000 + %3], %2
    mov byte [0xB8001 + %3], %1
%endmacro

%define BOOT_STACK  0x5000
%define BOOT_GDT  0x1800
%define BOOT_IDT  0x1000
%define BOOT_PGD  0x2000
%define BOOT_PT   0x3000

extern x86_setup_gdt

section .text
align 16
_start:
start:
    cli
    cld
    ; Setup stack
    mov eax, BOOT_STACK
    mov esp, eax

    ; Clean console
    xor eax, eax
    mov edi, 0xb8000
    mov ecx, 80 * 24
    rep stosw

    ; Setup GDT
    push BOOT_GDT
    call x86_setup_gdt
    lgdt [gdt32]
    PUTC 0x20, ' ', 0
    PUTC 0x20, 'G', 2
    PUTC 0x20, 'D', 4
    PUTC 0x20, 'T', 6
    PUTC 0x20, ' ', 8

    ; Enable CPU features

    ; Enable paging
    xor eax, eax
    mov edi, BOOT_PGD
    mov ecx, 1024
    rep stosd

    xor eax, eax
    or eax, 3 ; Present and writable
    mov edi, BOOT_PT
    mov ecx, 1024
.l1:
    mov [edi], eax
    add edi, 4
    add eax, 0x1000
    loop .l1

    mov eax, BOOT_PT
    or eax, 3 ; Present and writable
    mov edi, BOOT_PGD
    mov [edi], eax

    mov eax, BOOT_PGD
    or eax, 3 ; Present and writable
    mov edi, BOOT_PGD
    mov [edi + 0x4096 - 4], eax

    mov cr3, edi

    ; Disable PIC and NMI

    PUTC 0x47, 'u', 12
    jmp $


section .data
align 64
gdt32:
    dw 0xf8
    dd BOOT_GDT
align 64
idt32:
    dw 0x7f8
    dd BOOT_IDT




