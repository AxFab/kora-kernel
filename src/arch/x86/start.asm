use32

; Define some constant values
%define GRUB_MAGIC1     0x1BADB002
%define GRUB_MAGIC2     0x2BADB002
%define GRUB_FLAGS      0x00010007 ;0x00010003; 0x00010007


%define SMP_GDT_REG  0x7e8
%define SMP_CPU_LOCK  0x7f0
%define SMP_CPU_COUNT  0x7f8

%define KERNEL_INIT_STACK_BASE  0x100000 ; 1 Mb
%define KERNEL_INIT_STACK_LENGTH  0x1000 ; 4 Kb
%define KERNEL_PAGE_DIR  0x2000

; Segments values
%define KCS  0x08
%define KDS  0x10
%define KSS  0x18
%define UCS  0x23
%define UDS  0x2d
%define USS  0x33
%define TSS0  0x38

; Macro to save registers on interruption
%macro SAVE_REGS 0
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
%endmacro

%macro PUTC 3
    mov byte [0xB8000 + 154], %2
    mov byte [0xB8001 + 154], %1
    mov byte [0xB8002 + 154], %3
    mov byte [0xB8003 + 154], %1
%endmacro

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; List export routines
GLOBAL _start
GLOBAL cpu_save, cpu_restore
GLOBAL cpu_halt_x86, call_method
GLOBAL smp_start, smp_error

GLOBAL x86_enable_MMU, x86_enable_SSE
GLOBAL x86_active_FPU, x86_active_cache
GLOBAL x86_cpuid,
GLOBAL x86_interrupt, x86_syscall, x86_syswait, x86_syssigret
GLOBAL acpi_enter, acpi_leave

GLOBAL x86_exception00, x86_exception01, x86_exception02, x86_exception03
GLOBAL x86_exception04, x86_exception05, x86_exception06, x86_exception07
GLOBAL x86_exception08, x86_exception09, x86_exception0A, x86_exception0B
GLOBAL x86_exception0C, x86_exception0D, x86_exception0E, x86_exception0F

GLOBAL x86_IRQ0, x86_IRQ1, x86_IRQ2, x86_IRQ3, x86_IRQ4, x86_IRQ5
GLOBAL x86_IRQ6, x86_IRQ7, x86_IRQ8, x86_IRQ9, x86_IRQ10, x86_IRQ11
GLOBAL x86_IRQ12, x86_IRQ13, x86_IRQ14, x86_IRQ15
GLOBAL x86_IRQ16, x86_IRQ17, x86_IRQ18, x86_IRQ19, x86_IRQ20

GLOBAL outb, outw, outl, inb, inw, inl,
GLOBAL outsb, outsw, outsl, insb, insw, insl

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; List import routines
extern code, bss, end
extern kernel_start, kernel_ready
extern grub_start
extern cpu_setup_x86, cpu_setup_core_x86, cpu_exception_x86, cpu_no
extern sys_irq_x86, sys_call_x86, sys_wait_x86, sys_sigret_x86
extern page_fault_x86, page_error_x86

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
_start:
    jmp start

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
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

    ; dd 0 ; VGA mode
    ; dd 800 ; VGA width
    ; dd 600 ; VGA height
    ; dd 32 ; VGA depth

    dd 1; VGA mode
    dd 80 ; VGA width
    dd 25 ; VGA height
    dd 0 ; VGA depth


; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
align 16
start:
    cli ; Block interruption
    mov esp, KERNEL_INIT_STACK_BASE + KERNEL_INIT_STACK_LENGTH - 4 ; Set stack
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
    call grub_start
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

    ; Initialize GDT, IDT & TSS
    call cpu_setup_x86

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
    mov ax, TSS0 ; TSS
    ltr ax

    call kernel_start
    jmp cpu_halt_x86.halt

align 4
  .gdt_regs:
    dw 0xF8, 0, 0, 0
  .idt_regs:
    dw 0x7F8, 0x800, 0, 0



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
    mov eax, [esp + 8] ; Get value
    test eax, eax
    jnz .n
    inc eax ; Be sure we don't return zero!
.n:
    mov ebx, [edx] ; Restore EBX
    mov esi, [edx + 4] ; Restore ESI
    mov edi, [edx + 8] ; Restore EDI
    mov ebp, [edx + 12] ; Restore EBP
    mov ecx, [edx + 16] ; Restore ESP
    mov esp, ecx
    mov ecx, [edx + 20] ; Restore EIP
    sti
    jmp ecx


; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
cpu_halt_x86:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8] ; CPU Stack
    mov edi, [ebp + 12] ; TSS Address
    sub eax, 16
    cli

    ; Prepare stack
    mov esp, eax
    mov dword [esp + 0], cpu_halt_x86.halt  ; eip
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

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
x86_enable_MMU:
    mov eax, KERNEL_PAGE_DIR ; PAGE kernel
    mov cr3, eax
    mov eax, cr0
    or eax, (1 << 31) ; CR0 31b to activate mmu
    mov cr0, eax
    ret

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
x86_enable_SSE:
; Prepares the necessary CPU flags to enable SSE instructions
    mov eax, cr0
    and ax, 0xFFFB
    or ax, 0x2
    mov cr0, eax
    mov eax, cr4
    or ax, 3 << 9
    mov cr4, eax
    ret

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; CR0 Register
;   PE [0] - Real-address mode:0
;   MP [1] - WAIT/FWAIT instruction not trapped:0
;   EM [2] - x87 FPU instruction not trapped:0
;   TS [3] - No task switch:0
;   NE [5] - External x87 FPU error reporting:0
;   WP [16] - Write-protect disabled:0
;   AM [18] - Alignement check disabled:0
;   NW [29] - Not write-through disabled:1
;   CD [30] - Caching disabled:1
;   PG [31] - Paging disabled:0
; ------------------------------------------
x86_active_FPU:
    mov eax, cr0
    and eax, 0xfffffffb
    or eax, 2
    mov cr0, eax
    ret

x86_active_cache:
    mov eax, cr0
    and eax, 0x9fffffff
    mov cr0, eax
    ret

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
x86_cpuid:
    push ebp
    mov ebp, esp
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
    leave
    ret

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
%macro INT_HANDLER 1
    SAVE_REGS
    push esp
    push %1
    call cpu_exception_x86
    add esp, 8
    LOAD_REGS
    iret
%endmacro

x86_exception00:
    INT_HANDLER 0x0
x86_exception01:
    INT_HANDLER 0x1
x86_exception02:
    INT_HANDLER 0x2
x86_exception03:
    INT_HANDLER 0x3
x86_exception04:
    INT_HANDLER 0x4
x86_exception05:
    INT_HANDLER 0x5
x86_exception06:
    INT_HANDLER 0x6
x86_exception07:
    INT_HANDLER 0x7
x86_exception08:
    INT_HANDLER 0x8
x86_exception09:
    INT_HANDLER 0x9
x86_exception0A:
    INT_HANDLER 0xA
x86_exception0B:
    INT_HANDLER 0xB
x86_exception0C:
    INT_HANDLER 0xC
x86_exception0D:
    SAVE_REGS
    push esp
    push dword [esp + 52] ; get error code
    call page_error_x86
    add esp, 8
    LOAD_REGS
    add esp, 4
    iret
x86_exception0E:
    SAVE_REGS
    push esp
    push dword [esp + 52] ; get error code
    mov eax, cr2
    push eax
    call page_fault_x86
    add esp, 12
    LOAD_REGS
    add esp, 4
    iret
x86_exception0F:
    INT_HANDLER 0xF

x86_interrupt:
    INT_HANDLER -1

x86_syscall:
    SAVE_REGS
    push esp
    call sys_call_x86
    add esp, 4
    LOAD_REGS
    iret

x86_syswait:
    cli
    SAVE_REGS
    push esp
    call sys_wait_x86
    add esp, 4
    LOAD_REGS
    iret

x86_syssigret:
    cli
    SAVE_REGS
    push esp
    call sys_sigret_x86
    add esp, 4
    LOAD_REGS
    iret

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
%macro IRQ_HANDLER 1
    SAVE_REGS
    push esp
    push %1
    call sys_irq_x86
    add esp, 8
    LOAD_REGS
    iret
%endmacro

x86_IRQ0:
    IRQ_HANDLER 0
x86_IRQ1:
    IRQ_HANDLER 1
x86_IRQ2:
    IRQ_HANDLER 2
x86_IRQ3:
    IRQ_HANDLER 3
x86_IRQ4:
    IRQ_HANDLER 4
x86_IRQ5:
    IRQ_HANDLER 5
x86_IRQ6:
    IRQ_HANDLER 6
x86_IRQ7:
    IRQ_HANDLER 7
x86_IRQ8:
    IRQ_HANDLER 8
x86_IRQ9:
    IRQ_HANDLER 9
x86_IRQ10:
    IRQ_HANDLER 10
x86_IRQ11:
    IRQ_HANDLER 11
x86_IRQ12:
    IRQ_HANDLER 12
x86_IRQ13:
    IRQ_HANDLER 13
