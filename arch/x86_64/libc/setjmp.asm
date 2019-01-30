use64

global setjmp, longjmp

setjmp:
    mov [rdi], rbx  ; Store RBX
    mov [rdi + 8], rbp ; Store RBP
    mov [rdi + 16], r12
    mov [rdi + 24], r13
    mov [rdi + 32], r14
    mov [rdi + 40], r15
    lea rdx, [rsp + 8]
    mov [rdi + 48], rdx ; Store RSP
    mov rdx, [rsp]
    mov [rdi + 56], rdx ; Store RIP
    xor rax, rax
    ret

longjmp:
    mov rax, rsi
    test rax, rax
    jnz .n
    inc rax
.n:
    mov rbx, [rdi]
    mov rbp, [rdi + 8]
    mov r12, [rdi + 16]
    mov r13, [rdi + 24]
    mov r14, [rdi + 32]
    mov r15, [rdi + 40]
    mov rdx, [rdi + 48]
    mov rsp, rdx
    mov rdx, [rdi + 56]
    jmp rdx

