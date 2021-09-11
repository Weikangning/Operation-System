//
// Created by wkn on 2021/3/17.
//
#include <defs.h>
#include <MMLayout.h>
#include <MMU.h>
#include <x86.h>
#include <stdio.h>
#include <memoryInit.h>
//GCC编译后的内核结束地址
extern char end[];

extern int __boot_pt1[];

//内核加内核初始化数据占用空间的结束地址
char * kernnelpageend;

//struct Page数组的起始地址
struct Page * pageRecord=(struct Page *)0x0;

//物理内存页数量
int sumpagenum=0;

//内核页目录表起始地址
extern char __boot_pgdir[];

extern char pagepionter[];

int maxMemory=0;
int minMemory=0;

int num4KB=0;

struct taskstate ts = {0};

void tlb_invalidate(pde_t *pgdir, uintptr_t la) {
    if (rcr3() == pgdir-KERNBASE) {
        invlpg((void *)la);
    }
}

static struct segdesc gdt[] = {
        SEG_NULL,
        [SEG_KTEXT] = SEG(STA_X | STA_R, 0x0, 0xFFFFFFFF, DPL_KERNEL),
        [SEG_KDATA] = SEG(STA_W, 0x0, 0xFFFFFFFF, DPL_KERNEL),
        [SEG_UTEXT] = SEG(STA_X | STA_R, 0x0, 0xFFFFFFFF, DPL_USER),
        [SEG_UDATA] = SEG(STA_W, 0x0, 0xFFFFFFFF, DPL_USER),
        [SEG_TSS]   = SEG_NULL,
};

static struct pseudodesc gdt_pd = {
        sizeof(gdt) - 1, (uintptr_t)gdt
};

struct BucketDirectory kbucket ={
        .size16chain=0,
        .size32chain=0,
        .size64chain=0,
        .size128chain=0,
        .size256chain=0,
        .size512chain=0,
        .size1024chain=0
};
//提权使用
void
load_esp0(uintptr_t esp0) {
    ts.ts_esp0 = esp0;
}

void memory_init()
{
    //一个struct Page 36字节
    //init page pionter ,it will use in malloc a page ,opterating logic address 0xffc0 0000 can write new page
    ((int *)__boot_pgdir)[1023]=((((uint32_t)pagepionter)-KERNBASE) & 0xfffff000) + (PTE_P | PTE_U | PTE_W);

    getMaxMemory();

    mapPageForKernel();

    pageRecordInit();

    /*test the kmalloc4KB function
    char * tmp=0;
    tmp=kmalloc4KB(5000);

    tmp[5000*1024*4-1]=4;
    */

    /*
     * test bucket内存分配
    char *tmp=bucketmalloc(12);
    char *tmp1=bucketmalloc(12);
    char *tmp2=bucketmalloc(12);
    char *tmp3=bucketmalloc(12);
     cprintf("memory_init():test bucket:%d %d %d %d\n",tmp,tmp1,tmp2,tmp3);
    test end
    */
}

void reloadGDT()
{
    // set boot kernel stack and default SS0
    load_esp0((uintptr_t)0);
    ts.ts_ss0 = KERNEL_DS;

    // initialize the TSS filed of the gdt
    gdt[SEG_TSS] = SEGTSS(STS_T32A, (uintptr_t)&ts, sizeof(ts), DPL_KERNEL);

    asm volatile ("lgdt (%0)" :: "r" (&gdt_pd));
    asm volatile ("movw %%ax, %%gs" :: "a" (USER_DS));
    asm volatile ("movw %%ax, %%fs" :: "a" (USER_DS));
    asm volatile ("movw %%ax, %%es" :: "a" (KERNEL_DS));
    asm volatile ("movw %%ax, %%ds" :: "a" (KERNEL_DS));
    asm volatile ("movw %%ax, %%ss" :: "a" (KERNEL_DS));
    // reload cs
    asm volatile ("ljmp %0, $1f\n 1:\n" :: "i" (KERNEL_CS));

    ltr(GD_TSS);
}

