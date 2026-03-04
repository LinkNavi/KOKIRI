bits 64

; void enter_usermode(uint64_t rip, uint64_t rsp, uint64_t rflags)
; rdi = user RIP, rsi = user RSP, rdx = user RFLAGS
global enter_usermode
enter_usermode:
    ; set up iretq frame
    push qword 0x1B        ; user SS  (0x18 | 3)
    push rsi                ; user RSP
    push rdx                ; user RFLAGS
    push qword 0x23        ; user CS  (0x20 | 3)
    push rdi                ; user RIP

    ; zero all GP registers to not leak kernel data
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rsi, rsi
    xor rdi, rdi
    xor rbp, rbp
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15

    iretq

section .note.GNU-stack noalloc noexec nowrite progbits
