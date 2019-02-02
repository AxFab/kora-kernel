use64

global __syscall
extern __errno_location

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
