use64

global _start, __syscall
extern __libc_init, main, exit, __errno_location

_start:
    mov rbp, rsp
    and rsp, ~0xF
    call __libc_init
    xor rax, rax
    mov [rsp + 4], rax
    mov [rsp + 8], rax
    mov [rsp + 12], rax
    call main
    mov [rsp + 4], rax
    call exit
    jmp $

align 64
__syscall:
    push rbp
    mov rbp, rsp
    pushq
    mov rax, [rbp + 8]
    mov rcx, [rbp + 16]
    mov rdx, [rbp + 24]
    mov rbx, [rbp + 32]
    mov rdi, [rbp + 40]
    mov rsi, [rbp + 48]
    int 0x40
    mov rdi, rax
    call __errno_location
    mov [rax], rdx
    mov rax, rdi
    popq
    leave
    ret
