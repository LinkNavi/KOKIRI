#include "../include/elf.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include "../include/vga.h"
#include "../include/string.h"

uint64_t elf_load(process_t *proc, const uint8_t *data, uint64_t size) {
    if (size < sizeof(elf64_ehdr_t)) {
        vga_puts("[ELF] Too small\n");
        return 0;
    }

    const elf64_ehdr_t *ehdr = (const elf64_ehdr_t *)data;

    // Validate magic
    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' ||
        ehdr->e_ident[2] != 'L'  || ehdr->e_ident[3] != 'F') {
        vga_puts("[ELF] Bad magic\n");
        return 0;
    }

    if (ehdr->e_ident[4] != 2) { // EI_CLASS = 64-bit
        vga_puts("[ELF] Not 64-bit\n");
        return 0;
    }

    if (ehdr->e_machine != EM_X86_64) {
        vga_puts("[ELF] Not x86_64\n");
        return 0;
    }

    // Load PT_LOAD segments
    uint64_t highest_addr = 0;
    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        const elf64_phdr_t *phdr = (const elf64_phdr_t *)(data + ehdr->e_phoff + i * ehdr->e_phentsize);

        if (phdr->p_type != PT_LOAD) continue;

        uint64_t vaddr_start = phdr->p_vaddr & ~(uint64_t)(PAGE_SIZE - 1);
        uint64_t vaddr_end   = (phdr->p_vaddr + phdr->p_memsz + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);

        uint64_t flags = PAGE_PRESENT | PAGE_USER;
        if (phdr->p_flags & PF_W) flags |= PAGE_WRITE;

        // Allocate and map pages
        for (uint64_t addr = vaddr_start; addr < vaddr_end; addr += PAGE_SIZE) {
            void *phys = pmm_alloc();
            if (!phys) {
                vga_puts("[ELF] Out of memory\n");
                return 0;
            }
            memset((void *)(uintptr_t)phys, 0, PAGE_SIZE);
            vmm_map(proc->pagemap, addr, (uint64_t)(uintptr_t)phys, flags);
        }

        // Copy file data
        if (phdr->p_filesz > 0) {
            if (phdr->p_offset + phdr->p_filesz > size) {
                vga_puts("[ELF] Truncated\n");
                return 0;
            }

            // Copy page by page using physical addresses
            uint64_t file_off = phdr->p_offset;
            uint64_t vaddr = phdr->p_vaddr;
            uint64_t remaining = phdr->p_filesz;

            while (remaining > 0) {
                uint64_t page_offset = vaddr & (PAGE_SIZE - 1);
                uint64_t phys = vmm_virt_to_phys(proc->pagemap, vaddr & ~(uint64_t)(PAGE_SIZE - 1));
                if (!phys) {
                    vga_puts("[ELF] Map failed\n");
                    return 0;
                }
                // Since we're identity-mapped in kernel, phys == usable ptr
                uint64_t chunk = PAGE_SIZE - page_offset;
                if (chunk > remaining) chunk = remaining;
                memcpy((void *)(uintptr_t)(phys + page_offset), data + file_off, chunk);
                file_off += chunk;
                vaddr += chunk;
                remaining -= chunk;
            }
        }

        if (vaddr_end > highest_addr) highest_addr = vaddr_end;
    }

    // Set brk to end of loaded segments (page-aligned)
    proc->brk = highest_addr;
    proc->brk_start = highest_addr;

    vga_puts("[ELF] Loaded, entry=");
    // print hex
    const char *hex = "0123456789ABCDEF";
    vga_puts("0x");
    for (int i = 60; i >= 0; i -= 4)
        vga_putchar(hex[(ehdr->e_entry >> i) & 0xF]);
    vga_puts("\n");

    return ehdr->e_entry;
}
