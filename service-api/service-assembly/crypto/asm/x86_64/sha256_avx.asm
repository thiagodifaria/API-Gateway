bits 64
default rel

section .rdata align=64
K256:
    dd 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5
    dd 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5
    dd 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3
    dd 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174
    dd 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc
    dd 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da
    dd 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7
    dd 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967
    dd 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13
    dd 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85
    dd 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3
    dd 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070
    dd 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5
    dd 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3
    dd 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208
    dd 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2

section .text

global sha256_block_asm

; SHA-256 block processing function
sha256_block_asm:
    push rbp
    mov rbp, rsp

    push rbx
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    
    ; Prepare message schedule W[0..15]
    mov r13, 0
.init_loop:
    mov r14d, [rcx + r13*4]
    bswap r14d
    mov [rsp + r13*4], r14d
    inc r13
    cmp r13, 16
    jl .init_loop
    
    ; Extend message schedule W[16..63]
    mov r13, 16
.extend_loop:
    mov r14d, [rsp + (r13-2)*4]
    mov r15d, r14d
    ror r15d, 17
    mov esi, r14d
    ror esi, 19
    xor r15d, esi
    mov esi, r14d
    shr esi, 10
    xor r15d, esi
    
    add r15d, [rsp + (r13-7)*4]
    
    mov r14d, [rsp + (r13-15)*4]
    mov eax, r14d
    ror eax, 7
    mov esi, r14d
    ror esi, 18
    xor eax, esi
    mov esi, r14d
    shr esi, 3
    xor eax, esi
    add r15d, eax
    
    add r15d, [rsp + (r13-16)*4]
    mov [rsp + r13*4], r15d
    
    inc r13
    cmp r13, 64
    jl .extend_loop
    
    ; Restore working variables
    mov eax, [rdx]
    mov ebx, [rdx+4]
    mov ecx, [rdx+8]
    mov r8d, [rdx+12]
    mov r9d, [rdx+16]
    mov r10d, [rdx+20]
    mov r11d, [rdx+24]
    mov r12d, [rdx+28]
    
    ; Main compression loop
    mov r13, 0
.compress_loop:
    ; T1 = h + SIGMA1(e) + CH(e,f,g) + K[i] + W[i]
    mov r14d, r12d
    mov r15d, r9d
    ror r15d, 6
    mov esi, r9d
    ror esi, 11
    xor r15d, esi
    ror esi, 14
    xor r15d, esi
    add r14d, r15d
    
    ; CH(e,f,g)
    mov r15d, r9d
    and r15d, r10d
    mov esi, r9d
    not esi
    and esi, r11d
    xor r15d, esi
    add r14d, r15d
    
    ; Add K[i] and W[i]
    lea rsi, [K256]
    add r14d, [rsi + r13*4]
    add r14d, [rsp + r13*4]
    
    ; T2 = SIGMA0(a) + MAJ(a,b,c)
    mov r15d, eax
    ror r15d, 2
    mov esi, eax
    ror esi, 13
    xor r15d, esi
    ror esi, 9
    xor r15d, esi
    
    ; MAJ(a,b,c)
    mov esi, eax
    and esi, ebx
    mov edi, eax
    and edi, ecx
    xor esi, edi
    mov edi, ebx
    and edi, ecx
    xor esi, edi
    add r15d, esi
    
    ; Update working variables
    mov r12d, r11d
    mov r11d, r10d
    mov r10d, r9d
    mov r9d, r8d
    add r9d, r14d
    mov r8d, ecx
    mov ecx, ebx
    mov ebx, eax
    mov eax, r14d
    add eax, r15d
    
    inc r13
    cmp r13, 64
    jl .compress_loop
    
    ; Add to hash values
    add [rdx], eax
    add [rdx+4], ebx
    add [rdx+8], ecx
    add [rdx+12], r8d
    add [rdx+16], r9d
    add [rdx+20], r10d
    add [rdx+24], r11d
    add [rdx+28], r12d
    
    add rsp, 256

    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbx

    pop rbp
    ret
