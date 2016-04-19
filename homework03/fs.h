#ifndef _FS_
#define _FS_

#include "rbtree.h"
#include "threads.h"
#include "list.h"

#define ROOT_NAME ""
#define DELIMETER '/'

typedef enum {DIR, REG} node_t;
extern struct fs_tree tree;
extern lock_descriptor fs_lock;

struct list {
    struct list_head* root;
};

struct fs_node {
    struct list_head node;
    char* name;

    // DIR or REG
    node_t node_type;

    // if node_type is DIR, then content == NULL
    // if node_type is REG, then children.root == NULL
    void* content;
    uint64_t content_size;

    struct list_head children;
};

struct fs_tree {
    struct fs_node root;
};

struct fs_node* open(char* name);
struct fs_node* mkdir(char* name);
void close(char* name);
void read(struct fs_node* file, void* buffer, uint64_t size, uint64_t offset);
void write(struct fs_node* file, void* buffer, uint64_t size, uint64_t offset);
int readdir(struct fs_node* dir, struct fs_node* buffer, uint64_t size, uint64_t offset);


#endif
