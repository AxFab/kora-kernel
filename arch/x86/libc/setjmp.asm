use32

global setjmp, longjmp

setjmp:
    mov eax, [esp + 4] ; Get buffer
    mov [eax], ebx ; Store EBX
    mov [eax + 4], esi ; Store ESI
    mov [eax + 8], edi ; Store EDI
    mov [eax + 12], ebp ; Store EBP
    lea ecx, [esp + 4] ; ESP
    mov [eax + 16], ecx ; Store ESP
    mov ecx, [esp] ; EIP
    mov [eax + 20], ecx ; Store EIP
    xor eax, eax
    ret

longjmp:
    mov edx, [esp + 4] ; Get buffer
    mov eax, [esp + 8] ; Get value
    test eax, eax
    jnz .n
    inc eax ; Be sure we don't return zero!
.n:
    mov ebx, [edx] ; Restore EBX
    mov esi, [edx + 4] ; Restore ESI
    mov edi, [edx + 8] ; Restore EDI
    mov ebp, [edx + 12] ; Restore EBP
    mov ecx, [edx + 16] ; Restore ESP
    mov esp, ecx
    mov ecx, [edx + 20] ; Restore EIP
    jmp ecx
