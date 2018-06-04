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
    call cpu_early_setup

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



; -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=