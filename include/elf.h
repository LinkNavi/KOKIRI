#pragma once
#include <stdint.h>
#include "process.h"

// ELF64 types
#define EI_NIDENT 16
#define ET_EXEC   2
#define EM_X86_64 62
#define PT_LOAD   1
#define PT_INTERP 3
#define PT_TLS    7
#define PF_X      1
#define PF_W      2
#define PF_R      4

typedef struct {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf64_ehdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf64_phdr_t;

// Load a statically-linked ELF64 into the given process
// data: pointer to ELF file in memory, size: file size
// Returns entry point, or 0 on failure
uint64_t elf_load(process_t *proc, const uint8_t *data, uint64_t size);
