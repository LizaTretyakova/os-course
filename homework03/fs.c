#include "string.h"
#include "fs.h"
#include "stdlib.h"
#include "rbtree.h"
#include "kmem_cache.h"
#include "stdio.h"
#include "fs_utils.h"
#include "threads.h"
#include "list.h"

#include <stddef.h>


struct fs_tree tree;
lock_descriptor fs_lock;

// looks for a file with a specified name and type in the fs_tree;
// if it's present, returns its fs_node;
// if not, creates it and also returns its fs_node
struct fs_node* fs_helper(char* name, node_t type) {
    printf("Entered fs_helper with name %s\n", name);

    uint64_t rflags = lock(&fs_lock);
    struct fs_node* result = lookup_file(name, type, (struct fs_node*)(tree.root.children));
    unlock(&fs_lock, rflags);

    printf("Looked for the node with name %s: %p\n", name, result);

    if(result == NULL) {
        init_fs_node(&result, name, type);
        char* dirname = get_dirname(name);
        struct fs_node* containing_dir = fs_helper(dirname, type);
        kmem_free(dirname);

        printf("Containing dir: %p name: %s children: %p\n", containing_dir, containing_dir->name, containing_dir->children);
        rflags = lock(&fs_lock);
        insert_node(&(result->node), &(containing_dir->children));
        unlock(&fs_lock, rflags);
        printf("After inserting children is %p\n", containing_dir->children);
    }

    return result;
}

struct fs_node* open(char* name) {
    printf("Open a file with name %s\n", name);
    return fs_helper(name, REG);
}

struct fs_node* mkdir(char* name) {
    printf("Mkdir for name %s\n", name);
    return fs_helper(name, DIR);
}

void close (char* name) {
    (void)name;
}

void read(struct fs_node* file, void* buffer, uint64_t size, uint64_t offset) {
    if(file == NULL || buffer == NULL) {
        printf("Passed a NULL file/buffer");
        return;
    }

    uint64_t rflags = lock(&fs_lock);
    uint64_t len =
            (file->content_size - offset) < size ? file->content_size : size;
    memcpy(buffer, (char*)(file->content) + offset, len);
    unlock(&fs_lock, rflags);
}

void write(struct fs_node* file, void* buffer, uint64_t size, uint64_t offset) {
    if(file == NULL || buffer == NULL) {
        printf("Passed a NULL file/buffer");
        return;
    }

    uint64_t rflags = lock(&fs_lock);
    memcpy((char*)(file->content) + offset, buffer, size);

    if(file->content_size < offset + size) {
        file->content_size = offset + size;
    }
    unlock(&fs_lock, rflags);
}

int readdir(struct fs_node* dir, struct fs_node* buffer, uint64_t size, uint64_t offset) {
    if(dir == NULL || dir->children == NULL || buffer == NULL) {
        printf("Passed an empty dir/buffer\n");
        return 0;
    }

    uint64_t rflags = lock(&fs_lock);
    struct fs_node* cur = (struct fs_node*)(dir->children);
    int res = 0;
    printf("A part of listing the directory %s:\n", dir->name);
    for(uint64_t i = 0; i < list_size(dir->children); i++) {
        if(cur->node_type == REG) {
            printf("REG %s\n", cur->name);
        } else {
            printf("DIR %s\n", cur->name);
        }

        if(i >= offset && i < offset + size) {
            buffer[i - offset] = *cur;
            ++res;
        }
        cur = (struct fs_node*)(cur->node.next);
    }
    unlock(&fs_lock, rflags);

    return res;
}

