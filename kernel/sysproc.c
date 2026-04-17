#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

extern struct proc proc[NPROC];

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_memsize(void)
{
  return myproc()->sz;
}

uint64
sys_co_yield(void)
{
  int pid, value;
  struct proc *p = myproc();
  struct proc *target = 0;
  struct proc *first, *second;

  argint(0, &pid);
  argint(1, &value);

  if(pid <= 0 || pid == p->pid)
    return -1;

  for(struct proc *q = proc; q < &proc[NPROC]; q++) {
    acquire(&q->lock);
    if(q->pid == pid) {
      target = q;
      break;
    }
    release(&q->lock);
  }

  if(target == 0)
    return -1;

  if(target->killed || target->state == UNUSED || target->state == ZOMBIE) {
    release(&target->lock);
    return -1;
  }

  release(&target->lock);

  first = p < target ? p : target;
  second = p < target ? target : p;
  acquire(&first->lock);
  acquire(&second->lock);

  if(target->killed || target->state == UNUSED || target->state == ZOMBIE) {
    release(&second->lock);
    release(&first->lock);
    return -1;
  }

  if(target->state == SLEEPING && target->chan == (void *)p) {
    target->trapframe->a0 = value;
    p->chan = (void *)target;
    p->state = SLEEPING;
    release(&target->lock);
    cohandoff(p, target);
    p->chan = 0;
    if(killed(p))
      return -1;
    return p->trapframe->a0;
  }

  p->chan = (void *)target;
  p->state = SLEEPING;
  release(&target->lock);
  sched();
  p->chan = 0;
  if(killed(p))
    return -1;
  return p->trapframe->a0;
}