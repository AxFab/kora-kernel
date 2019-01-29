use64

global _start
extern __libc_init, main, exit

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
