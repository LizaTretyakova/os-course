#ifndef _MAP_
#define _MAP_

#include "fs.h"
#include "threads.h"

void insert_node(struct list_head* node, struct list_head** dest);
struct fs_node* lookup_file(char* name, node_t type, struct fs_node* src);
void init_fs_node(struct fs_node** result, char* name, node_t type);
void init_fs();
char* get_dirname(char* filename);
//(struct fs_tree* tree, struct lock_descriptor* fs_lock);

#endif
