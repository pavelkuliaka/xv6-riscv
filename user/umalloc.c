#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/riscv.h"

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void
free(void *ap)
{
  Header *bp, *p;

  bp = (Header *)ap - 1;
  for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if (bp + bp->s.size == p->s.ptr) {
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if (p + p->s.size == bp) {
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

static Header *
morecore(uint nu)
{
  char *p;
  Header *hp;

  if (nu < 4096)
    nu = 4096;
  p = sbrk(nu * sizeof(Header));
  if (p == SBRK_ERROR)
    return 0;
  hp = (Header *)p;
  hp->s.size = nu;
  free((void *)(hp + 1));
  return freep;
}

void *
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
  if ((prevp = freep) == 0) {
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
    if (p->s.size >= nunits) {
      if (p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void *)(p + 1);
    }
    if (p == freep)
      if ((p = morecore(nunits)) == 0)
        return 0;
  }
}

// ===========================================================================

const int sfl_debug = 1;

#define USED 1
#define NCLASS 24 // число списков

typedef struct blk {
  uint size; // размер с заголовком
  uint prev; // размер предыдущего блока
} blk; // блок

typedef struct fblk {
  blk h; // блок
  struct fblk *next, *prev; // следующий и предыдущий свободные блоки
} fblk; // свободный блок

#define MINBLK sizeof(fblk) // минимальный размер свободного блока

static char *start, *end; // границы кучи
static blk *tail; // последний блок кучи
static fblk *fl[NCLASS]; // массив списков свободных блоков

static uint sz(blk *b) { return b->size & ~USED; } // размер блока
static int used(blk *b) { return b->size & USED; } // используется ли блок

// следующий блок
static blk *nxt(blk *b) {
  char *p = (char *)b + sz(b);
  return p < end ? (blk *)p : 0;
}

// предыдущий блок
static blk *prv(blk *b) {
  return b->prev ? (blk *)((char *)b - b->prev) : 0;
}

// выброр корзины для блока
static int cls(uint n) {
  int c = 0;
  while (n >>= 1) c++;
  return c < NCLASS ? c : NCLASS - 1;
}

// добавление блока в список
static void fl_add(blk *b) {
  fblk *f = (fblk *)b;
  int c = cls(sz(b));
  f->prev = 0;
  f->next = fl[c];
  if (f->next) f->next->prev = f;
  fl[c] = f;
}

// удаление блока из списка
static void fl_del(blk *b) {
  fblk *f = (fblk *)b;
  if (f->prev) f->prev->next = f->next; else fl[cls(sz(b))] = f->next;
  if (f->next) f->next->prev = f->prev;
}

// вывод отладочной информации
static void dump(const char *op, uint n) {
  if (!sfl_debug) return;
  uint heap = end - start, u = 0;
  printf("[sfl] %s(%d): heap=[%p..%p) size=%d\n", op, n, start, end, heap);
  for (char *p = start; p < end; p += sz((blk *)p)) {
    blk *b = (blk *)p;
    printf("  %p %s %d\n", b, used(b) ? "USED" : "FREE", sz(b));
    if (used(b)) u += sz(b);
  }
  printf("  used=%d/%d (%d%%)\n", u, heap, heap ? u * 100 / heap : 0);
}

// найти подходящий блок
static blk *find(uint need) {
  for (int c = cls(need); c < NCLASS; c++)
    for (fblk *f = fl[c]; f; f = f->next)
      if (sz(&f->h) >= need)
        return &f->h;
  return 0;
}

// расширить кучу
static int grow(uint need) {
  uint extra = (tail && !used(tail)) ? need - sz(tail) : need;
  uint bytes = PGROUNDUP(extra);

  char *p = sbrk(bytes);
  if (p == SBRK_ERROR) return -1;
  end += bytes;

  if (tail && !used(tail)) {
    fl_del(tail);
    tail->size += bytes;
    fl_add(tail);
  } else {
    blk *b = (blk *)p;
    b->size = bytes;
    b->prev = tail ? sz(tail) : 0;
    tail = b;
    fl_add(b);
  }
  if (sfl_debug) printf("[sfl] sbrk(+%d) -> brk=%p\n", bytes, end);
  return 0;
}

// выделение памяти
void *nmalloc(uint n) {
  if (n == 0 || n > (1 << 28)) return 0;
  uint need = (n + sizeof(blk) + 7) & ~7;
  if (need < MINBLK) need = MINBLK;

  if (!start) start = end = sbrk(0);

  blk *b = find(need);
  if (!b) {
    if (grow(need) < 0) return 0;
    b = find(need);
  }
  fl_del(b);

  uint rest = sz(b) - need;
  if (rest >= MINBLK) {
    blk *r = (blk *)((char *)b + need);
    r->size = rest;
    r->prev = need;
    blk *x = nxt(r);
    if (x) x->prev = rest; else tail = r;
    b->size = need | USED;
    fl_add(r);
  } else {
    b->size |= USED;
  }

  dump("malloc", n);
  return (char *)b + sizeof(blk);
}

// освобождение памяти
void nfree(void *ap) {
  if (!ap) return;
  blk *b = (blk *)((char *)ap - sizeof(blk));
  if ((char *)b < start || (char *)ap >= end || !used(b)) {
    printf("[sfl] free: bad pointer %p\n", ap);
    return;
  }
  b->size &= ~USED;

  blk *n = nxt(b);
  if (n && !used(n)) {
    fl_del(n);
    b->size += sz(n);
    if (n == tail) tail = b;
  }
  blk *p = prv(b);
  if (p && !used(p)) {
    fl_del(p);
    p->size += sz(b);
    if (b == tail) tail = p;
    b = p;
  }
  n = nxt(b);
  if (n) n->prev = sz(b);

  if (b == tail && sz(b) >= MINBLK + PGSIZE) {
    uint r = PGROUNDDOWN(sz(b) - MINBLK);
    if (sbrk(-(int)r) != SBRK_ERROR) {
      end -= r;
      b->size -= r;
      if (sfl_debug) printf("[sfl] sbrk(-%d) -> brk=%p\n", r, end);
    }
  }
  fl_add(b);
  dump("free", 0);
}
