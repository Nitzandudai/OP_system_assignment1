#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
extern struct spinlock wait_lock;
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
sys_co_yield(void)
{
  int target_pid;
  int value;
  struct proc *p = myproc();
  struct proc *target = 0;
  struct proc *pp;

  argint(0, &target_pid);
  argint(1, &value);

  if(target_pid <= 0 || target_pid == p->pid)
    return -1;

  acquire(&wait_lock);

  // Find the target process. Ignore unused or exited processes.
  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->pid == target_pid && pp->state != UNUSED && pp->state != ZOMBIE){
      target = pp;
      break;
    }
  }

  if(target == 0 || target->state == ZOMBIE || target->killed){
    release(&wait_lock);
    return -1;
  }

  // Direct handoff: target is sleeping on co_chan (inside co_yield), or runnable.
  void *target_chan = (void*)(uint64)target->pid;
  int sleeping_on_chan = (target->state == SLEEPING && target->chan == target_chan);
  int runnable = (target->state == RUNNABLE);

  if(sleeping_on_chan || runnable){
    acquire(&target->lock);

    // Re-check under target->lock.
    sleeping_on_chan = (target->state == SLEEPING && target->chan == target_chan);
    runnable = (target->state == RUNNABLE);

    if(target->killed || (!sleeping_on_chan && !runnable)){
      release(&target->lock);
      release(&wait_lock);
      return -1;
    }

    // Current process will wait for the opposite yield.
    p->chan = (void*)(uint64)p->pid;
    p->state = SLEEPING;

    // Deliver the value only if the target is already inside co_yield.
    // A RUNNABLE target hasn't called co_yield yet — writing a0 would
    // clobber its pending return value (e.g., fork's return value of 0).
    if(sleeping_on_chan)
      target->trapframe->a0 = value;

    target->state = RUNNING;

    release(&wait_lock);

    co_handoff(p, target);

    p->chan = 0;
    if(holding(&p->lock))
      release(&p->lock);

    if(p->killed)
      return -1;

    return p->trapframe->a0;
  }

  // Target is not in a valid state — return error.
  release(&wait_lock);
  return -1;
}
