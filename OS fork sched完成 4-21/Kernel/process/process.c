//
// Created by wkn on 2021/4/9.
//
#include <process.h>
#include <UserMemory.h>
#include <MMU.h>
#include <MMLayout.h>
#include <x86.h>
#include <defs.h>
#include <trap.h>
#include <sched.h>

extern char __boot_pgdir[];
extern char bootstacktop[];
extern char bootstack[];


//为操作系统内核创建一个进程，作为所有进程的父进程
void PCBInit(struct proc_struct * kernel_process,int pid,char * name,
        char state,char needsched,
        struct proc_struct * parent,struct proc_struct * child,
        uint32_t kstack,struct trapframe *tf,
        uintptr_t cr3 )
{
    kernel_process->pid=pid;
    //C语言的数组不可以给数组赋值，要用字符串复制函数
    kernel_process->name[0]=name[0];
    kernel_process->name[1]=name[1];
    kernel_process->name[2]=name[2];

    kernel_process->state=state;
    kernel_process->wait_state=0;
    kernel_process->need_resched=needsched;

    kernel_process->parent=parent;
    kernel_process->child=child;

    kernel_process->kstack=kstack;
    kernel_process->tf=tf;

    kernel_process->cr3=cr3;
}

void InitIdleProcess()
{
    struct proc_struct * IdlePorcess=(struct proc_struct *)bucketmalloc(sizeof (struct proc_struct));
    PCBInit(IdlePorcess,1,"KER",1,1,0,0,(uint32_t)bootstack,0,(uint32_t)__boot_pgdir);
    CurrentProcess=IdlePorcess;
}


//复制父进程内存空间 暂不使用
int CopyParentMemory(struct  proc_struct * process)
{
    struct proc_struct * parent=process->parent;
    //将父进程页表，代码，数据复制一份，但内核只需要复制页表即可，不可以复制内核代码数据
    //复制内核页表
    if(process->cr3==0)
    {
        process->cr3=kmalloc4KB(1);
    }
    int * kernelPDT=(int *)parent->cr3;
    //768项以上是内核空间，下是用户空间
    for (int i=768;i<1024;i++)
    {
        ((int *)process->cr3)[i]=kernelPDT[i];
    }
    //复制用户空间
    for (int i=0;i<768;i++)
    {
        if(0!=kernelPDT[i])
        {   //页表
            char * UserPage=kmalloc4KB(1);
            //将Userpage转换为物理地址填入新进程PDT中，
            ((int * )(process->cr3))[i]=((uint32_t)UserPage-KERNBASE)&0xfffff000 + (PTE_P  | PTE_W);
            for (int j=0;j<1024;j++)
            {
                //父进程PDT中i项索引页表，将页表物理地址转换为逻辑地址并访问该页表j项，
                if(0!=((int * )(((uint32_t)(kernelPDT[i])& 0xfffff000)+KERNBASE))[j])
                {
                    char * parentneededcopypage=((int * )(((uint32_t)(kernelPDT[i])& 0xfffff000)+KERNBASE))[j];
                    //该页表索引的内存存储数值也需要复制
                    struct UserMemoryRes NewUserPage = UserMalloc4KB(1);
                    //将Userphymemory逻辑地址转换为物理地址，并填入UserPage中
                    //TODO Userphymemory变换为物理地址
                    ((int *)UserPage)[j]= (NewUserPage.phypageindex*4*1024) & 0xfffff000 + (PTE_P | PTE_W);
                    //复制数据
                    for (int n=0;n<1024;n++)
                    {
                        ((int * )NewUserPage.logicaddress)[n] = ((int * )parentneededcopypage)[n];
                    }
                }
            }
        }
    }
}

//得到一个可用的进程ID
int GetUsableID()
{
    //懒得写 直接返回个1；
    return 1;
}

//得到当前进程ID
int GetCurrentProcessID()
{
    return CurrentProcess->pid;
}

//为进程开辟一个内核栈
void CreateKernelStackForProcess(struct proc_struct * process)
{
    //Intel的栈是从高地址往低地址减，内核栈大8KB
    process->kstack=(uintptr_t)kmalloc4KB(2);
}

//加载一个进程，这个进程的父进程是kernelprocess
int ExceProcess()
{
    //从磁盘或者某进程使用该函数将子进程独立到内核进程
}

//进程开辟，一次调用，两次返回，三个返回值 这里是内核进程
int Fork()
{

    //内核进程全局变量，堆中变量共享，但是堆中变量由于无法获知逻辑地址，可能无法访问。
    //局部变量不共享，他们存储在栈中，每个内核进程都有自己的栈，当然这个栈会复制一份
    struct proc_struct* newPCB=(struct proc_struct*)bucketmalloc(sizeof(struct proc_struct));

    CurrentProcess->child=newPCB;
    newPCB->parent=CurrentProcess;

    //copy栈
    char * kernelstack=kmalloc4KB(2);   //内核栈
    //将内核栈复制过来，这里只需要复制中断帧部分即可，为了代码方便则将8KB内核全复制了
    for (int i=0;i<1024*2;i++)
    {
        ((int *)kernelstack)[i]=((int *)bootstack)[i];
    }

    *newPCB= *CurrentProcess;

    newPCB->kstack=(uintptr_t)kernelstack;
    //中断帧与新内核对齐至同等地方
    newPCB->tf=(struct trapframe * )((uintptr_t)CurrentProcess->tf-CurrentProcess->kstack + newPCB->kstack);

    newPCB->tf->tf_regs.reg_eax=0;

    newPCB->context.esp= (uint32_t)kernelstack +8*1024;    //新进程内核创建

    newPCB->context.eip=(uintptr_t)forkret;

    AddRunableProcess(newPCB);
    cprintf("Fork finish\n");
    return CurrentProcess->pid;
}

//将@newprocess进程添加至runnableprocess链表
int AddRunableProcess(struct proc_struct * newprocess)
{
    if(runnableprocess==0)
        runnableprocess=newprocess;
    else
    {
        newprocess->next=runnableprocess->next;
        runnableprocess->next=newprocess;
    }

}


void RunProcess( struct proc_struct * readyprocess)
{

    if (readyprocess != CurrentProcess) {
        //bool intr_flag;
        struct proc_struct *prev = CurrentProcess, *next = readyprocess;

        //local_intr_save(intr_flag);
        {

            CurrentProcess=readyprocess;
            //TODO  提权使用 内核
            //load_esp0(next->kstack + KSTACKSIZE);

            //内核进程不用更换CR3
            //lcr3(next->cr3);

            switch_to(&(prev->context), &(next->context));


        }
        //local_intr_restore(intr_flag);
    }
}

void forkret(void)
{

    forkrets(CurrentProcess->tf);
    cprintf("**");
}