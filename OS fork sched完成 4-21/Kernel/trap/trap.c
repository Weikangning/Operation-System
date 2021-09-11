//
// Created by wkn on 2021/3/12.
//
#include <trap.h>
#include <defs.h>
#include <x86.h>
#include <CRTDisplay.h>
#include <MMLayout.h>
#include <stdio.h>
#include <syscall.h>
#include <process.h>
struct trapgate IDT[256]={0};
extern struct proc_struct * CurrentProcess;
//这里IDT减去了KERNBASE 但是idt_pd不用减KERNBASE？？？
//idt_pd是个标签，在通过lidt汇编时，属于访问一个变量，使用使用DS寄存器
//IDT通过lidt汇编指令写入IDTR（是个寄存器）中，当有中断触发时，CPU读取IDTR的base值，不算访问数据段，不使用用DS。换言之，加载到了IDTR寄存器中，它就丢失了数据段标签的含义
//      目前没有开启页模式，没有地址映射，编译的起始地址是0xc0000000，但实际加载内存地址是1MB，所以这里要自己减去KERNBASE，才能正常访问
//      这是访问数据段与访问寄存器，DS的作用范围了
struct pseudodesc idt_pd = {
        sizeof(IDT)-1, (unsigned long int)IDT
};

extern unsigned long int __vectors[];

void idtinit(void) {

    // dispatch based on what type of trap occurred
    for (int i=0;i<sizeof(IDT)/sizeof (struct trapgate);i++)
    {
        IDT[i].CodeSegmentOffset_0_15=(__vectors[i])&0xffff;
        IDT[i].CodeSegmentOffset_16_31=(uint32_t)(__vectors[i]) >> 16;
        IDT[i].DPL=0;
        IDT[i].TargetCodeSegmentSelector=GD_KTEXT;
        IDT[i].Type=0xE;    //中断
        IDT[i].P=1;
        IDT[i].Reserved=0;
    }

    //按照惯例，0x80是系统调用，这里我们也保留下来
    IDT[0x80].CodeSegmentOffset_0_15=__vectors[0x80]&0xffff;
    IDT[0x80].CodeSegmentOffset_16_31=(uint32_t)(__vectors[0x80]) >> 16;
    IDT[0x80].DPL=3;        //DPL为3，用户程序可以使用
    IDT[0x80].TargetCodeSegmentSelector=GD_KTEXT;
    IDT[0x80].Type=0xF;    //陷阱
    IDT[0x80].P=1;
    IDT[0x80].Reserved=0;

    asm volatile ("lidt (%0)" : : "r" (&idt_pd) : "memory");
}

void trap(struct trapframe *tf)
{
    CurrentProcess->tf=tf;

    if (tf->tf_trapno==0x32)
    {//0x32中断处理程序
        cprintf("the inter 0x32!\n");
        return ;
    }

    if(IRQ_OFFSET + IRQ_TIMER ==tf->tf_trapno)
    {
        cprintf("shizhong**\n");
        return ;
    }

    if(0x80 == tf->tf_trapno)
    {
        syscall();
        cprintf("inter syscall\n");
        return ;
    }
    //其余中断
    cprintf("else inter %d\n",tf->tf_trapno);
    while(1);
}
