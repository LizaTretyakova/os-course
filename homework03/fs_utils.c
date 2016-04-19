#include "string.h"
#include "fs.h"
#include "fs_utils.h"
#include "stdlib.h"
#include "memory.h"
#include "kmem_cache.h"
#include "stdio.h"
#include "threads.h"
#include "list.h"
#include "initramfs.h"

#include <stddef.h>

struct fs_tree tree;
lock_descriptor fs_lock;

void init_fs() {
    printf("Init_fs\n");

    fs_lock.is_occupied = FALSE;

    extern const uint32_t mboot_info;
    struct multiboot_info *mboot_header = (void *)((uintptr_t)mboot_info);

    if(!((1 << 3) & mboot_header->flags)) {
        printf("Invalid mboot_header -- halting\n");
        while(1);
    }

    multiboot_module_t* modules = (void*)((uintptr_t)(mboot_header->mods_addr));
    for(uint64_t i = 0; i < mboot_header->mods_count; ++i) {
        if((modules + i)->mod_end - (modules + i)->mod_start >= sizeof(cpio_header)) {
            // compare with MAGIC and further
        }
    }


    tree.root.name = ROOT_NAME;
    tree.root.node_type = DIR;
    tree.root.content = kmem_alloc(2 * PAGE_SIZE);
    tree.root.content_size = 0;
    list_init(&(tree.root.children));

    printf("Returning from init_fs\n");
}

struct fs_node* lookup_file(char* name, node_t type, struct fs_node* src) {
    printf("Entered lookup_file for %s\n", name);

    if(strcmp(name, ROOT_NAME) == 0) {
        printf("lookup We are (G)root\n");
        return &(tree.root);
    }
//    printf("We are not a root\n");

    if(src == NULL) {
        printf("lookup Passed NULL\n");
        return NULL;
    }
//    printf("Src is not NULL\n");

    printf("***%s***%s***\n", name, src->name);
    if(type == REG) {
        printf("***REG***");
    } else {
        printf("***DIR***");
    }
    if(src->node_type == REG) {
        printf("REG***\n");
    } else {
        printf("DIR***\n");
    }
    if(strcmp(name, src->name) == 0 && src->node_type == type) {
        printf("lookup We found what we wanted\n");
        return src;
    }
//    printf("It's not the node we are looking for\n");

    if(src->node_type == REG || (src->node_type == DIR && list_empty(&(src->children)))) {
        printf("lookup We have not found and are ni a leaf\n");
        return NULL;
    }
//    printf("Src is not a reg_file, it is %s\n", src->name);

//    printf("Cur is %p\n", cur);
    for (struct list_head* cur = src->children.next; cur != &(src->children); cur = cur->next) {
        struct fs_node* subtree_res = lookup_file(name, type, (struct fs_node*)cur);
        if(subtree_res != NULL) {
            printf("lookup Found in a subtree\n");
            return subtree_res;
        }
    }
    printf("lookup Nothing found\n");
    return NULL;
}

void init_fs_node(struct fs_node** result, char* name, node_t type) {
    (*result) = kmem_alloc(sizeof(struct fs_node));
    if(*result == NULL) {
        printf("Failed to allocate memory for a new file");
        while(1);
    }

    (*result)->name = name;
    (*result)->node_type = type;

    if(type == REG) {
        // allocate memory for the file
        (*result)->content = kmem_alloc(2 * PAGE_SIZE);
    } else if(type == DIR) {
        // which content? =O_o=
        (*result)->content = NULL;
    }
    list_init(&((*result)->children));
}

char* get_dirname(char* filename) {
    int i = (int)strlen(filename);

    // let's root be '/'
    while(i >= 0 && filename[i] != DELIMETER) {
        --i;
    }

    char* result = kmem_alloc(i * sizeof(char));
    memcpy(result, filename, i);
    result[i] = 0;

    return result;
}

/*
void insert_node(struct list_head* node, struct list_head** head) {
    if(node == NULL || *head == NULL) {
        printf("Neither node or head should be NULL\n");
        return;
    }

    if(*head == NULL) {
        (*head) = node;
        list_init(*head);
    } else {
        list_add(node, *head);
    }
}
*/
