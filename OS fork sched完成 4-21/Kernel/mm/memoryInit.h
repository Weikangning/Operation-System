//
// Created by wkn on 2021/3/19.
//

#ifndef OS_MEMORYINIT_H
#define OS_MEMORYINIT_H
#include <defs.h>
static void pageRecordInit();

void memory_init();

static void mapPageForKernel();

void reloadGDT();

static void getMaxMemory();

void getPage();

void mapVMA();

char * kmalloc4KB(int num_n);

char * kernelgetpages(int num_n);

char * mallocBucketDes();

char * bucketmalloc(int byte);

void load_esp0(uintptr_t esp0);

int getpageindex(int num_n,int startoffset);


struct BucketDes
{
    char * page;    //分配给他的4KB页逻辑地址
    struct BucketDes * next;    //下一个struct BucketDes地址
    int rest;   //剩余可分配的空间 比如：16字节，总可用空间为1024*4/（16）
    int freeoffset;//4KB页内尚未使用的空间offset 链表头
};

struct BucketDirectory
{
    struct BucketDes * size16chain;
    struct BucketDes * size32chain;
    struct BucketDes * size64chain;
    struct BucketDes * size128chain;
    struct BucketDes * size256chain;
    struct BucketDes * size512chain;
    struct BucketDes * size1024chain;
};


#endif //OS_MEMORYINIT_H
