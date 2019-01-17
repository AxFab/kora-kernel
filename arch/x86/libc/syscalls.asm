use32

global __syscall
extern __errno_location

align 64
__syscall:
    push ebp
    mov ebp, esp
    pusha
    mov eax, [ebp + 8]
    mov ecx, [ebp + 16]
    mov edx, [ebp + 24]
    mov ebx, [ebp + 32]
    mov edi, [ebp + 40]
    mov esi, [ebp + 48]
    int 0x40
    mov edi, eax
    call __errno_location
    mov [eax], edx
    mov eax, edi
    popa
    leave
    ret
