#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  int parent_pid = getpid();
  int child_pid = fork();

  if(child_pid < 0){
    printf("fork failed\n");
    exit(1);
  }

  if(child_pid == 0){
    for(;;){
      int value = co_yield(parent_pid, 1);
      printf("child received: %d\n", value);
    }
  } else {
    for(;;){
      int value = co_yield(child_pid, 2);
      printf("parent received: %d\n", value);
    }
  }

  exit(0);
}