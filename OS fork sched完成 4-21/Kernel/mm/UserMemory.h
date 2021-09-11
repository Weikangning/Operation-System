//
// Created by wkn on 2021/4/16.
//

#ifndef OS_USERMEMORY_H
#define OS_USERMEMORY_H
#include <defs.h>
#include <x86.h>
struct UserMemoryRes
{
    char * logicaddress;
    uint32_t phypageindex;
};

//用户空间开辟内存
struct UserMemoryRes UserMalloc4KB(int num);

//用户空间页 逻辑地址与虚拟地址映射
void MapPhyToLogicAddress(char * logicaddress,char *phyaddress,int num);

//查询页表获取一个num连续可用的逻辑空间 4KB粒度
char * GetFreeUserLogicAddress(int num);

//TODO 暂不启用通过用户空间的逻辑地址 查询页表得到物理页
uint32_t UserGetPhyPageAddressByLogicAddress(char * logic);

#endif //OS_USERMEMORY_H
