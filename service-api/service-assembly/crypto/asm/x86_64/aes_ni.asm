bits 64
section .text
global aes_encrypt_block_asm

aes_encrypt_block_asm:
    movdqa  xmm0, [rcx]
    movdqa  xmm1, [r8]
    pxor    xmm0, xmm1

    movdqa  xmm1, [r8 + 16]
    aesenc  xmm0, xmm1
    movdqa  xmm1, [r8 + 32]
    aesenc  xmm0, xmm1
    movdqa  xmm1, [r8 + 48]
    aesenc  xmm0, xmm1
    movdqa  xmm1, [r8 + 64]
    aesenc  xmm0, xmm1
    movdqa  xmm1, [r8 + 80]
    aesenc  xmm0, xmm1
    movdqa  xmm1, [r8 + 96]
    aesenc  xmm0, xmm1
    movdqa  xmm1, [r8 + 112]
    aesenc  xmm0, xmm1
    movdqa  xmm1, [r8 + 128]
    aesenc  xmm0, xmm1
    movdqa  xmm1, [r8 + 144]
    aesenc  xmm0, xmm1

    movdqa  xmm1, [r8 + 160]
    aesenclast xmm0, xmm1

    movdqa  [rdx], xmm0
    ret