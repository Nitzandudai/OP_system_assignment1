#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
    printf("=== Testing co_yield error conditions ===\n");

    int mypid = getpid();

    // Test (a): non-existent PID
    int ret = co_yield(9999, 1);
    printf("(a) Yield to non-existent PID: %s\n", ret == -1 ? "PASS" : "FAIL");

    // Test (c): self-yield
    ret = co_yield(mypid, 1);
    printf("(c) Self-yield: %s\n", ret == -1 ? "PASS" : "FAIL");

    // Test (b): killed process
    int pid = fork();
    if (pid == 0) {
        // child just exits immediately
        exit(0);
    }
    wait(0);  // wait for child to die
    sleep(1); // give kernel time to clean up
    ret = co_yield(pid, 1);
    printf("(b) Yield to killed process: %s\n", ret == -1 ? "PASS" : "FAIL");

    printf("\n=== Testing normal co_yield ===\n");

    int pid1 = getpid();
    int pid2 = fork();
    if (pid2 == 0) { // Child
        for (int i = 0; i < 10; i++) {
            int value = co_yield(pid1, 1);
            printf("Child received: %d\n", value);
        }
    } else { // Parent
        for (int i = 0; i < 7; i++) {
            int value = co_yield(pid2, 2);
            printf("parent received: %d\n", value);
        }
    }
    exit(0);
}