x86_IRQ14:
    IRQ_HANDLER 14
x86_IRQ15:
    IRQ_HANDLER 15

x86_IRQ16:
    IRQ_HANDLER 16
x86_IRQ17:
    IRQ_HANDLER 17
x86_IRQ18:
    IRQ_HANDLER 18
x86_IRQ19:
    IRQ_HANDLER 19
x86_IRQ20:
    IRQ_HANDLER 20

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
%macro IO_OUT 1
    push ebp
    mov ebp, esp
    mov dx, [esp + 8]
    mov %1, [esp + 12]
    out dx, %1
    leave
    ret
%endmacro

%macro IO_OUTS 1
    push ebp
    mov ebp, esp
    mov dx, [esp + 8]
    mov esi, [esp + 12]
    mov ecx, [esp + 16]
    cld
    rep %1
    leave
    ret
%endmacro

%macro IO_IN 1
    push ebp
    mov ebp, esp
    mov dx, [esp + 8]
    xor eax, eax
    in %1, dx
    leave
    ret
%endmacro

%macro IO_INS 1
    push ebp
    mov ebp, esp
    mov dx, [esp + 8]
    mov edi, [esp + 12]
    mov ecx, [esp + 16]
    cld
    rep %1
    leave
    ret
%endmacro

outb:
    IO_OUT al
outw:
    IO_OUT ax
outl:
    IO_OUT eax
inb:
    IO_IN al
inw:
    IO_IN ax
inl:
    IO_IN eax
outsb:
    IO_OUTS outsb
outsw:
    IO_OUTS outsw
outsl:
    IO_OUTS outsd
insb:
    IO_INS insb
insw:
    IO_INS insw
insl:
    IO_INS insd


; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; int call_method(
;      void *entry,
;      int argc,
;      void *argv);
call_method:
  mov eax, [esp] ; EIP backup
  mov ebx, [esp + 4] ; entry point
  mov ecx, [esp + 8] ; argc
  mov esi, [esp + 12] ; argv

  sub esp, ecx ; ARGS
  mov edi, esp
  push eax ; EIP
  shr ecx, 2
  rep movsd; Copy args
  jmp ebx

GLOBAL cpu_run_x86, cpu_cr3_x86, cpu_ticks
cpu_run_x86:
    mov esp, [esp + 4] ; Stack base
    mov al,0x20
    out 0x20,al
    LOAD_REGS
    iret

cpu_cr3_x86:
    mov eax, [esp + 4] ; DIRECTORY
    mov ebx, cr3
    cmp eax, ebx
    je .done
    mov cr3, eax
  .done:
    ret

cpu_ticks:
    rdtsc ; Read Time-Stamp Counter
    ret


; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
align 4096
use16
smp_start:
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

    jmp dword KCS:smp_stage    ; code segment

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
align 32
use32
extern cpu_on
smp_stage:
    cli

    lidt [startup.idt_regs] ; IDT

    ; Lock CPU Initialization Spin-Lock
  .spinlock:
    cmp dword [SMP_CPU_LOCK], 0    ; Check if lock is free
    je .getlock
    pause
    jmp .spinlock
  .getlock:
    mov eax, 1
    xchg eax, [SMP_CPU_LOCK]  ; Try to get lock
    cmp eax, 0            ; Test is successful
    jne .spinlock
  .criticalsection:

    ; TODO - Ensure we don't wake up more CPUs than we can handle (x32).

    ; Compute stack
    mov eax, [.stack]
    add eax, KERNEL_INIT_STACK_LENGTH
    mov [.stack], eax
    add eax, KERNEL_INIT_STACK_LENGTH - 4
    mov esp, eax

    ; Set TSS
    call cpu_on
    call cpu_no
    shl eax, 3 ; cpu_no x 8
    add ax, TSS0 ; TSS
    ltr ax

    call x86_enable_MMU

    call cpu_setup_core_x86

    ; Unlock the spinlock
    xor eax, eax
    mov [SMP_CPU_LOCK], eax

    call kernel_ready
    jmp cpu_halt_x86.halt

  .stack:
    dd KERNEL_INIT_STACK_BASE + KERNEL_INIT_STACK_LENGTH


; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; Enters the ACPI subsystem
acpi_enter:
    mov eax, cr0
    and eax, 0x7FFFFFFF
    or eax, 0x60000000
    mov cr0, eax
    ret

; Leaves the ACPI subsystem
acpi_leave:
    mov eax, cr0
    or eax, 0x80000000
    and eax, ~0x60000000
    mov cr0, eax
    ret

; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
align 4096
use16
smp_error:
    nop
    jmp $
