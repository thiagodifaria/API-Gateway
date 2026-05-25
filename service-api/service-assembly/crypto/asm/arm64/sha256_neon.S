.arch armv8-a+crypto

.text
.global sha256_block_asm

.section .rodata
.align 4
K256:
    .word 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5
    .word 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5
    .word 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3
    .word 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174
    .word 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc
    .word 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da
    .word 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7
    .word 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967
    .word 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13
    .word 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85
    .word 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3
    .word 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070
    .word 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5
    .word 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3
    .word 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208
    .word 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2

.text
.align 4

sha256_block_asm:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    
    ld1 {v16.4s, v17.4s}, [x1]
    
    ld1 {v0.16b-v3.16b}, [x0]
    
    rev32 v0.16b, v0.16b
    rev32 v1.16b, v1.16b
    rev32 v2.16b, v2.16b
    rev32 v3.16b, v3.16b
    
    adrp x2, K256
    add x2, x2, :lo12:K256
    
    mov v20.16b, v16.16b    // a,b,c,d
    mov v21.16b, v17.16b    // e,f,g,h
    
    mov x3, #0
    
.L_round_loop:
    ldr w4, [x2, x3, lsl #2]
    dup v18.4s, w4
    
    mov x5, x3
    and x6, x5, #3
    lsr x5, x5, #2
    
    cmp x5, #0
    beq .L_use_v0
    cmp x5, #1
    beq .L_use_v1
    cmp x5, #2
    beq .L_use_v2
    b .L_use_v3
    
.L_use_v0:
    mov v19.16b, v0.16b
    b .L_continue_round
.L_use_v1:
    mov v19.16b, v1.16b
    b .L_continue_round
.L_use_v2:
    mov v19.16b, v2.16b
    b .L_continue_round
.L_use_v3:
    mov v19.16b, v3.16b
    
.L_continue_round:
    mov v22.s[0], v19.s[x6]
    dup v22.4s, v22.s[0]
    
    sha256h q21, q20, v22.4s
    sha256h2 q20, q21, v22.4s
    add v22.4s, v22.4s, v18.4s
    
    add x3, x3, #1
    cmp x3, #16
    blt .L_round_loop
    
    mov x3, #16
    
.L_extend_loop:
    cmp x3, #64
    bge .L_compression_done
    
    add x3, x3, #1
    b .L_extend_loop
    
.L_compression_done:
    add v16.4s, v16.4s, v20.4s
    add v17.4s, v17.4s, v21.4s
    
    st1 {v16.4s, v17.4s}, [x1]
    
    ldp x29, x30, [sp], #16
    ret