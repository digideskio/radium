#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "kernel_page.h"
#include "multiboot.h"
#include "paging.h"
#include "panic.h"
#include "string.h"
#include "task.h"
#include "types.h"

static multiboot_info_t* mb;

static multiboot_module_t*
find_module(const char* name)
{
    multiboot_module_t* mods = (void*)mb->mods_addr;

    for(size_t i = 0; i < mb->mods_count; i++) {
        if(streq((const char*)mods[i].cmdline, name)) {
            return &mods[i];
        }
    }

    return NULL;
}

void
kmain(multiboot_info_t* mb_, uint32_t magic)
{
    (void)magic;
    mb = mb_;

    interrupts_disable();

    console_init();

    printf("Radium booting from %s.\n", (const char*)mb->boot_loader_name);

    for(size_t i = 0; i < mb->mods_count; i++) {
        multiboot_module_t* mods = (void*)mb->mods_addr;
        paging_set_allocatable_start(mods[i].mod_end);
    }

    gdt_init();
    idt_init();
    paging_init(mb);

    task_init();

    printf("Booted.\n");

    task_t a, b;
    task_new(&a, "a");
    task_new(&b, "b");

    interrupts_enable();

    printf("sizeof(tss_t) == %d\n", sizeof(tss_t));

    multiboot_module_t* mod = find_module("/init.bin");
    page_map(USER_BEGIN, mod->mod_start, PE_PRESENT | PE_USER);

    printf("first char: %d\n", (int)*(uint8_t*)USER_BEGIN);

    while(1) {
        __asm__ volatile("hlt");
    }
}
