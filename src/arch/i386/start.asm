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
extern grub_init, cpu_early_init

; Define some constant values
%define GRUB_MAGIC1     0x1BADB002
%define GRUB_MAGIC2     0x2BADB002
%define GRUB_FLAGS      0x00010007 ;0x00010003; 0x00010007

%define KSTACK0         0x4000
%define KSTACK_LEN      0x1000

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
    PUTC 0x57, 'U', 'n'
    jmp .failed
.errorLoader:
    PUTC 0x57, 'E', 'r'
    jmp .failed
.failed:
    hlt
    jmp $

.grubLoader:
    PUTC 0x57, 'G', 'r'
    push ebx
    call grub_init
    pop ebx
    test eax, eax
    jnz .errorLoader

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
startup:
    ; Clear the first 4 Kb
    xor eax, eax
    mov edi, eax
    mov ecx, 2048
    rep stosd

    PUTC 0x37, 'G', '0'
    jmp $