//
// Created by wkn on 2021/4/9.
//

#ifndef OS_PROCESS_H
#define OS_PROCESS_H

#include <defs.h>
#include <list.h>
#include <trap.h>
#include <MMLayout.h>

#define PROC_NAME_LEN               15
#define MAX_PROCESS                 4096
#define MAX_PID                     (MAX_PROCESS * 2)

// process's state in his life cycle
enum proc_state {
    PROC_UNINIT = 0,  // uninitialized
    PROC_SLEEPING,    // sleeping
    PROC_RUNNABLE,    // runnable(maybe running)
    PROC_ZOMBIE,      // almost dead, and wait parent proc to reclaim his resource
};

// Saved registers for kernel context switches.
// Don't need to save all the %fs etc. segment registers,
// because they are constant across kernel contexts.
// Save all the regular registers so we don't need to care
// which are caller save, but not the return register %eax.
// (Not saving %eax just simplifies the switching code.)
// The layout of context must match code in switch.S.
struct context {
    uint32_t eip;
    uint32_t esp;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
};

extern list_entry_t proc_list;

struct proc_struct {
    int pid;                                    // Process ID
    char name[PROC_NAME_LEN + 1];               // Process name]

    enum proc_state state;                      // Process state
    uint32_t wait_state;                        // waiting state
    volatile bool need_resched;                 // bool value: need to be rescheduled to release CPU?

    struct proc_struct * parent;                 // the parent process
    struct proc_struct * child;

    uintptr_t kstack;                           // Process kernel stack
    struct trapframe *tf;                       // Trap frame for current interrupt
    struct context context;                     // Switch here to run process
    uintptr_t cr3;                              // CR3 register: the base addr of Page Directroy Table(PDT)

    int exit_code;                              // exit code (be sent to parent proc)

    struct proc_struct * next;
};

struct proc_struct * CurrentProcess;

struct proc_struct * runnableprocess;

struct proc_struct * waitprocess;

void PCBInit(struct proc_struct * kernel_process,int pid,char * name,
                       char state,char needsched,
                       struct proc_struct * parent,struct proc_struct * child,
                       uint32_t kstack,struct trapframe *tf,
                       uintptr_t cr3 );

int CopyParentMemory(struct  proc_struct * process);
int GetUsableID();
int GetCurrentProcessID();
void CreateKernelStackForProcess(struct proc_struct * process);
int ExceProcess();
int Fork();
void RunProcess( struct proc_struct * readyprocess);
int AddRunableProcess(struct proc_struct * newprocess);

void forkret(void);
void forkrets(struct trapframe *tf);
#endif //OS_PROCESS_H
