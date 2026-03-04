bits 64

; syscall entry point
; Linux x86_64 ABI: rax=syscall#, rdi=arg1, rsi=arg2, rdx=arg3, r10=arg4, r8=arg5, r9=arg6
; On syscall: RCX=saved RIP, R11=saved RFLAGS, RSP is NOT saved by CPU

global syscall_entry
extern syscall_dispatch

section .data
align 16
kernel_stack_save: dq 0

section .text

syscall_entry:
    ; swap to kernel stack
    ; save user rsp in scratch register
    mov [rel kernel_stack_save], rsp

    ; load kernel rsp from TSS rsp0 — we stored it in a known location
    extern tss_rsp0_ptr
    mov rsp, [rel tss_rsp0_ptr]

    ; build a trap frame on the kernel stack
    push qword 0x1B        ; user SS (0x18 | 3)
    push qword [rel kernel_stack_save]  ; user RSP
    push r11                ; user RFLAGS (saved by syscall)
    push qword 0x23        ; user CS (0x20 | 3)
    push rcx                ; user RIP (saved by syscall)

    ; save callee-saved + args
    push rax                ; syscall number
    push rdi
    push rsi
    push rdx
    push r10                ; arg4 (r10, not rcx — rcx clobbered by syscall)
    push r8
    push r9
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; call C handler: int64_t syscall_dispatch(uint64_t num, uint64_t a1..a6)
    ; args already in place: rax=num, rdi=a1, rsi=a2, rdx=a3, r10=a4, r8=a5, r9=a6
    ; re-arrange for C calling convention
    mov rdi, rax            ; arg1 = syscall number
    ; rsi already = arg2 (user's rsi was original arg2)
    ; but we pushed everything, so restore from stack
    mov rsi, [rsp + 13*8]  ; original rdi (user arg1)
    mov rdx, [rsp + 12*8]  ; original rsi (user arg2)
    mov rcx, [rsp + 11*8]  ; original rdx (user arg3)
    mov r8,  [rsp + 10*8]  ; original r10 (user arg4)
    mov r9,  [rsp + 9*8]   ; original r8  (user arg5)
    ; arg7 (r9) would go on stack but we don't use 7-arg syscalls

    call syscall_dispatch

    ; rax = return value

    ; restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop r9
    pop r8
    pop r10
    pop rdx
    pop rsi
    pop rdi
    add rsp, 8             ; skip saved rax

    ; restore user RIP and RFLAGS for sysret
    pop rcx                ; user RIP
    add rsp, 8             ; skip CS
    pop r11                ; user RFLAGS
    pop rsp                ; user RSP (skip SS implicitly)

    ; sysretq returns to user mode
    ; rcx = user RIP, r11 = user RFLAGS
    sysretq

section .note.GNU-stack noalloc noexec nowrite progbits