static void getMaxMemory()
{
    struct e820map * memory_record=(struct e820map *)(0x8000+KERNBASE);

    //已经映射了4MB，依靠这4MB完成内存探测，并将映射范围覆盖至全部内存，内存不一定是4GB

    //根据内存范围构建页表；0~4MB 与 KERNBASE~KERNBASE+4MB映射至0~4MB
    for (int i=0;i<memory_record->nr_map;i++)
    {
        uint32_t seg_start = memory_record->map[i].addr;
        uint32_t seg_end = memory_record->map[i].addr + memory_record->map[i].size;

        if (seg_end > maxMemory)
            maxMemory = seg_end;
        if (seg_start < minMemory)
            minMemory = seg_start;
    }

}

static void mapPageForKernel() {

    uint32_t *kernelbootpgdir = (uint32_t *) __boot_pgdir+KERNBASE;

    sumpagenum = maxMemory / (4 * 1024);

    uint32_t neededmemory = sumpagenum * sizeof(struct Page);//struct page数组所需空间

    if(0!=neededmemory & 0xfff) //4KB对齐
        neededmemory=((neededmemory>>12)+1)<<12;

    neededmemory+= neededmemory/1024+1024*4;   //记录page数组，页表映射需要的空间

    kernnelpageend=end + neededmemory;
    if(neededmemory<(4*1024*1024- (uint32_t) end + KERNBASE ))//初始化提供的4MB空间足够,多余的映射消除
    {

        if(0 != kernnelpageend & 0xfff)
            kernnelpageend= (((uint32_t)kernnelpageend>>12)+1)<<12;

        for (int i=(((uint32_t)kernnelpageend-KERNBASE)>>12)+1;i<1024;i++)
            ((int *)__boot_pt1)[i]=0;

        pageRecord=(struct Page *)end;
        return;
    }


    neededmemory -= (4*1024*1024- (uint32_t) end + KERNBASE );

    if (0 != neededmemory & 0xfff)
        neededmemory=((neededmemory>>12)+1)<<12;


    int num4M = neededmemory / (4 * 1024 * 1024);
    if (0 != neededmemory & 0xfffff)    //4MB
        num4M++;

    num4KB = neededmemory >> 12;


    kernnelpageend=(char *)(KERNBASE+4*1024*1024+neededmemory);
    //4KB对齐
    if (0 != (uint32_t)kernnelpageend % (4 * 1024))
        kernnelpageend=(char * )((((uint32_t) kernnelpageend)>>12 + 1)<<12);

    uint32_t *pagetable = (uint32_t *) end;

    int countpt=0;
    for (countpt = 0; countpt < num4KB ; countpt++) {
        pagetable[countpt] = 4 * 1024 * 1024 + countpt * 1024 * 4 + (PTE_P | PTE_U | PTE_W);
    }
    for (int i = 1; i <= num4M ; i++) {
        kernelbootpgdir[i + ((KERNBASE + 4 * 1024 * 1024) >> 22)] =
                ((uint32_t)(pagetable) + (i - 1) * 1024) - KERNBASE + (PTE_P | PTE_U | PTE_W);
    }

    pageRecord=(struct Page * )(pagetable+countpt);
    if(0!= ((uint32_t)pageRecord & 0x3ff))
    {//4KB对齐 不可以侵占最后一个4KB页表
        pageRecord=(((uint32_t)pageRecord)>>12 + 1)<<12;
    }
    else
    {
        pageRecord=(((uint32_t)pageRecord)>>12 )<<12;
    }
}

