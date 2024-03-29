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

; Macro to save registers on interruption
%macro SAVE_REGS 0
    push ebp
    mov ebp, esp
    pushad
    push ds
    push es
    push fs
    push gs
    push ebx
    mov bx,0x10
    mov ds,bx
    pop ebx
%endmacro

%macro LOAD_REGS 0
    pop gs
    pop fs
    pop es
    pop ds
    popad
    leave
%endmacro

%macro FAULT_HANDLER 1
    SAVE_REGS
    push esp
    push %1
    call x86_fault
    add esp, 8
    LOAD_REGS
    iret
%endmacro

%macro IRQ_HANDLER 1
    SAVE_REGS
    push esp
    push %1
    call x86_irq
    add esp, 8
    LOAD_REGS
    iret
%endmacro

%macro IPI_HANDLER 1
    SAVE_REGS
    push esp
    push %1
    call x86_ipi
    add esp, 8
    LOAD_REGS
    iret
%endmacro

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

extern x86_fault, x86_error, x86_pgflt, x86_irq, x86_ipi, x86_syscall
global int_exDV, int_exDB, int_exNMI, int_exBP, int_exOF, int_exBR,
global int_exUD, int_exNM, int_exDF, int_exCO, int_exTS, int_exNP,
global int_exSS, int_exGP, int_exPF, int_exMF, int_exAC, int_exMC,
global int_exXF, int_exVE, int_exSX
global int_irq0, int_irq1, int_irq2, int_irq3, int_irq4, int_irq5
global int_irq6, int_irq7, int_irq8, int_irq9, int_irq10, int_irq11
global int_irq12, int_irq13, int_irq14, int_irq15, int_irq16, int_irq17
global int_irq18, int_irq19, int_irq20, int_irq21, int_irq22, int_irq23
global int_irq24, int_irq25, int_irq26, int_irq27, int_irq28, int_irq29
global int_irq30, int_irq31,
global int_isr123, int_isr124, int_isr125, int_isr126, int_isr127, int_irqLT
global int_default, int_syscall

int_exDV:
    FAULT_HANDLER  0x0
int_exDB:
    FAULT_HANDLER  0x1
int_exNMI:
    FAULT_HANDLER  0x2
int_exBP:
    FAULT_HANDLER  0x3
int_exOF:
    FAULT_HANDLER  0x4
int_exBR:
    FAULT_HANDLER  0x5
int_exUD:
    FAULT_HANDLER  0x6
int_exNM:
    FAULT_HANDLER  0x7
int_exDF:
    FAULT_HANDLER  0x8
int_exCO:
    FAULT_HANDLER  0x9
int_exTS:
    FAULT_HANDLER  0xA
int_exNP:
    FAULT_HANDLER  0xB
int_exSS:
    FAULT_HANDLER  0xC
int_exMF:
    FAULT_HANDLER  0x10
int_exAC:
    FAULT_HANDLER  0x11
int_exMC:
    FAULT_HANDLER  0x12
int_exXF:
    FAULT_HANDLER  0x13
int_exVE:
    FAULT_HANDLER  0x14
int_exSX:
    FAULT_HANDLER  0x15
int_exGP:
    SAVE_REGS
    push esp
    push dword [esp + 56] ; get error code
    push 0xD
    call x86_error
    add esp, 12
    LOAD_REGS
    add esp, 4
    iret
int_exPF:
    SAVE_REGS
    push esp
    push dword [esp + 56] ; get error code
    mov eax, cr2
    push eax
    call x86_pgflt
    add esp, 12
    LOAD_REGS
    add esp, 4
    iret


int_irq0:
    IRQ_HANDLER  0
int_irq1:
    IRQ_HANDLER  1
int_irq2:
    IRQ_HANDLER  2
int_irq3:
    IRQ_HANDLER  3
int_irq4:
    IRQ_HANDLER  4
int_irq5:
    IRQ_HANDLER  5
int_irq6:
    IRQ_HANDLER  6
int_irq7:
    IRQ_HANDLER  7
int_irq8:
    IRQ_HANDLER  8
int_irq9:
    IRQ_HANDLER  9
int_irq10:
    IRQ_HANDLER  10
int_irq11:
    IRQ_HANDLER  11
int_irq12:
    IRQ_HANDLER  12
int_irq13:
    IRQ_HANDLER  13
int_irq14:
    IRQ_HANDLER  14
int_irq15:
    IRQ_HANDLER  15
int_irq16:
    IRQ_HANDLER  16
int_irq17:
    IRQ_HANDLER  17
int_irq18:
    IRQ_HANDLER  18
int_irq19:
    IRQ_HANDLER  19
int_irq20:
    IRQ_HANDLER  20
int_irq21:
    IRQ_HANDLER  21
int_irq22:
    IRQ_HANDLER  22
int_irq23:
    IRQ_HANDLER  23
int_irq24:
    IRQ_HANDLER  24
int_irq25:
    IRQ_HANDLER  25
int_irq26:
    IRQ_HANDLER  26
int_irq27:
    IRQ_HANDLER  27
int_irq28:
    IRQ_HANDLER  28
int_irq29:
    IRQ_HANDLER  29
int_irq30:
    IRQ_HANDLER  30
int_irq31:
    IRQ_HANDLER  31


int_isr123:
    IPI_HANDLER 123
int_isr124:
    IPI_HANDLER 124
int_isr125:
    IPI_HANDLER 125
int_isr126:
    IPI_HANDLER 126
int_isr127:
    IPI_HANDLER 127

int_irqLT:
    IPI_HANDLER 128


int_default:
    FAULT_HANDLER  -1

int_syscall:
    SAVE_REGS
    push esp
    call x86_syscall
    add esp, 4
    LOAD_REGS
    iret
