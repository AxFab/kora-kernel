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
; C P U   R O U T I N E S -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
use32

global cpu_no, cpu_save, cpu_restore, cpu_halt
extern apic_mmio

cpu_no:
    mov eax, apic_mmio
    test eax, eax
    jnz .n
    ret
.n:
    mov eax, [eax + 0x20] ; Read APIC ID
    ret

cpu_save:
    mov eax, [esp + 4] ; Get buffer
    mov [eax], ebx ; Store EBX
    mov [eax + 4], esi ; Store ESI
    mov [eax + 8], edi ; Store EDI
    mov [eax + 12], ebp ; Store EBP
    lea ecx, [esp + 4] ; ESP
    mov [eax + 16], ecx ; Store ESP
    mov ecx, [esp] ; EIP
    mov [eax + 20], ecx ; Store EIP
    xor eax, eax
    ret

cpu_restore:
    mov edx, [esp + 4] ; Get buffer
    xor eax, eax
    inc eax
    mov ebx, [edx] ; Restore EBX
    mov esi, [edx + 4] ; Restore ESI
    mov edi, [edx + 8] ; Restore EDI
    mov ebp, [edx + 12] ; Restore EBP
    mov ecx, [edx + 16] ; Restore ESP
    mov esp, ecx
    mov ecx, [edx + 20] ; Restore EIP
    sti
    jmp ecx

cpu_halt:
    sti
    hlt
    jmp $