static void pageRecordInit()
{
    struct e820map * memory_record=(struct e820map *)(0x8000+KERNBASE);
    //获取所有可用的内存空间并组成4KB大小的页
    int countpage=0;
    for (int i=0;i<memory_record->nr_map;i++) {

        uint32_t seg_start = memory_record->map[i].addr;
        uint32_t seg_end = memory_record->map[i].addr + memory_record->map[i].size;

        if (memory_record->map[i].type == E820_ARM) {
            //将地址对齐至4KB
            if (0 != (seg_start & 0xfff)) {
                seg_start = ((seg_start & 0xFFFFF000)+1) << 12;
            }
            seg_end = seg_end & 0xFFFFF000;
        }
        else {

            //将地址对齐至4KB
            if (0 != (seg_end & 0xfff)) {
                seg_end =( (seg_end & 0xFFFFF000) +1) << 12;
            }
            seg_start = seg_start & 0xFFFFF000;
            //cprintf("%d %d\n",seg_start,seg_end);
        }

        if ((seg_end - seg_start) > PGSIZE) {
            int pagenum = (seg_end - seg_start) / PGSIZE;

            for (int j = 0; j < pagenum; j++) {
                pageRecord[countpage + j].ref = 0;

            }
            countpage += pagenum;
        }
    }

    //将已经使用的页进行标记
    //0~4MB物理内存被映射两次，ref值为2
    //4MB~ kernnelpageend-KERNBASE 物理内存被映射一次，ref为1

    for (int i=0;i<=(uint32_t)(kernnelpageend-KERNBASE)/PGSIZE;i++)
        pageRecord[i].ref=2;

}

//查询pageRecord数组，开辟@startoffset物理页帧起始处连续的 @num_n 个物理页，返回符合要求的page下标
int getpageindex(int num_n,int startoffset)
{
    char flag=1;
    for (int i=startoffset;i<128*1024/4;i++)
        if(pageRecord[i].ref==0)
        {
            int j=0;
            flag=1;

            for (j=0;j<num_n;j++)
                if(pageRecord[i+j].ref!=0)
                {flag=0;break;}

            if(1==flag)
            {
                for (j=0;j<num_n;j++)
                    pageRecord[i+j].ref++;
                return i;
            }
            i += j;
        }
    return 0;
}

//查询pageRecord数组，开辟连续的 @num_n 个物理页，返回第一个物理页的逻辑地址
char * kernelgetpages(int num_n)
{
    int resindex=getpageindex(num_n,1024);
    if(0==resindex)
        return (char * )0;
    else
        return ((char *)(resindex*PGSIZE+KERNBASE));

}

//内核开辟@num * 4KB内存
char * kmalloc4KB(int num_n)
{
    char * startaddress=kernelgetpages( num_n);
    //cprintf("start address is %d  ",startaddress);
    if(startaddress==0)
    {
        //cprintf("kmalloc memory is space! kamlloc failed\n");
        return startaddress;
    }
    //内核页目录表换算为逻辑地址，startaddress高10位填入

    //通过页目录表中索引得 页表物理地址 转换为虚拟地址（+KERNBASE），填页表相关项
    for (int i=0;i<num_n;i++)
    {
        //待映射的逻辑页地址
        char * waitingmaplogicaddress=startaddress+i*PGSIZE;

        //待映射页的PDT内偏移值：映射页逻辑地址的高10位
        int PDToffset= (uint32_t)waitingmaplogicaddress>>22;
        //待映射页的页表内偏移值：映射页逻辑地址的中10位
        int PToffset = ((uint32_t)waitingmaplogicaddress & 0x3FF000)>>12;

        //waitingmaplogicaddress得出页目录偏移值

        if(((int *)__boot_pgdir) [PDToffset]==0)
        {
            //开辟一个页表tmppage
            //tmppage地址还未填写页表，直接操作内存奔溃

            char * tmppage =kernelgetpages(1);

            //将这个新的地址tmppage暂时映射到pagepionter下,tmppage是新开辟的页表
            ((int * )pagepionter)[0]=(((uint32_t)tmppage-KERNBASE) & 0xfffff000) + (PTE_P | PTE_U | PTE_W);
            invlpg(0xffc00000);
            //新开辟的页表转换为物理地址，补齐权限写入PDT中
            ((int *)__boot_pgdir) [PDToffset]=(((uint32_t)tmppage-KERNBASE) & 0xfffff000)+ (PTE_P | PTE_U | PTE_W);
            //cprintf("%d\n",((int *)__boot_pgdir) [PDToffset]);
            //0xffc0 0000地址是一个特殊的用于操作新开辟页表的地址，
            for (int n=0;n<1024;n++)
            { //将该页表1024项记录全部清空
                ((int *)0xffc00000)[n]= 0;
            }

        }
        else
        {
            //通过PDT中的物理地址+KERNBASE得到PDToffset索引页表的逻辑地址  注意地12位是标记
            ((int * )pagepionter)[0]= ((int *)__boot_pgdir)[PDToffset];
            invlpg(0xffc00000);
        }

        //将待映射页映射入页表  the kmallocnum is more than space 4MB this code will break down
        ((int *)0xffc00000)[PToffset]=( ((uint32_t)waitingmaplogicaddress -KERNBASE) & 0xfffff000 )+ (PTE_P | PTE_U | PTE_W);

    }

    //cprintf("/// %d  %d %d %d\n",((int *)__boot_pgdir) [768],((int *)__boot_pgdir) [769],((int *)__boot_pgdir) [770],((int *)__boot_pgdir) [1023]);

    //while(1);
    return startaddress;
}

