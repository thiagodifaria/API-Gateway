bits 64
default rel

section .text

global base64_encode_simd_asm
global base64_decode_simd_asm
global uuid_generate_v4_asm
global hex_encode_fast_asm

base64_encode_simd_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 128
    
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov r10, rcx
    mov r11, rdx
    mov r12, r8
    
    mov rax, 0
    cmp r11, 0
    je .encode_done
    
    mov r13, 0
    mov r14, 0
    
.encode_loop:
    cmp r13, r11
    jge .encode_finalize
    
    movzx eax, byte [r10 + r13]
    shl eax, 16
    
    inc r13
    cmp r13, r11
    jge .encode_pad2
    
    movzx ebx, byte [r10 + r13]
    shl ebx, 8
    or eax, ebx
    
    inc r13
    cmp r13, r11
    jge .encode_pad1
    
    movzx ebx, byte [r10 + r13]
    or eax, ebx
    inc r13
    
    mov ebx, eax
    shr ebx, 18
    and ebx, 63
    call .get_base64_char
    mov [r12 + r14], al
    inc r14
    
    mov ebx, eax
    shr ebx, 12
    and ebx, 63
    call .get_base64_char
    mov [r12 + r14], al
    inc r14
    
    mov ebx, eax
    shr ebx, 6
    and ebx, 63
    call .get_base64_char
    mov [r12 + r14], al
    inc r14
    
    mov ebx, eax
    and ebx, 63
    call .get_base64_char
    mov [r12 + r14], al
    inc r14
    
    jmp .encode_loop
    
.encode_pad2:
    shl eax, 8
    mov ebx, eax
    shr ebx, 18
    and ebx, 63
    call .get_base64_char
    mov [r12 + r14], al
    inc r14
    
    mov ebx, eax
    shr ebx, 12
    and ebx, 63
    call .get_base64_char
    mov [r12 + r14], al
    inc r14
    
    mov byte [r12 + r14], '='
    inc r14
    mov byte [r12 + r14], '='
    inc r14
    jmp .encode_finalize
    
.encode_pad1:
    shl eax, 4
    mov ebx, eax
    shr ebx, 18
    and ebx, 63
    call .get_base64_char
    mov [r12 + r14], al
    inc r14
    
    mov ebx, eax
    shr ebx, 12
    and ebx, 63
    call .get_base64_char
    mov [r12 + r14], al
    inc r14
    
    mov ebx, eax
    shr ebx, 6
    and ebx, 63
    call .get_base64_char
    mov [r12 + r14], al
    inc r14
    
    mov byte [r12 + r14], '='
    inc r14
    
.encode_finalize:
    mov rax, r14
    
.encode_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    
    add rsp, 128
    pop rbp
    ret
    
.get_base64_char:
    cmp bl, 26
    jl .char_A_Z
    cmp bl, 52
    jl .char_a_z
    cmp bl, 62
    jl .char_0_9
    cmp bl, 62
    je .char_plus
    mov al, '/'
    ret
.char_plus:
    mov al, '+'
    ret
.char_A_Z:
    add bl, 'A'
    mov al, bl
    ret
.char_a_z:
    sub bl, 26
    add bl, 'a'
    mov al, bl
    ret
.char_0_9:
    sub bl, 52
    add bl, '0'
    mov al, bl
    ret

base64_decode_simd_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 128
    
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov r10, rcx
    mov r11, rdx
    mov r12, r8
    
    mov rax, 0
    cmp r11, 0
    je .decode_done
    
    mov r13, 0
    mov r14, 0
    
.decode_loop:
    cmp r13, r11
    jge .decode_finalize
    
    movzx eax, byte [r10 + r13]
    call .decode_base64_char
    shl eax, 18
    mov ebx, eax
    
    inc r13
    cmp r13, r11
    jge .decode_finalize
    
    movzx eax, byte [r10 + r13]
    call .decode_base64_char
    shl eax, 12
    or ebx, eax
    
    inc r13
    cmp r13, r11
    jge .decode_finalize
    
    movzx eax, byte [r10 + r13]
    cmp al, '='
    je .decode_final1
    call .decode_base64_char
    shl eax, 6
    or ebx, eax
    
    inc r13
    cmp r13, r11
    jge .decode_finalize
    
    movzx eax, byte [r10 + r13]
    cmp al, '='
    je .decode_final2
    call .decode_base64_char
    or ebx, eax
    
    mov eax, ebx
    shr eax, 16
    mov [r12 + r14], al
    inc r14
    
    mov eax, ebx
    shr eax, 8
    mov [r12 + r14], al
    inc r14
    
    mov eax, ebx
    mov [r12 + r14], al
    inc r14
    
    inc r13
    jmp .decode_loop
    
.decode_final1:
    mov eax, ebx
    shr eax, 16
    mov [r12 + r14], al
    inc r14
    jmp .decode_finalize
    
.decode_final2:
    mov eax, ebx
    shr eax, 16
    mov [r12 + r14], al
    inc r14
    
    mov eax, ebx
    shr eax, 8
    mov [r12 + r14], al
    inc r14
    
.decode_finalize:
    mov rax, r14
    
.decode_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    
    add rsp, 128
    pop rbp
    ret

.decode_base64_char:
    cmp al, '+'
    je .dec_plus
    cmp al, '/'
    je .dec_slash
    cmp al, '0'
    jge .check_digit
    cmp al, 'A'
    jge .check_upper
    cmp al, 'a'
    jge .check_lower
    mov eax, 0
    ret
.dec_plus:
    mov eax, 62
    ret
.dec_slash:
    mov eax, 63
    ret
.check_digit:
    cmp al, '9'
    jle .is_digit
    jmp .check_upper
.is_digit:
    sub al, '0'
    add al, 52
    movzx eax, al
    ret
.check_upper:
    cmp al, 'Z'
    jle .is_upper
    jmp .check_lower
.is_upper:
    sub al, 'A'
    movzx eax, al
    ret
.check_lower:
    cmp al, 'z'
    jle .is_lower
    mov eax, 0
    ret
.is_lower:
    sub al, 'a'
    add al, 26
    movzx eax, al
    ret

uuid_generate_v4_asm:
    push rbp
    mov rbp, rsp
    
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov r10, rcx
    
    rdrand rax
    mov [r10], rax
    
    rdrand rax
    mov [r10 + 8], rax
    
    mov al, [r10 + 6]
    and al, 0x0F
    or al, 0x40
    mov [r10 + 6], al
    
    mov al, [r10 + 8]
    and al, 0x3F
    or al, 0x80
    mov [r10 + 8], al
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    
    pop rbp
    ret

hex_encode_fast_asm:
    push rbp
    mov rbp, rsp
    
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    mov r10, rcx
    mov r11, rdx
    mov r12, r8
    
    mov r13, 0
    mov r14, 0
    
.hex_loop:
    cmp r13, r11
    jge .hex_done
    
    movzx eax, byte [r10 + r13]
    mov ebx, eax
    shr ebx, 4
    and ebx, 0x0F
    
    cmp bl, 10
    jl .hex_digit1
    add bl, 'A' - 10
    jmp .hex_store1
.hex_digit1:
    add bl, '0'
.hex_store1:
    mov [r12 + r14], bl
    inc r14
    
    and eax, 0x0F
    cmp al, 10
    jl .hex_digit2
    add al, 'A' - 10
    jmp .hex_store2
.hex_digit2:
    add al, '0'
.hex_store2:
    mov [r12 + r14], al
    inc r14
    
    inc r13
    jmp .hex_loop
    
.hex_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    
    pop rbp
    ret