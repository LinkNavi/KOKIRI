; KOKIRI - let GRUB handle protected mode, we just set up long mode
MB2_MAGIC    equ 0xE85250D6
MB2_ARCH     equ 0
MB2_LEN      equ mb2_end - mb2_start
MB2_CHECKSUM equ -(MB2_MAGIC + MB2_ARCH + MB2_LEN)

section .multiboot2
align 8
mb2_start:
    dd MB2_MAGIC
    dd MB2_ARCH
    dd MB2_LEN
    dd MB2_CHECKSUM
    ; end tag
    align 8
    dw 0
    dw 0
    dd 8
mb2_end:

section .bss
align 16
mb_magic: resd 1
mb_info:  resd 1
stack_bottom:
    resb 32768
stack_top:

align 4096
pml4_table:  resb 4096
pdp_table:   resb 4096
pd_table:    resb 4096

section .rodata
align 8
gdt64:
    dq 0                                        ; null
.code: equ $ - gdt64
    dq (1<<43)|(1<<44)|(1<<47)|(1<<53)          ; 64-bit code
.data: equ $ - gdt64
    dq (1<<41)|(1<<44)|(1<<47)                  ; 64-bit data
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .text
bits 32
global _start
extern kernel_main

_start:
    mov esp, stack_top

    ; save multiboot2 values before they get clobbered
    mov [mb_magic], eax
    mov [mb_info],  ebx

    ; clear page tables
    mov edi, pml4_table
    mov ecx, 3*4096/4
    xor eax, eax
    rep stosd

    ; pml4[0] -> pdp
    mov eax, pdp_table
    or  eax, 3
    mov [pml4_table], eax

    ; pdp[0] -> pd
    mov eax, pd_table
    or  eax, 3
    mov [pdp_table], eax

    ; map 512 x 2MB pages = 1GB identity map
    mov ecx, 0
.map:
    mov eax, 0x200000
    mul ecx
    or  eax, 0x83           ; present + writable + huge
    mov [pd_table + ecx*8], eax
    inc ecx
    cmp ecx, 512
    jne .map

    ; load PML4
    mov eax, pml4_table
    mov cr3, eax

    ; enable PAE
    mov eax, cr4
    or  eax, (1<<5)
    mov cr4, eax

    ; set LME
    mov ecx, 0xC0000080
    rdmsr
    or  eax, (1<<8)
    wrmsr

    ; enable paging + protected mode
    mov eax, cr0
    or  eax, (1<<31)|(1<<0)
    mov cr0, eax

    ; load 64-bit GDT and far jump
    lgdt [gdt64.pointer]
    jmp  gdt64.code:long_mode

bits 64
long_mode:
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; restore saved multiboot2 values
    mov edi, [mb_magic]
    mov esi, [mb_info]
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang
