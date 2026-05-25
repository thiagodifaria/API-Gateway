bits 64
default rel

section .text

global fast_memcpy_avx2
global fast_memchr_avx2
global fast_memmove_avx2

fast_memcpy_avx2:
    mov rax, rcx
    
    cmp r8, 32
    jb .small_copy
    
    mov r9, rcx
    and r9, 31
    jz .aligned_start
    
    mov r10, 32
    sub r10, r9
    cmp r8, r10
    cmovb r10, r8
    
    sub r8, r10
.prefix_loop:
    movzx r11, byte [rdx]
    mov [rcx], r11b
    inc rcx
    inc rdx
    dec r10
    jnz .prefix_loop
    
.aligned_start:
    mov r9, r8
    shr r9, 7
    jz .handle_remainder
    
.main_loop:
    vmovdqu ymm0, [rdx]
    vmovdqu ymm1, [rdx + 32]
    vmovdqu ymm2, [rdx + 64]
    vmovdqu ymm3, [rdx + 96]
    
    vmovdqa [rcx], ymm0
    vmovdqa [rcx + 32], ymm1
    vmovdqa [rcx + 64], ymm2
    vmovdqa [rcx + 96], ymm3
    
    add rcx, 128
    add rdx, 128
    dec r9
    jnz .main_loop
    
.handle_remainder:
    and r8, 127
    jz .done
    
    mov r9, r8
    shr r9, 5
    jz .handle_small
    
.chunk_loop:
    vmovdqu ymm0, [rdx]
    vmovdqa [rcx], ymm0
    add rcx, 32
    add rdx, 32
    dec r9
    jnz .chunk_loop
    
.handle_small:
    and r8, 31
    jz .done
    
.small_copy:
.byte_loop:
    movzx r9, byte [rdx]
    mov [rcx], r9b
    inc rcx
    inc rdx
    dec r8
    jnz .byte_loop
    
.done:
    vzeroupper
    ret

fast_memchr_avx2:
    mov rax, 0
    
    test r8, r8
    jz .not_found
    
    movd xmm0, edx
    vpbroadcastb ymm0, xmm0
    
    mov r9, rcx
    and r9, 31
    jz .aligned_search
    
    mov r10, 32
    sub r10, r9
    cmp r8, r10
    cmovb r10, r8
    
.prefix_search:
    movzx r11, byte [rcx]
    cmp r11b, dl
    je .found_at_rcx
    inc rcx
    dec r8
    dec r10
    jnz .prefix_search
    
    test r8, r8
    jz .not_found
    
.aligned_search:
    mov r9, r8
    shr r9, 5
    jz .search_remainder
    
.search_loop:
    vmovdqa ymm1, [rcx]
    vpcmpeqb ymm2, ymm1, ymm0
    vpmovmskb r10d, ymm2
    
    test r10d, r10d
    jnz .found_in_chunk
    
    add rcx, 32
    sub r8, 32
    dec r9
    jnz .search_loop
    
.search_remainder:
    and r8, 31
    jz .not_found
    
.remainder_search:
    movzx r9, byte [rcx]
    cmp r9b, dl
    je .found_at_rcx
    inc rcx
    dec r8
    jnz .remainder_search
    
.not_found:
    xor rax, rax
    vzeroupper
    ret
    
.found_in_chunk:
    bsf r11d, r10d
    add rcx, r11
    mov rax, rcx
    vzeroupper
    ret
    
.found_at_rcx:
    mov rax, rcx
    vzeroupper
    ret

fast_memmove_avx2:
    mov rax, rcx
    
    cmp rcx, rdx
    jae .copy_backward
    
    jmp fast_memcpy_avx2
    
.copy_backward:
    mov r9, rcx
    add r9, r8
    mov r10, rdx
    add r10, r8
    
    cmp r8, 32
    jb .small_backward
    
    mov r11, r9
    and r11, 31
    jz .aligned_backward
    
    cmp r8, r11
    cmovb r11, r8
    sub r8, r11
    
.suffix_loop:
    dec r10
    dec r9
    movzx rax, byte [r10]
    mov [r9], al
    dec r11
    jnz .suffix_loop
    
.aligned_backward:
    mov r11, r8
    shr r11, 5
    jz .backward_remainder
    
.backward_loop:
    sub r10, 32
    sub r9, 32
    vmovdqu ymm0, [r10]
    vmovdqa [r9], ymm0
    dec r11
    jnz .backward_loop
    
.backward_remainder:
    and r8, 31
    jz .backward_done
    
.small_backward:
.backward_byte_loop:
    dec r10
    dec r9
    movzx r11, byte [r10]
    mov [r9], r11b
    dec r8
    jnz .backward_byte_loop
    
.backward_done:
    mov rax, rcx        
    vzeroupper
    ret