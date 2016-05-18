#include "elf.h"
#include "vfs.h"
#include "stdio.h"
#include "paging.h"
#include "string.h"
#include "memory.h"
#include "threads.h"
#include "kmem_cache.h"
#include "thread_regs.h"

static void read(struct fs_file* file, void* buffer, int offset, int cnt) {
    int res = 0;
    vfs_seek(file, offset, FSS_SET);
    while(res < cnt) {
        res += vfs_read(file, buffer, cnt);
    }
}

void read_programm_header(struct elf_phdr elf_phdr, struct fs_file file, phys_t pml4_paddr, pte_t* pml4) {
    printf("Reading program headers\nMemsize: %llu\nFilesize: %llu\n", (unsigned long long)elf_phdr.p_memsz, (unsigned long long)elf_phdr.p_filesz);

    // map [p_vaddr .. p_vaddr + p_memsz] to phys with [p_offset .. p_offset + p_filesz]
    // so, populating
    const int page_num = (elf_phdr.p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
//    const uint64_t p_vaddr_end = elf_phdr.p_vaddr + page_num * PAGE_SIZE;
//    const uint64_t p_vaddr_end = elf_phdr.p_vaddr + page_num * PAGE_SIZE;
    const uint64_t p_vaddr_start = (elf_phdr.p_vaddr / PAGE_SIZE) * PAGE_SIZE;
    const uint64_t p_vaddr_end = p_vaddr_start + (page_num + 1) * PAGE_SIZE;
    //const pte_t page_flags = PTE_WRITE | PTE_PRESENT;

    const int rc = pt_populate_range(
                pml4,
                p_vaddr_start,
                p_vaddr_end
    );

    if (rc) {
        printf("Error in mapping creating for the elf file");
        while(1);
    }

    // mapping the lower layers
    struct pt_iter iter;
    for_each_slot_in_range(pml4, p_vaddr_start, p_vaddr_end, iter) {
        const int level = iter.level;
        const int index = iter.idx[level];
        pte_t *pt = iter.pt[level];

        if(level > 0) {
            printf("Lost somewhere in this table forest O,o");
            while(1);
        }

        struct page* page = alloc_pages(0);
        phys_t p_page_addr = page_paddr(page);
        pt[index] = p_page_addr | PTE_PT_FLAGS | PTE_PRESENT;
    }

    // to be able to use memcpy
    store_pml4(pml4_paddr);

    //copying the necessary content
    void* content = kmem_alloc(elf_phdr.p_filesz);
    read(&file, content, elf_phdr.p_offset, elf_phdr.p_filesz);
    memcpy((void*)elf_phdr.p_vaddr/*it should be like this, shouldn't be aligned*/, content, elf_phdr.p_filesz);
    printf("CHECK\n");
    if(elf_phdr.p_filesz > 0) {
        printf("First int of content: %d\nFirst int of written: %d\n", *(int*)content, *(int*)elf_phdr.p_vaddr);
    } else {
        printf("This is zero: %llu\n", (unsigned long long)elf_phdr.p_filesz);
    }
    kmem_free(content);
}

void prepare_stack(struct elf_hdr elf_header, phys_t pml4_paddr, pte_t* pml4) {
    // getting regs, allocating memory for stack
    struct thread_regs* regs = thread_regs(current());
    struct page* stack = alloc_pages(0);
    if(!stack) {
        printf("Not enough memory for executing the file.");
        exit();
    }

    // getting the first free slot and mapping it to the allocated memory
    struct pt_iter iter;
//  struct pt_iter* slot = pt_iter_set(&(iter), pml4, 2 * PAGE_SIZE);
    struct pt_iter* slot = pt_iter_set(&(iter), pml4, 0);
    const int level = slot->level;
    const int index = slot->idx[level];
    pte_t *pt = slot->pt[level];
    if(level > 0) {
        printf("Lost somewhere in this table forest O,o");
        while(1);
    }
    phys_t p_stack_addr = page_paddr(stack);
    pt[index] = p_stack_addr | PTE_PT_FLAGS | PTE_PRESENT;
    store_pml4(pml4_paddr);

    // setting regs
    regs->cs = USER_CODE;
    regs->ss = USER_DATA;
    // writing the end of the allocated stack
    regs->rsp = (uint64_t)((char*)(slot->addr) + PAGE_SIZE);
    regs->rip = elf_header.e_entry;
}

int read_elf(void* a) {
    (void)a;

    struct fs_file file;
    struct elf_hdr elf_header;
    struct elf_phdr elf_phdr;

    vfs_open("/initramfs/hello", &file);
    read(&file, &elf_header, 0, sizeof(elf_header));
    phys_t pml4_paddr = load_pml4();
    pte_t* pml4 = va(pml4_paddr);

    printf("Reading the elf file:\nEntry point address: %llu\nNumber of program headers: %llu\n", (unsigned long long)elf_header.e_entry, (unsigned long long)elf_header.e_phnum);
    //uint16_t phentsize = elf_header.e_phentsize;

    for(int i = 0; i < elf_header.e_phnum; ++i) {
        read(&file, &elf_phdr, sizeof(elf_header) + i * elf_header.e_phentsize, sizeof(elf_phdr));
        if(elf_phdr.p_type == PT_LOAD) {
            read_programm_header(elf_phdr, file, pml4_paddr, pml4);
        }
    }
    vfs_release(&file);

    prepare_stack(elf_header, pml4_paddr, pml4);

    printf("read_elf ended\n");
    return 0;
}

pid_t run_elf_new_thread() {
    const pid_t pid = create_kthread(&read_elf, 0);
    return pid;
}
