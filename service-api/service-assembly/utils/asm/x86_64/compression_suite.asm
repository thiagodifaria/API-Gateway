bits 64
default rel

section .text

global deflate_compress_small_asm
global lz4_compress_fast_asm
global brotli_compress_web_asm

deflate_compress_small_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 512
    
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov r10, rcx
    mov r11, rdx
    mov r12, r8
    mov r13, r9
    
    mov rax, 0
    cmp r11, 0
    je .deflate_done
    
    cmp r11, 65536
    jg .deflate_done
    
    mov byte [r12], 0x78
    mov byte [r12 + 1], 0x9c
    add r12, 2
    sub r13, 2
    
    mov r14, 0
.deflate_copy_loop:
    cmp r14, r11
    jge .deflate_finalize
    cmp r14, r13
    jge .deflate_finalize
    
    mov al, [r10 + r14]
    mov [r12 + r14], al
    inc r14
    jmp .deflate_copy_loop
    
.deflate_finalize:
    add r12, r14
    mov dword [r12], 0x00000001
    add r14, 6
    mov rax, r14
    
.deflate_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    
    add rsp, 512
    pop rbp
    ret

lz4_compress_fast_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 256
    
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov r10, rcx
    mov r11, rdx
    mov r12, r8
    mov r13, r9
    
    mov rax, 0
    cmp r11, 0
    je .lz4_done
    
    mov r14, 0
    mov r15, 0
    
.lz4_compress_loop:
    cmp r14, r11
    jge .lz4_finalize
    cmp r15, r13
    jge .lz4_finalize
    
    mov al, [r10 + r14]
    mov bl, al
    
    cmp r14, 4
    jl .lz4_literal
    
    mov ecx, [r10 + r14 - 4]
    cmp ecx, [r10 + r14]
    je .lz4_match
    
.lz4_literal:
    mov [r12 + r15], al
    inc r14
    inc r15
    jmp .lz4_compress_loop
    
.lz4_match:
    mov byte [r12 + r15], 0xF0
    inc r15
    mov word [r12 + r15], 4
    add r15, 2
    add r14, 4
    jmp .lz4_compress_loop
    
.lz4_finalize:
    mov rax, r15
    
.lz4_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    
    add rsp, 256
    pop rbp
    ret

brotli_compress_web_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 1024
    
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov r10, rcx
    mov r11, rdx
    mov r12, r8
    mov r13, r9
    
    mov rax, 0
    cmp r11, 0
    je .brotli_done
    
    mov byte [r12], 0x8b
    mov byte [r12 + 1], 0x03
    add r12, 2
    sub r13, 2
    
    mov r14, 0
    mov r15, 0
    
.brotli_analyze_loop:
    cmp r14, r11
    jge .brotli_compress_start
    
    mov al, [r10 + r14]
    cmp al, 0x20
    je .brotli_space_found
    cmp al, 0x3C
    je .brotli_html_found
    cmp al, 0x7B
    je .brotli_css_found
    
    inc r14
    jmp .brotli_analyze_loop
    
.brotli_space_found:
    inc r15
    inc r14
    jmp .brotli_analyze_loop
    
.brotli_html_found:
    add r15, 2
    inc r14
    jmp .brotli_analyze_loop
    
.brotli_css_found:
    add r15, 3
    inc r14
    jmp .brotli_analyze_loop
    
.brotli_compress_start:
    mov r14, 0
    mov rax, 0
    
.brotli_copy_loop:
    cmp r14, r11
    jge .brotli_finalize
    cmp rax, r13
    jge .brotli_finalize
    
    mov bl, [r10 + r14]
    
    cmp bl, 0x20
    je .brotli_compress_space
    cmp bl, 0x3C
    je .brotli_compress_html
    
    mov [r12 + rax], bl
    inc rax
    inc r14
    jmp .brotli_copy_loop
    
.brotli_compress_space:
    mov byte [r12 + rax], 0x81
    inc rax
    inc r14
    jmp .brotli_copy_loop
    
.brotli_compress_html:
    mov byte [r12 + rax], 0x82
    inc rax
    inc r14
    jmp .brotli_copy_loop
    
.brotli_finalize:
    mov byte [r12 + rax], 0x03
    inc rax
    
.brotli_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    
    add rsp, 1024
    pop rbp
    ret