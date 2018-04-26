
## Memory management

  The memory management is definitely one of the most important subsystem of
  the kernel. The kernel has the task to provide protected memory resources
  to every application but also to share some space with external devices or
  for inter-process communication (IPC).

  The kernel manipulate both physical and virtual memory. The virtualization
  of the memory allo a task to access the full extends of system memory, as it
  was the only one to use it. More than that, it looks like available memory
  is the maximum supported by the platform, even if the amount of physical
  memory is much smaller.

  This chapter discuss about how the kernel manage virtual and physical memory
  and shared it amoung tasks.

### The Memory Address Spaces

  An address space concist of the virtual memory addressable by a process and
  the addresses within the virtual memory that a process is allowed to use.
  Each process are given a flat, continuous address space, with the size
  depending on the architecture.

  As an exemple, with the configuration in use at this time on x86 machines.
  User space process use a virtual memory address space that extends from
  the address 00400000 (4Mb) to c0000000 (3Gb). Note that using virtual
  memory, process can store different data using the same addresses as they
  all have each their own memory space.

  Although a process can address up to 3068Mb in our exemple, it doesn't have
  premission to access all of it. Instead it must ask the kernel to allocate
  some addresses intervals (such as 00404000-00409000) in which the process
  might have access to. Those intervals of legal addresses are called _virtual
  memory areas_ or VMAs.

  A process can access a memory address only in a valid VMA, which is
  associated with permission such as readable, writable and executable. If a
  process access a memory address outside of valid VMA or in an invalid manner
  the kernel kills the process involving a "Segmentation Fault" error.

  VMA are used to map several sorts of data such as:
  - Mapping sections of an executable file (code or data).
  - Allocate a thread stack.
  - Mapping data files
  - Mapping shared memory segments used as IPC.
  - An anonymous memory area for used with `malloc()`.

### Address space operations

  Hoping the principe of address space is clear, let digs into implemenation.
  The kernel code define a class to handle this address space. As we are using
  **C**, the data structure is named `mspace_t` and comes with a set of
  functions named like `mspace_*` which will opereate on the address space in
  a secure way. Interface is defined on `kernel/memory.c` header file and
  implemented on  `core/mspace.c` source file.

```c
typedef struct mspace mspace_t;

struct mspace {
    long users;  /* Usage counter */
    bbtree_t tree;  /* Binary tree of VMAs sorted by addresses */
    page_t directory;  /* Page global directory */
    size_t lower_bound;  /* Lower bound of the address space */
    size_t upper_bound;  /* Upper bound of the address space */
    size_t v_size;  /* Virtual allocated page counter */
    size_t p_size;  /* Private allocated page counter */
    size_t s_size;  /* Shared allocated page counter */
    size_t phys_pg_count;  /* Physical page used on this address space */
    splock_t lock;  /* Memory space protection lock */
};

/* Map a memory area inside the provided address space. */
void *mspace_map(mspace_t *mspace, size_t address, size_t length,
                 inode_t *ino, off_t offset, off_t limit, int flags);
/* Change the protection flags of a memory area. */
int mspace_protect(mspace_t *mspace, size_t address, size_t length, int flags);
/* Change the flags of a memory area. */
int mspace_unmap(mspace_t *mspace, size_t address, size_t length);
/* Remove disabled memory area */
int mspace_scavenge(mspace_t *mspace);
/* Print the information of memory space -- used for /proc/{x}/mmap  */
void mspace_display();
/* Create a memory space for a user application */
mspace_t *mspace_create();
/* Release all VMA marked as DEAD. */
void mspace_sweep(mspace_t *mspace);
/* Search a VMA structure at a specific address */
vma_t *mspace_search_vma(mspace_t *mspace, size_t address);

```



```c
typedef struct vma vma_t;
struct vma {
    bbnode_t node;  /* Binary tree node of VMAs contains the base address */
    size_t length;  /* The length of this VMA */
    inode_t *ino;  /* If the VMA is linked to a file, a pointer to the inode */
    off_t offset;  /* An offset to a file or an physical address, depending of type */
    off_t limit;  /* If non zero used to specify a limit after which one the rest of pages are blank. */
    int flags;  /* VMA flags */
};

```

  The operation handle on address space is to create, delete or update VMAs.
  The VMA structure is defined as `vma_t`.






How to create/update vma. (class MSpace)

Pages Fault / Resolv

RAM INIT (+Zones)

KMap / Kalloc






  Two structures are used to manipulate a memory space.






MEMORY.H provide Access to mspace_* method, page_ methods and mmu_ methods.
VMA.H HAVE VMA_* flags, vma_ structure, and kMMU structure
MMU.H  PAGE_SIZE, page_t   (+ MMU_BMP & MMU_LG)

MSPACE.C provice mspace_ methods
PAGE.C provide page_ methods
MMU.C provide mmu_ methods




  Memory allocation is also not as easy as regular application due to the lack
  or runtimes libraries.
