use64

global _start
extern __libc_init, _main, exit
extern _GLOBAL_OFFSET_TABLE_

%macro  get_GOT 0

        call    %%getgot
  %%getgot:
        pop     rbx
        add     rbx,_GLOBAL_OFFSET_TABLE_+$$-%%getgot wrt ..gotpc

%endmacro


_start:
    mov rbp, rsp
    and rsp, ~0xF

    get_GOT
    mov rax,[rbx + __libc_init wrt ..got]
    call rax

    mov rax, [rbp + 4] ; argc
    mov [rsp], rax
    mov rax, [rbp + 8] ; argv
    mov [rsp + 4], rax
    mov rax, [rbp + 12] ; env
    mov [rsp + 8], eax
    call _main
    mov [rsp], rax
    get_GOT
    mov rax,[rbx + exit wrt ..got]
    call rax
    jmp $

