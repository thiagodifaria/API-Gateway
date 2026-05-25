bits 64
default rel

section .data align=32
json_chars: db 0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            db 1,0,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0
            db 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1
            db 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0

unsafe_chars: db '<','>', '&', '"', '\'', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
              times 16 db 0

utf8_continuation: times 32 db 0x80

section .text

global json_validate_fast_avx2
global input_sanitize_basic_avx2
global utf8_validate_simd_avx2

json_validate_fast_avx2:
    test rdx, rdx
    jz .invalid
    
    mov rax, 0
    mov r8, rcx
    mov r9, rcx
    add r9, rdx
    
    mov r10, 0
    mov r11, 0
    
.validate_loop:
    cmp r8, r9
    jae .check_balance
    
    movzx rax, byte [r8]
    
    cmp al, 0x7F
    ja .invalid
    
    lea rbx, [json_chars]
    movzx rax, byte [rbx + rax]
    test al, al
    jz .invalid
    
    movzx rax, byte [r8]
    cmp al, '{'
    je .open_brace
    cmp al, '}'
    je .close_brace
    cmp al, '['
    je .open_bracket
    cmp al, ']'
    je .close_bracket
    cmp al, '"'
    je .quote_state
    
    inc r8
    jmp .validate_loop
    
.open_brace:
    inc r10
    inc r8
    jmp .validate_loop
    
.close_brace:
    dec r10
    test r10, r10
    js .invalid
    inc r8
    jmp .validate_loop
    
.open_bracket:
    inc r11
    inc r8
    jmp .validate_loop
    
.close_bracket:
    dec r11
    test r11, r11
    js .invalid
    inc r8
    jmp .validate_loop
    
.quote_state:
    inc r8
.find_quote_end:
    cmp r8, r9
    jae .invalid
    
    movzx rax, byte [r8]
    cmp al, '"'
    je .quote_end
    cmp al, '\'
    je .escape_char
    
    inc r8
    jmp .find_quote_end
    
.escape_char:
    inc r8
    cmp r8, r9
    jae .invalid
    inc r8
    jmp .find_quote_end
    
.quote_end:
    inc r8
    jmp .validate_loop
    
.check_balance:
    test r10, r10
    jnz .invalid
    test r11, r11
    jnz .invalid
    
    mov rax, 0
    ret
    
.invalid:
    mov rax, 1
    ret

input_sanitize_basic_avx2:
    test rdx, rdx
    jz .done
    
    vmovdqu xmm0, [unsafe_chars]
    
    mov r8, rcx
    mov r9, rcx
    add r9, rdx
    
.sanitize_loop:
    cmp r8, r9
    jae .done
    
    movzx rax, byte [r8]
    
    cmp al, '<'
    je .replace_char
    cmp al, '>'
    je .replace_char
    cmp al, '&'
    je .replace_char
    cmp al, '"'
    je .replace_char
    cmp al, 0x27
    je .replace_char
    cmp al, 0x00
    je .replace_char
    cmp al, 0x0A
    je .replace_char
    cmp al, 0x0D
    je .replace_char
    
    inc r8
    jmp .sanitize_loop
    
.replace_char:
    mov byte [r8], '_'
    inc r8
    jmp .sanitize_loop
    
.done:
    mov rax, 1
    vzeroupper
    ret

utf8_validate_simd_avx2:
    test rdx, rdx
    jz .valid
    
    mov r8, rcx
    mov r9, rcx
    add r9, rdx
    
.utf8_loop:
    cmp r8, r9
    jae .valid
    
    movzx rax, byte [r8]
    
    cmp al, 0x80
    jb .ascii_char
    
    cmp al, 0xC0
    jb .invalid
    
    cmp al, 0xE0
    jb .two_byte
    
    cmp al, 0xF0
    jb .three_byte
    
    cmp al, 0xF8
    jb .four_byte
    
    jmp .invalid
    
.ascii_char:
    inc r8
    jmp .utf8_loop
    
.two_byte:
    inc r8
    cmp r8, r9
    jae .invalid
    
    movzx rax, byte [r8]
    and al, 0xC0
    cmp al, 0x80
    jne .invalid
    
    inc r8
    jmp .utf8_loop
    
.three_byte:
    inc r8
    mov r10, 2
    
.check_continuation:
    cmp r8, r9
    jae .invalid
    
    movzx rax, byte [r8]
    and al, 0xC0
    cmp al, 0x80
    jne .invalid
    
    inc r8
    dec r10
    jnz .check_continuation
    
    jmp .utf8_loop
    
.four_byte:
    inc r8
    mov r10, 3
    jmp .check_continuation
    
.valid:
    mov rax, 1
    vzeroupper
    ret
    
.invalid:
    xor rax, rax
    vzeroupper
    ret