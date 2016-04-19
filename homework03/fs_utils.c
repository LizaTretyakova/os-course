#include "string.h"
#include "fs.h"
#include "fs_utils.h"
#include "stdlib.h"
#include "rbtree.h"
#include "memory.h"
#include "kmem_cache.h"
#include "stdio.h"
#include "threads.h"
#include "list.h"

#include <stddef.h>

struct fs_tree tree;
lock_descriptor fs_lock;

void init_fs() {
    printf("Before lock init\n");
    fs_lock.is_occupied = FALSE;

    printf("Before tree init\n");
    tree.root.name = ROOT_NAME;
    printf("Node_type?\n");
    tree.root.node_type = DIR;
    printf("Content?\n");
    tree.root.content = kmem_alloc(2 * PAGE_SIZE);
    printf("Content_size?\n");
    tree.root.content_size = 0;
    printf("Children?\n");
    tree.root.children = NULL;

    printf("Returning from init_fs\n");
}

struct fs_node* lookup_file(char* name, node_t type, struct fs_node* src) {
    printf("Entered lookup_file for %s\n", name);

    if(strcmp(name, ROOT_NAME) == 0) {
        return &(tree.root);
    }
    printf("We are not a root\n");

    if(src == NULL) {
        return NULL;
    }
    printf("Src is not NULL\n");

    if(strcmp(name, src->name) == 0 && src->node_type == type) {
        return src;
    }
    printf("It's not the node we are looking for\n");

    if(src->node_type == REG) {
        return NULL;
    }
    printf("Src is not a reg_file, it is %s\n", src->name);


    struct fs_node* cur = (struct fs_node*)(src->children);
    printf("Cur is %p\n", cur);
    if(cur == NULL) {
        return NULL;
    }
    for(uint64_t i = 0; i < list_size(src->children); ++i) {
        struct fs_node* subtree_res = lookup_file(name, type, cur);
        if(subtree_res != NULL) {
            return subtree_res;
        }
    }
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
    (*result)->children = NULL;
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

void insert_node(struct list_head* node, struct list_head** head) {
    if(node == NULL) {
        printf("Node should not be NULL\n");
        return;
    }

    if(*head == NULL) {
        (*head) = node;
        list_init(*head);
    } else {
        list_add(node, *head);
    }
}

