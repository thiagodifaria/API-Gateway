bits 64
default rel

section .data align=32
crlf_pattern: times 8 dw 0x0d0a
crlf_crlf: db 0x0d, 0x0a, 0x0d, 0x0a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

section .text

global http_find_header_end_avx2
global http_parse_method_uri_avx2

http_find_header_end_avx2:
    test rdx, rdx
    jz .not_found
    
    cmp rdx, 4
    jb .not_found
    
    vmovdqu xmm0, [crlf_crlf]
    
    mov rax, rcx
    mov r8, rcx
    add r8, rdx
    sub r8, 3
    
.search_loop:
    cmp rcx, r8
    ja .not_found
    
    vmovdqu xmm1, [rcx]
    vpcmpeqd xmm2, xmm1, xmm0
    vpmovmskb r9d, xmm2
    
    test r9d, r9d
    jnz .check_match
    
    inc rcx
    jmp .search_loop
    
.check_match:
    mov r10d, [rcx]
    cmp r10d, 0x0a0d0a0d
    je .found
    
    inc rcx
    jmp .search_loop
    
.found:
    sub rcx, rax
    add rcx, 4
    mov [r8], rcx
    mov rax, 1
    vzeroupper
    ret
    
.not_found:
    xor rax, rax
    vzeroupper
    ret

http_parse_method_uri_avx2:
    test rdx, rdx
    jz .error
    
    mov rax, r8
    mov qword [rax], 0
    mov qword [rax+8], 0
    mov qword [rax+16], 0
    mov qword [rax+24], 0
    mov qword [rax+32], 0
    mov byte [rax+40], 0
    
    mov r9, rcx
    mov r10, rcx
    add r10, rdx
    
.find_method_end:
    cmp r9, r10
    jae .error
    
    movzx r11, byte [r9]
    cmp r11b, 0x20
    je .method_found
    
    inc r9
    jmp .find_method_end
    
.method_found:
    mov r11, r9
    sub r11, rcx
    mov [rax], r11
    
    inc r9
    
.skip_spaces1:
    cmp r9, r10
    jae .error
    
    movzx r11, byte [r9]
    cmp r11b, 0x20
    jne .find_uri_start
    
    inc r9
    jmp .skip_spaces1
    
.find_uri_start:
    mov r11, r9
    sub r11, rcx
    mov [rax+8], r11
    
.find_uri_end:
    cmp r9, r10
    jae .error
    
    movzx r11, byte [r9]
    cmp r11b, 0x20
    je .uri_found
    
    inc r9
    jmp .find_uri_end
    
.uri_found:
    mov r11, r9
    sub r11, rcx
    sub r11, [rax+8]
    mov [rax+16], r11
    
    mov byte [rax+40], 1
    mov rax, 1
    vzeroupper
    ret
    
.error:
    mov byte [rax+40], 0
    xor rax, rax
    vzeroupper
    ret