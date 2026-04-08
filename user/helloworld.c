#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
    write(1, "Hello World xv6\n", 16);
    exit(0);
}
