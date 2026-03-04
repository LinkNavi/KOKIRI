#pragma once
#include <stdint.h>

void gdt_init(void);
void gdt_install_tss(void);
