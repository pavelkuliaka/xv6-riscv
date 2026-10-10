#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "file.h"
#include "proc.h"

int
mutexalloc(struct file **f)
{
  struct sleeplock *m;

  *f = 0;
  if ((*f = filealloc()) == 0) {
    return -1;
  }

  if ((m = kalloc()) == 0) {
    fileclose(*f);
    *f = 0;
    return -1;
  }

  initsleeplock(m, "mutex");
  (*f)->type = FD_MUTEX;
  (*f)->mutex = m;
  (*f)->readable = 0;
  (*f)->writable = 0;
  printk("mutexalloc: pid %d mutex %p\n", myproc()->pid, m);
  return 0;
}

void
mutexclose(struct sleeplock *m)
{
  printk("pid %d: mutexclose: mutex %p, kfree\n", myproc()->pid, m);
  kfree(m);
}
