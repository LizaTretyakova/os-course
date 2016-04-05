#include "kmem_cache.h"
#include "interrupt.h"
#include "memory.h"
#include "serial.h"
#include "paging.h"
#include "stdio.h"
#include "misc.h"
#include "time.h"
#include "threads.h"

#include <sys/types.h>

volatile pid_t is_set_threading;

/*
static bool range_intersect(phys_t l0, phys_t r0, phys_t l1, phys_t r1)
{
	if (r0 <= l1)
		return false;
	if (r1 <= l0)
		return false;
	return true;
}

static void buddy_smoke_test(void)
{
#define PAGES 10
	struct page *page[PAGES];
	int order[PAGES];

	for (int i = 0; i != PAGES; ++i) {
		page[i] = alloc_pages(i);
		if (page[i]) {
			const phys_t begin = page_paddr(page[i]);
			const phys_t end = begin + (PAGE_SIZE << i);
			printf("allocated [%#llx-%#llx]\n", begin, end - 1);
			order[i] = i;
		}
	}

	for (int i = 0; i != PAGES - 1; ++i) {
		if (!page[i])
			break;
		for (int j = i + 1; j != PAGES; ++j) {
			if (!page[j])
				break;

			const phys_t ibegin = page_paddr(page[i]);
			const phys_t iend = ibegin + (PAGE_SIZE << order[i]);

			const phys_t jbegin = page_paddr(page[j]);
			const phys_t jend = jbegin + (PAGE_SIZE << order[j]);

			DBG_ASSERT(!range_intersect(ibegin, iend, jbegin, jend));
		}
	}

	for (int i = 0; i != PAGES; ++i) {
		if (!page[i])
			continue;

		const phys_t begin = page_paddr(page[i]);
		const phys_t end = begin + (PAGE_SIZE << i);
		printf("freed [%#llx-%#llx]\n", begin, end - 1);
		free_pages(page[i], order[i]);
	}
#undef PAGES
}

struct intlist {
	struct list_head link;
	int data;
};

static void slab_smoke_test(void)
{
#define ALLOCS 1000000
	struct kmem_cache *cache = KMEM_CACHE(struct intlist);
	LIST_HEAD(head);
	int i;

	for (i = 0; i != ALLOCS; ++i) {
		struct intlist *node = kmem_cache_alloc(cache);

		if (!node)
			break;
		node->data = i;
		list_add_tail(&node->link, &head);
	}

	printf("Allocated %d nodes\n", i);

	while (!list_empty(&head)) {
		struct intlist *node = LIST_ENTRY(list_first(&head),
					struct intlist, link);

		list_del(&node->link);
		kmem_cache_free(cache, node);
	}

	kmem_cache_destroy(cache);
#undef ALLOCS
}
//*/
//*
void print_meow(void* unused_arg) {
    (void) unused_arg;
    printf("Meow! =^^=");
}

void print_meow_xx(void* unused_arg) {
    (void) unused_arg;
    printf("Meow! =x_x=");
    //while(1);
}

static void create_thread_test(thread **a, thread **b) {
    (*a) = create_thread(print_meow, NULL);
    (*b) = create_thread(print_meow_xx, NULL);
}

static void create_n_join_test() {
    thread* a;
    thread* b;
    create_thread_test(&a, &b);
    exit(a);
    exit(b);
    join(a);
    join(b);
}
//*/
//*
void fork_join_and_die(void* arg) {
    uint64_t i = (uint64_t)arg;
    if (i > 0) {
        printf("I am creating a thread, my i = %d\n", i);
        --i;
        thread* child = create_thread(fork_join_and_die, (void*)i);
        printf("Joining child, I am: %d\n", i + 1);
        join(child);
        printf("Child dead, so do I\n");
    } else {
        printf("i is <= 0, I am useless\n");
    }
}

static void cascade_test(uint64_t a) {
    uint64_t i = a;
    printf("Enter cascade thread\n");
    thread* first = create_thread(fork_join_and_die, (void*)i);
    printf("Joining first\n");
    join(first);
    printf("Joined first\n");
}
//*/
//*
void print_many(void* unused_arg) {
    (void) unused_arg;
    for (int i = 0; i < 100000; ++i) {
        printf("%d\n", i);
    }
}

static void many_test() {
    thread* first = create_thread(print_many, NULL);
    thread* second = create_thread(print_many, NULL);
    thread* third = create_thread(print_many, NULL);
    join(first);
    join(second);
    join(third);

    printf("Joined the three\n");
}
//*/
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

//    buddy_smoke_test();
//    slab_smoke_test();

    many_test();
    create_n_join_test();
    cascade_test(3);

    printf("Still alive!\n");
	while (1);
}
