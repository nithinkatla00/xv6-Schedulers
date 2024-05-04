#define DEFAULT_TICKETS 1
#define NSEGS 7

// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?

  struct cpu *cpu;
  struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };
//This enumerator will be used to determine the color of each process in the red-black tree
enum procColor {RED, BLACK};		

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  int priority;                // Process priority
  uint ctime;                   // Process creation time
  int stime;                   //process SLEEPING time
  int retime;                  //process READY(RUNNABLE) time
  int rutime;                  //process RUNNING time
  int tickets;                  // Process tickets used in LOTTERY scheduling algorithm
  int virtualRuntime;
  int currentRuntime;
  int maximumExecutiontime;
  int niceValue;
  int weightValue;

  /*
  Red-Black Tree data structure:
  -Each process that is in the red-black tree will contain three pointers to processes that are the parent and two childern. 
  -If the currently inserted process in the red-black tree run queue is the root, i.e. the only process in the red-black tree or is the highest process in the red-black tree, then the parent pointer will point to itself, and the left/right pointers will point to either null or another process if it is in the tree.
  -Other processes in the red-black tree will have a pointer to a parent process, i.e those processes are children of a process closer to the root, and the left/right pointers either will point to NULL or processes.
  -Each pointer pointing to null will be considered as a black node on the true. 
  -A process in the red-black tree can be either red or black. 
*/

  enum procColor coloring;
  struct proc *left;
  struct proc *right;
  struct proc *parentP;
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

static void wakeup1(void *chan);

void updatestatistics();
int random(int max);
int totalTickets();
struct proc* findReadyProcess(int *index1, int *index2, int *index3, uint *priority);
