//
// Created by wkn on 2021/1/26.
//
#include <CRTDisplay.h>
#include <trap.h>
#include <stdio.h>
#include <memoryInit.h>
#include <process.h>
void kernel_init(void) __attribute__((noreturn));
int Memory=0;



void kernel_init()
{
    reloadGDT();

    cga_init();

    memory_init();

    idtinit();

    clock_init();

    CurrentProcess=0;
    runnableprocess=0;

    InitIdleProcess();

    int testforkreturn=-1;

    //为内核虚化一个进程并初始化他的PCB
    //未封装Fork()调用函数，内联汇编完成
    asm volatile ("movl $0x0,%eax");
    asm volatile ("int $0x80");

    asm volatile ("movl %%eax , %0":"=m"(testforkreturn)::"%eax");
    cprintf("testforkreturn is %d\n",testforkreturn);

    if(testforkreturn==0)
    {   //子进程
        cprintf("child process\n");
    }
    else
    {//父进程
        cprintf("parent process\n");
        //未封装sched()调用函数，内联汇编完成
        asm volatile ("movl $0x2,%eax");
        asm volatile ("int $0x80");
    }



    while(1);

}