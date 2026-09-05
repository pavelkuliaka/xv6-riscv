#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_add(void)
{
  int a, b;
  argint(0, &a);
  argint(1, &b);

  int sum = a + b;
  
  printk("[kernel debug]\na=%d\nb=%d\nsum=%d\n", a, b, sum);
  return sum;
}