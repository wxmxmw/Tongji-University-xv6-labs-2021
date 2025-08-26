// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
struct run *steal(int cpu_id);
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void
kinit()
{
  int i; 
  for (i = 0; i < NCPU; ++i) { 
  //snprintf(kmem[i].lockname, 8, "kmem_%d", i); //the name of the lock 
  initlock(&kmem[i].lock,"kmem"); 
  } 
  //  initlock(&kmem.lock, "kmem");   // lab8-1 
  freerange(end, (void*)PHYSTOP); 
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int c;    // cpuid - lab8-1 

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  // get the current core number - lab8-1 
  push_off(); 
  c = cpuid(); 
  pop_off(); 
  // free the page to the current cpu's freelist - lab8-1
  acquire(&kmem[c].lock);
  r->next = kmem[c].freelist;
  kmem[c].freelist = r;
  release(&kmem[c].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int c;
  push_off(); 
  c = cpuid(); 
  pop_off(); 

  acquire(&kmem[c].lock);
  r = kmem[c].freelist;

  if(r)
    kmem[c].freelist = r->next;
  release(&kmem[c].lock);
  if(!r && (r = steal(c))){
    acquire(&kmem[c].lock);
    kmem[c].freelist = r->next;
    release(&kmem[c].lock);
  }
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

struct run *steal(int cpu_id){
  int i;
  int c =cpu_id;
  struct run *fast,*slow,*head;
  if(cpu_id!=cpuid()){
    panic("steal: cpu_id err");
  }
  for(i=1;i<NCPU;++i){
    if(++c == NCPU){
      c=0;
    }
    acquire(&kmem[c].lock);
    if(kmem[c].freelist){
      slow = head = kmem[c].freelist;
      fast = slow->next;
      while(fast){
        fast=fast->next;
        if(fast){
          slow=slow->next;
          fast=fast->next;
        }
      }
      //the latter half change to current cpu freelist
      kmem[c].freelist =slow->next;
      release(&kmem[c].lock);
      //clear the first half 
      slow->next=0;
      return head;
    }
    release(&kmem[c].lock);
  }
  return 0;
}