//桶分配小字节内存
char * bucketmalloc(int byte)
{
    struct BucketDes * p=0;
    int type=1024;
    //使用那个尺寸块
    if(byte<=1024-1)
    {
        p=kbucket.size1024chain;
        type=1024;
    }
    if(byte<=512-1)
    {
        p=kbucket.size512chain;
        type=512;
    }
    if(byte<=256-1)
    {
        p=kbucket.size256chain;
        type=256;
    }
    if(byte<=128-1)
    {
        p=kbucket.size128chain;
        type=128;
    }
    if(byte<=64-1)
    {
        p=kbucket.size64chain;
        type=64;
    }
    if(byte<=32-1)
    {
        p=kbucket.size32chain;
        type=32;
    }
    if(byte<=16-1)
    {
        p=kbucket.size16chain;
        type=16;
    }
    //标记能否有合适块
    char flag=0;
    struct BucketDes *pre=p;//for循环判断条件对只有一个节点的链表会判断错误，使用pre来防范
    for (;p!=0 && p->next != 0 ;p=p->next)
        if(p->rest>0)
        {flag=1;break;}

    if(pre!=0 && pre->rest>0)      //链表第一个节点很特殊
    {p=pre;flag=1;}

    char * resaddress=0;
    if(1==flag)
    {//有合适的块空间
        int currentaddress=(p->freeoffset);//通过BucketDes确定页内块起始偏移量
        resaddress=p->page+currentaddress;  //得到该块的逻辑地址
        p->freeoffset=resaddress[0]*16;     //该块未分配前，第一个字节处存储的是下一个空闲块的页内偏移量，依靠这个偏移量更新BucketDes的freeoffset值
        p->rest--;  //可用块减一
        resaddress[0]=type/16;  //该块的第一个字节存储该块大小（除16是为了能让一个字节就存储下，不然需要耗费两个字节）
        resaddress++;   //真正可用地址需要加一
    }
    else
    {//没有合适的块，开辟一个页，依靠type初始化里面的块，主要初始化空闲链表

        //首先需要开辟一个BucketDes并接入对应的kbucket.next
        struct BucketDes* newbucketDes= (struct BucketDes *)mallocBucketDes();
        //开一新的4KB页
        char * newpage=kmalloc4KB(1);
        //初始化新页的空闲链表
        for(int i=0;i<PGSIZE/type;i++)
        {
            newpage[i*type]= (i+1)*type/16;
        }

        //最后一块的空闲指针用-1标记
        newpage[PGSIZE-type]=-1;
        if(p==0)
            //接入对应的kbucket.next
            switch (type) {
                case 16: kbucket.size16chain=newbucketDes;break;
                case 32: kbucket.size32chain=newbucketDes;break;
                case 64: kbucket.size64chain=newbucketDes;break;
                case 128: kbucket.size128chain=newbucketDes;break;
                case 256: kbucket.size256chain=newbucketDes;break;
                case 512: kbucket.size512chain=newbucketDes;break;
                case 1024: kbucket.size1024chain=newbucketDes;break;

            }
        else
            p->next=newbucketDes;   //将新的BucketDes拼接在p后面
        //初始化这个描述块
        newbucketDes->next=0;
        newbucketDes->page=newpage;
        newbucketDes->freeoffset=0;
        newbucketDes->rest=PGSIZE/type;
        //划分出type大小并修改空闲块链表，同bucketmalloc(int byte)函数
        int currentaddress=(newbucketDes->freeoffset);
        resaddress=newbucketDes->page+currentaddress;
        newbucketDes->freeoffset=resaddress[0]*16;
        newbucketDes->rest--;
        resaddress[0]=type/16;
        resaddress++;
    }
    return resaddress;
}

