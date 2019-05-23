use32

global _start
extern __libc_init, _main, exit
extern _GLOBAL_OFFSET_TABLE_

%macro  get_GOT 0

        call    %%getgot
  %%getgot:
        pop     ebx
        add     ebx,_GLOBAL_OFFSET_TABLE_+$$-%%getgot wrt ..gotpc

%endmacro


_start:
    mov ebp, esp
    and esp, ~0xF
    sub esp, 0x4

    get_GOT
    mov eax,[ebx + __libc_init wrt ..got]
    call eax

    mov eax, [ebp + 4] ; argc
    mov [esp], eax
    mov eax, [ebp + 8] ; argv
    mov [esp + 4], eax
    mov eax, [ebp + 12] ; env
    mov [esp + 8], eax
    call _main
    mov [esp], eax

    get_GOT
    mov eax,[ebx + exit wrt ..got]
    call eax

    jmp $
