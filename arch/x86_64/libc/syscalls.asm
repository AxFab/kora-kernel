use64

global __syscall
extern __errno_location
extern _GLOBAL_OFFSET_TABLE_

%macro  get_GOT 0

        call    %%getgot
  %%getgot:
        pop     rbx
        add     rbx,_GLOBAL_OFFSET_TABLE_+$$-%%getgot wrt ..gotpc

%endmacro



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

    get_GOT
    mov rax,[rbx + __errno_location wrt ..got]
    call rax

    mov [rax], rdx
    mov rax, rdi
    popq
    leave
    ret
