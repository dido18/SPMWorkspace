
# Debugging
## gdb
ompilation for debugging must insert -g flag
icpc -std=c++11 -Wall -I ~/fastflow -g

### execute gdb
gdb --args ./program  arg1 [arg2 ]

### commands gdb
b  9      (breakpoint at line 9)
n         (=next)
up        (navigate tyhe stack call)
down
r         ( run again the program)
d         ( delete the breakpont)
l         (listing the code)
s         (=step go inside a function)
finish    (exit)
p  v      (=print the value)
c         (continue)
info threads (information about threads)
* 				(indicate the thread inside)
thread n  ( jump to thread n)
bt        (=backtrace all the stack call till now inside the threas)


## Valgrind
for finding race condition and memory leaks.
Overrun and underrun access to chard memory


# Profiling Tools

#### oprofile
System profiler open source,  works with threads.

#### PAPI
helpful when you have specific call on your code to profile.
Information about counter.
Semplified versino of vtune aplifier

#### Intel vtune amplifier
Is propetary.
It is installed in the machine in the department

```
ssh -X spm1505@phispm    // -X in order to forward and use GUI intarface

```

source /opt/intel/vtune_amplifires

start  -> newproject -> spm

projecct properties: you have the target System
profile an application running :
  local: the host
  host launch :

Application
select the .icc compiled version with make icc
the source file of matrix.cpp
cd samples/en/C++

start button: lauch the application using vtune

mkl is a kernel library installed int the system .
Used for matrix multiplication  and performance exacution


# FastFlow memory allocator
the standard memory allocator `malloc`  are not really efficient
 qhen there are many small piece of memory ( a struct of 64 bytes and you must produce a stream of thousand tasks).

 ff has its own memory allocation. Sometimes can get improvment in the allocation.
 The memory allocation can be critical for fine grained computation.

Efficient when you have
-  one thread performing a lots of malloc.
-  one thread generates many malloc and multiple threads delete the memory.

provides two interface in `#include ff/allocator.hpp`
- `ff_allocator`
- ''FFAllocator' is based on the previoues but not efficient as the first.

`alloc_std.cpp` is an example of allocator in fast flow.
```

               /---- 1
  x ---- s------2
          \ ----3
```
  x perform new malloc . a tsk is a vector and an atomic counter.

  `fecth_sub()` first fetch the element and then decrement.

Allocator
  `allocator.init()`  just slice the big chunk in smaller chunks.
  `allocator.getslabs()...`
    initiliaze thea llocator with a given amount of memory, every malloc decrease
    this total amount of memeory. If at hrea perform a free the memory id freed.
    Paass the allocator to the first worker and the

using the allocato to allocate  thememory for thetASK.
```
void *p = alloc.malloc(sizeof(Task));
Task *t = new (p) Task;     // the space for storing the object is the position of p
```

Releasing the memory.
```
t-> ~Task();   // MUST call the the cdescrutcor, because the object in is not been allocated with the standard new
alloc.fre(et);

```


`alloc.registerAllocator();` You must say to the Allocator if the thread will perform a free or a malloc-.

The ff allocator can improve in 10 per cent the standard alloator.
With some specific application you can adopt the your own allocator.
Attention on the dynamic memory allocation because is costly.
