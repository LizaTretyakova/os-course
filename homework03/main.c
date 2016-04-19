#include "kmem_cache.h"
#include "interrupt.h"
#include "memory.h"
#include "serial.h"
#include "paging.h"
#include "stdio.h"
#include "misc.h"
#include "time.h"
#include "threads.h"
#include "fs.h"
#include "fs_utils.h"

#include <sys/types.h>

volatile pid_t is_set_threading;

static struct fs_node* create_dir(char* name) {
    return mkdir(name);
}

static struct fs_node* create_and_list_empty() {
    char* name = "/abc";
    struct fs_node a[10];
    struct fs_node* dir = create_dir(name);
    readdir(dir, a, 10, 0);
    return dir;
}

static void create_and_list() {
    struct fs_node* dir = create_and_list_empty();
    struct fs_node a[10];

    char* filename = "/abc/file";
    char* dirname = "/abc/dir";
    //struct fs_node* file =
    open(filename);
    //struct fs_node* subdir =
    mkdir(dirname);

    readdir(dir, a, 10, 0);
}


void main(void)
{
    is_set_threading = FALSE;

	setup_serial();
	setup_misc();
	setup_ints();
	setup_memory();
	setup_buddy();
	setup_paging();
	setup_alloc();
    setup_threading();
	setup_time();
    local_irq_enable();

    init_fs();
    create_and_list();

    printf("Still alive!\n");
	while (1);
}
