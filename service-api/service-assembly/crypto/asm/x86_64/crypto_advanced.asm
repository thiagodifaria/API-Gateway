bits 64
default rel

section .text

global chacha20_encrypt_block_asm
global poly1305_mac_block_asm
global blake3_hash_chunk_asm
global x25519_scalar_mult_asm

chacha20_encrypt_block_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 256
    
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov dword [rsp], 0x61707865
    mov dword [rsp+4], 0x3320646e
    mov dword [rsp+8], 0x79622d32
    mov dword [rsp+12], 0x6b206574
    
    mov rax, [rdx]
    mov [rsp+16], rax
    mov rax, [rdx+8]
    mov [rsp+24], rax
    
    mov rax, [rdx+16]
    mov [rsp+32], rax
    mov rax, [rdx+24]
    mov [rsp+40], rax
    
    mov [rsp+48], r9d
    
    mov r10, 10
    
.chacha_round_loop:
    mov eax, [rsp]
    add eax, [rsp+16]
    mov [rsp+64], eax
    
    mov eax, [rsp+48]
    xor eax, [rsp+64]
    rol eax, 16
    mov [rsp+48], eax
    
    mov eax, [rsp+32]
    add eax, [rsp+48]
    mov [rsp+32], eax
    
    mov eax, [rsp+16]
    xor eax, [rsp+32]
    rol eax, 12
    mov [rsp+16], eax
    
    dec r10
    jnz .chacha_round_loop
    
    mov r10, 0
.output_loop:
    mov eax, [rsp + r10*4]
    xor eax, [rcx + r10*4]
    mov [r8 + r10*4], eax
    
    inc r10
    cmp r10, 16
    jl .output_loop
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    
    add rsp, 256
    pop rbp
    ret

poly1305_mac_block_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 128
    
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov r10, rdx
    mov r11, rcx
    
    mov rax, [r8]
    mov rbx, 0x0ffffffc0fffffff
    and rax, rbx
    mov [rsp], rax
    
    mov rax, [r8 + 8]
    mov rbx, 0x0ffffffc0ffffffc
    and rax, rbx
    mov [rsp + 8], rax
    
    xor r12, r12
    xor r13, r13
    
.poly_loop:
    test r10, r10
    jz .poly_done
    
    mov rax, [r11]
    add r12, rax
    adc r13, 0
    
    mov rax, r12
    mul qword [rsp]
    mov r14, rax
    mov r15, rdx
    
    mov rax, r13
    mul qword [rsp + 8]
    add r14, rax
    adc r15, rdx
    
    mov r12, r14
    mov r13, r15
    
    add r11, 16
    sub r10, 16
    jmp .poly_loop
    
.poly_done:
    mov [r9], r12
    mov [r9 + 8], r13
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    
    add rsp, 128
    pop rbp
    ret

blake3_hash_chunk_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 256
    
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov dword [rsp], 0x6A09E667
    mov dword [rsp+4], 0xBB67AE85
    mov dword [rsp+8], 0x3C6EF372
    mov dword [rsp+12], 0xA54FF53A
    mov dword [rsp+16], 0x510E527F
    mov dword [rsp+20], 0x9B05688C
    mov dword [rsp+24], 0x1F83D9AB
    mov dword [rsp+28], 0x5BE0CD19
    
    mov r10, 0
.blake3_round_loop:
    cmp r10, 32
    jge .blake3_done
    
    mov eax, [rsp + r10]
    ror eax, 2
    mov ebx, [rsp + r10 + 4]
    xor eax, ebx
    mov [rsp + r10 + 128], eax
    
    add r10, 4
    jmp .blake3_round_loop
    
.blake3_done:
    mov r10, 0
.copy_loop:
    cmp r10, 32
    jge .copy_done
    mov eax, [rsp + 128 + r10]
    mov [r8 + r10], eax
    add r10, 4
    jmp .copy_loop
    
.copy_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    
    add rsp, 256
    pop rbp
    ret

x25519_scalar_mult_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 512
    
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov r10, 0
.copy_inputs:
    cmp r10, 32
    jge .copy_done
    
    mov al, [rdx + r10]
    mov bl, [rcx + r10]
    mul bl
    mov [rsp + r10], al
    
    inc r10
    jmp .copy_inputs
    
.copy_done:
    mov r10, 0
.x25519_reduce_loop:
    cmp r10, 32
    jge .x25519_reduce_done
    
    mov al, [rsp + r10]
    mov [r8 + r10], al
    
    inc r10
    jmp .x25519_reduce_loop
    
.x25519_reduce_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    
    add rsp, 512
    pop rbp
    ret