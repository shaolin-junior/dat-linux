section .bss
    pipefd resb 8

section .text
    global dasm

dasm:
    push r12
    mov r12d, edi
    call _pipefd
    jmp _readFile

_pipefd:
    mov rax, 22
    lea rdi, [rel pipefd]
    syscall
    test rax, rax
    js _errPipe
    
    xor rax, rax
    ret

_readFile:
    mov rax, 275
    mov rdi, r12
    xor rsi, rsi
    mov edx, [rel pipefd+4]
    xor r10, r10
    mov r8, 262144
    xor r9, r9
    syscall

    test rax, rax
    js _errWhileSplice
    jz _done

    jmp _printFile

_printFile:
    mov r8, rax

.print:
    mov rax, 275
    mov edi, [rel pipefd]
    xor esi, esi
    mov edx, 1
    xor r10d, r10d
    xor r9d, r9d
    syscall

    test rax, rax
    js _errWhileSplice
    jz _errWhileSplice

    sub r8, rax
    test r8, r8
    jnz .print

    jmp _readFile

_closePipe:
    mov eax, 3
    mov edi, [rel pipefd]
    syscall

    mov eax, 3
    mov edi, [rel pipefd+4]
    syscall

    ret

_errPipe:
    mov eax, 2
    jmp _return

_errWhileSplice:
    call _closePipe
    mov eax, 1
    jmp _return

_done:
    call _closePipe
    xor eax, eax
    jmp _return
_return:
    pop r12
    ret