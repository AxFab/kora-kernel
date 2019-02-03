use32

global __syscall
extern __errno_location

align 64
__syscall:
    push ebp
    mov ebp, esp
    pusha
    mov eax, [ebp + 8]
    mov ecx, [ebp + 12]
    mov edx, [ebp + 16]
    mov ebx, [ebp + 20]
    mov esi, [ebp + 24]
    mov edi, [ebp + 28]
    int 0x40
    mov edi, eax
    call __errno_location
    mov [eax], edx
    mov [esp], edi
    popa
    mov eax, edi
    leave
    ret
