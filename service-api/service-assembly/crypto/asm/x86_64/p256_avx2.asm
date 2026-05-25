bits 64
default rel

section .rdata align=32
P256_PRIME:
    dq 0xffffffffffffffff, 0x00000000ffffffff
    dq 0x0000000000000000, 0xffffffff00000001

section .text

global p256_mul_mont
global p256_sqr_mont
global p256_add_mod
global p256_sub_mod
global p256_point_add
global p256_point_double

p256_mul_mont:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov rax, [rdx]
    mov rbx, [r8]
    mul rbx
    mov [rcx], rax
    mov [rcx+8], rdx
    
    xor rax, rax
    mov [rcx+16], rax
    mov [rcx+24], rax
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

p256_sqr_mont:
    push rbp
    mov rbp, rsp
    
    mov rax, [rdx]
    mul rax
    mov [rcx], rax
    mov [rcx+8], rdx
    
    xor rax, rax
    mov [rcx+16], rax
    mov [rcx+24], rax
    
    pop rbp
    ret

p256_add_mod:
    mov rax, [rdx]
    add rax, [r8]
    mov [rcx], rax
    
    mov rax, [rdx+8]
    adc rax, [r8+8]
    mov [rcx+8], rax
    
    mov rax, [rdx+16]
    adc rax, [r8+16]
    mov [rcx+16], rax
    
    mov rax, [rdx+24]
    adc rax, [r8+24]
    mov [rcx+24], rax
    
    ret

p256_sub_mod:
    mov rax, [rdx]
    sub rax, [r8]
    mov [rcx], rax
    
    mov rax, [rdx+8]
    sbb rax, [r8+8]
    mov [rcx+8], rax
    
    mov rax, [rdx+16]
    sbb rax, [r8+16]
    mov [rcx+16], rax
    
    mov rax, [rdx+24]
    sbb rax, [r8+24]
    mov [rcx+24], rax
    
    ret

p256_point_add:
    push rbp
    mov rbp, rsp
    
    mov rax, 12
.copy_loop:
    mov r10, [rdx + rax*8 - 8]
    mov [rcx + rax*8 - 8], r10
    dec rax
    jnz .copy_loop
    
    pop rbp
    ret

p256_point_double:
    push rbp
    mov rbp, rsp
    
    mov rax, 12
.copy_loop:
    mov r10, [rdx + rax*8 - 8]
    mov [rcx + rax*8 - 8], r10
    dec rax
    jnz .copy_loop
    
    pop rbp
    ret