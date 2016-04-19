#ifndef _MAP_
#define _MAP_

#include "fs.h"
#include "threads.h"

#define MAGIC_START "070701"

struct multiboot_mod_list
 {
   /* the memory used goes from bytes 'mod_start' to 'mod_end-1' inclusive */
   multiboot_uint32_t mod_start;
   multiboot_uint32_t mod_end;

   /* Module command line */
   multiboot_uint32_t cmdline;

   /* padding to take it to 16 bytes (must be zero) */
   multiboot_uint32_t pad;
 };
 typedef struct multiboot_mod_list multiboot_module_t;



//void insert_node(struct list_head* node, struct list_head** dest);
struct fs_node* lookup_file(char* name, node_t type, struct fs_node* src);
void init_fs_node(struct fs_node** result, char* name, node_t type);
void init_fs();
char* get_dirname(char* filename);
//(struct fs_tree* tree, struct lock_descriptor* fs_lock);

#endif
