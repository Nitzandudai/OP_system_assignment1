#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
    int before, after_alloc, after_free;
    char *ptr;

    // (a) לפני
    before = memsize();
    printf("Before allocation: %d bytes\n", before);

    // (b) הקצאה של 20KB
    ptr = malloc(20000);

    // (c) אחרי הקצאה
    after_alloc = memsize();
    printf("After allocation: %d bytes\n", after_alloc);

    // (d) שחרור
    free(ptr);

    // (e) אחרי שחרור
    after_free = memsize();
    printf("After free: %d bytes\n", after_free);

    exit(0);
}