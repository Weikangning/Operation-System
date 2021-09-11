//
// Created by wkn on 2021/4/16.
//

#include <UserMemory.h>
#include <memoryInit.h>
#include <process.h>
#include <MMU.h>
#include <MMLayout.h>
//int getpageindex(int num_n,int startoffset)

//TODO 用户空间开辟内存
struct UserMemoryRes UserMalloc4KB(int num)
{
    struct UserMemoryRes Res;
    //在struct page数组中 128MB空间起始处查找符合的空间
    int freephyindex=getpageindex(num, 128*1024/4);

    char * usablelogicaddress=GetFreeUserLogicAddress(num);

    if(0==freephyindex || 0==usablelogicaddress)
        {Res.phypageindex=freephyindex;Res.logicaddress=usablelogicaddress; return Res;}
    else
        MapPhyToLogicAddress(usablelogicaddress,freephyindex*4*1024,num);

    Res.logicaddress=usablelogicaddress;
    Res.phypageindex=freephyindex;
    return Res;
}

//TODO 用户空间页 逻辑地址与虚拟地址映射
void MapPhyToLogicAddress(char * logicaddress,char *phyaddress,int num)
{
    int * PDT=(int *)(CurrentProcess->cr3);
    for (int i=0;i<num;i++)
    {
        logicaddress += 4*1024;
        phyaddress += 4*1024;
        int PDToffset = ((uint32_t)logicaddress & 0xffc00000)>>22;

        int PToffset = ((uint32_t)logicaddress & 0x3ff000) >>12;

        if(0==PDT[PDToffset])
        {
            int * newpage= (int *)kmalloc4KB(1);    //页表需要由内核管理
            //新页表填入PDT
            PDT[PDToffset]=((((uint32_t)newpage)-KERNBASE) & 0xfffff000) + (PTE_P| PTE_W);    //这里权限没有 PTE_U 因为这是用户进程
            //新页表初始化
            for (int n=0;n<1024;n++)
                newpage[n]=0;
        }
        //获取页表逻辑地址
        int * pagelogic= (int *)((PDT[PDToffset] +KERNBASE) & 0xfffff000);
        //物理地址与逻辑地址映射
        pagelogic[PToffset]= ((uint32_t)phyaddress & 0xfffff000) + (PTE_P| PTE_W);
    }
}

//查询页表获取一个可用的逻辑空间 4KB粒度
char * GetFreeUserLogicAddress(int num)
{
    int count=0;//本次查询连续空闲页表项数
    uint32_t prestartlogicaddress=0;
    char findflag=1;

    uint32_t USERCR3= CurrentProcess->cr3;   //是一个内核地址，转换为可访问的逻辑地址

    int * USERCR3Logicaddress=(int * )(USERCR3+KERNBASE);

    for (int i=0;i<768;i++)
    {
        if(0!=USERCR3Logicaddress[i])
        {
            //PDT i项物理地址换算为逻辑地址
            int * pageilogicaddress=((USERCR3Logicaddress[i] & 0xfffffc00)+KERNBASE);
            for (int j=0;j<1024;j++)
            {
                findflag=1;
                if(0 == pageilogicaddress[j])   //页表该项为空 从此处开始探测连续的num逻辑地址空间是否为空
                {
                    prestartlogicaddress=4*1024*1024*i + j*4*1024;
                    //从此处开始探测，prestartlogicaddress累加4KB，将其转换为PDT与页表查看是否空闲，若有一处占用则失败，从失败处重新探测

                    int * tmpprestartlogicaddress=prestartlogicaddress;
                    for(count=0;count<num;)
                    {
                        //当前探测逻辑地址的页目录偏移量
                        tmpprestartlogicaddress = (int *)((char *)tmpprestartlogicaddress+ 4*1024);
                        int prestartlogicaddressPDToffset = ((uint32_t)tmpprestartlogicaddress & 0xffc00000)>>22;
                        //当前探测逻辑地址的页表偏移量
                        int prestartlogicaddressPToffset = ((uint32_t)tmpprestartlogicaddress & 0x3ff000) >>12;
                        if(0 == USERCR3Logicaddress[prestartlogicaddressPDToffset])//PDT该项为空 4MB空间都可用
                        {
                            if( num <= count+1024 )   //PDT为空，且开辟空间小于count+4MB，则找到，将其返回
                                return (char * )tmpprestartlogicaddress;
                            else
                            {
                                tmpprestartlogicaddress = (int *)((char *)tmpprestartlogicaddress+4*1024*1024);    //探测地址加4MB
                                count += 1024;
                            }
                        }
                        else
                        {//PDT不为空，考察该MB空间中是否连续可用
                            //当前地址PDT对应的页表物理地址 换算成逻辑地址
                            int * PTLogicAddress = (int *)((USERCR3Logicaddress[prestartlogicaddressPDToffset] & 0xfffffc00)+KERNBASE);
                            if(0!=PTLogicAddress[prestartlogicaddressPToffset]) //某个4KB空间非空闲，本次探测失败
                            {
                                findflag=0;
                                //i与j状态更新
                                j=prestartlogicaddressPToffset;
                                if(j==1023)
                                    i=prestartlogicaddressPDToffset;
                                else
                                    i=prestartlogicaddressPDToffset-1;

                                break;
                            }
                            count++;
                        }
                    }
                    if(1==findflag)
                        return (char * )prestartlogicaddress;
                }
            }
        }
    }
    return 0;
}

//TODO 暂不启用通过用户空间的逻辑地址 查询页表得到物理页
uint32_t UserGetPhyPageAddressByLogicAddress(char * logic);
