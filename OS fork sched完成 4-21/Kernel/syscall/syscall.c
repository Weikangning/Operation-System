//
// Created by wkn on 2021/4/18.
//
#include <syscall.h>
#include <process.h>

extern struct proc_struct * CurrentProcess;


int sys_fork(uint32_t arg[]) {
    struct trapframe *tf = CurrentProcess->tf;
    uintptr_t stack = tf->tf_esp;
    return Fork();
}

int sys_sched(uint32_t arg[]) {
    RunProcess( runnableprocess);
    return 1;
}

//函数指针数组
int (*syscallarray[])(uint32_t arg[]) = {
        [0]              sys_fork,
        [2]              sys_sched,

};

void syscall()
{
    //cprintf("syscall num is\n");
    struct trapframe *tf = CurrentProcess->tf;
    //cprintf("syscall num is\n");
    uint32_t arg[5];
    //cprintf("syscall num is %d\n",CurrentProcess->tf);
    int num =tf->tf_regs.reg_eax;
    cprintf("syscall num is %d\n",num);

    if (num >= 0 && num < 5) {
        if (syscallarray[num] != NULL) {
            arg[0] = tf->tf_regs.reg_edx;
            arg[1] = tf->tf_regs.reg_ecx;
            arg[2] = tf->tf_regs.reg_ebx;
            arg[3] = tf->tf_regs.reg_edi;
            arg[4] = tf->tf_regs.reg_esi;
            tf->tf_regs.reg_eax = syscallarray[num](arg);       //c语言约定返回值存放在eax与edx中
            return;
        }
    }
}