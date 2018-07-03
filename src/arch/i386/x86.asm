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
use32

global x86_cpuid
global x86_enable_mmu
global x86_set_cr3
global x86_set_tss
global x86_delay

%define KERNEL_PAGE_DIR 0x2000


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

x86_enable_mmu:
    mov eax, KERNEL_PAGE_DIR
    mov cr3, eax
    mov eax, cr0
    or eax, (1 << 31) ; CR0 31b to activate mmu
    mov cr0, eax
    ret

x86_set_cr3:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

x86_set_tss:
    mov eax, [esp + 4]
    shl ax, 3 ; TSS
    ltr ax
    ret

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
