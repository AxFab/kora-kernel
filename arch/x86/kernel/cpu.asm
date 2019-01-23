;      This file is part of the KoraOS project.
;  Copyright (C) 2018  <Fabien Bavent>
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

global cpu_no, cpu_save, cpu_restore, cpu_halt, cpu_usermode
extern apic_regs, cpu_table

%define KCS 0x8
%define KDS 0x10
%define KSS 0x18
%define UCS 0x23
%define UDS 0x2B
%define USS 0x33

cpu_no:
    mov eax, [apic_regs]
    test eax, eax
    jnz .n
    ret
.n:
    mov eax, [eax + 0x20] ; Read APIC ID
    shr eax, 24
    and eax, 0xf
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
    cli
    ; Get TSS Address
    call cpu_no
    mov ecx, eax
    shl eax, 7
    add eax, 0x1000
    mov edi, eax

    ; Compute stack
    mov ebx, [cpu_table]
    mov edx, ecx
    shl edx, 5
    add ebx, edx
    add ebx, 12
    mov eax, [ebx]
    add eax, 0xFF0
    mov esp, eax

    ; Prepare stack
    mov esp, eax
    mov dword [esp + 0], cpu_halt.halt  ; eip
    mov dword [esp + 4], KCS  ; cs
    mov dword [esp + 8], 0x200  ; eflags

    ; Set TSS ESP0
    add edi, 4
    mov [edi], eax

    mov ax, KDS
    mov ds, ax
    mov es, ax

    ; End of interupt
    mov al,0x20
    out 0x20,al
    iret

.halt:
    sti
    hlt
    pause
    jmp .halt



cpu_usermode:
    cli
    mov ebx, [esp + 4]
    mov eax, [esp + 8]
    push USS ; Stack segment
    push eax ; User stack

    pushf ; Flags
    pop eax
    or eax, 0x200 ; Interupt flags (on)
    and eax, 0xffffbfff ; Nested task (off)
    push eax

    push UCS ; Code segment
    push ebx ; Instruction pointer

    push UDS ; Data segment
    pop ds

    iret