//开辟一个块描述符，sizeof（struct BucketDes）=16 ,故本质是开辟一个32字节空间
char * mallocBucketDes()
{
    struct BucketDes * p=0;
    int type=1024;
    int byte=sizeof (struct BucketDes);
    if(byte<=32-1)
    {
        p=kbucket.size32chain;
        type=32;
    }
    if(byte<=16-1)
    {
        p=kbucket.size16chain;
        type=16;
    }
    char flag=0;
    struct BucketDes * pre=p;
    //查看32字节块中是否用空间
    for (;p!=0 && p->next != 0; p=p->next)
        if(p->rest>0)
        {flag=1;break;}

    if(pre!=0 && pre->rest>0)      //链表第一个节点很特殊
        {p=pre;flag=1;}

    char * resaddress=0;
    if(1==flag)
    {//有合适的32字节块，同bucketmalloc(int byte)对应的操作
        int currentaddress=(p->freeoffset);
        resaddress=p->page+currentaddress;
        p->freeoffset=resaddress[0]*16;
        p->rest--;
        resaddress[0]=type/16;
        resaddress++;
    }else
    {//需要开辟新的4KB页并初始化
        char *newpage=kmalloc4KB(1);

        for (int i=0;i<PGSIZE/type;i++)
        {//初始化空闲块链表
            newpage[i*type]= (i+1)*type/16;
        }

        int freeoffset=newpage[0];
        if(p==0)
            switch (type) {
                case 16: kbucket.size16chain=((struct  BucketDes * )(newpage+1)); break;
                case 32: kbucket.size32chain=((struct  BucketDes * )(newpage+1)); break;
            }
        else
            p->next=((struct  BucketDes * )(newpage+1));    //在新的页中选定第一个块存储32字节的块描述符
        //下面都是使用newpage+1逻辑地址 通过指针类型转换写入描述快初始化数值
        newpage[0]=type/16;//存储该块占用空间值/16，压缩空间
        resaddress=((struct  BucketDes * )(newpage+1));
        ((struct  BucketDes * )(newpage+1))->next=0;
        ((struct  BucketDes * )(newpage+1))->freeoffset=freeoffset*16;
        ((struct  BucketDes * )(newpage+1))->rest=PGSIZE/type-1;
        ((struct  BucketDes * )(newpage+1))->page=newpage;
    }
    return resaddress;
}

/*
char * PiontToPageLogicAddress(char * PDT,char * pageaddress)
{
    int PDToffset=pageaddress & 0xffc00000;
    ((int *)PDT)[1023]=((((uint32_t)pagepionter)-KERNBASE) & 0xfffff000) + (PTE_P | PTE_U | PTE_W);
    ((int * )pagepionter)[0]= ((int *)pageaddress)[PDToffset];
    invlpg(0xffc00000);
}
*/

