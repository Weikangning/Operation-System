//
// Created by wkn on 2021/3/19.
//

#include <defs.h>
#include <stdio.h>
#include <CRTDisplay.h>
#include <stdarg.h>


//目前无法显示负数，负数取反按正数输出
void printint(long int arg)
{
    unsigned long int num=arg;
    if(num<0)
        num=0-num;
    long int class=1;
    for (int i=9;i>=0;i--)
    {
        class=1;
        for (int j=0;j<i;class*=10,j++);
        int printnum=num/class;
        cga_putc(printnum+'0');
        num=num-class*printnum;
    }


}
void printstring(long int arg)
{
    char * stringpionter= (char *) arg;
    for (int i=0;stringpionter[i]!=0;i++)
        cga_putc(stringpionter[i]);
}
void printchar(int arg)
{
    char * charpionter= (char *) arg;
    cga_putc(charpionter[0]);
}


/* *
 * cprintf - formats a string and writes it to stdout
 *
 * The return value is the number of characters which would be
 * written to stdout.
 * */
int cprintf(const char *fmt, ...) {

    long int arg[10]={0};
    int cnt=0;
    int argnum=0;
    //通过输出格式fmt中的%作为 可变参数个数的考量
    for (int i=0;fmt[i]!=0;i++)
    {
        if(fmt[i]=='%')
            argnum++;
    }
    if(argnum>=10)
        argnum=10;
    //获取所有参数，最大参数为10

    va_list ap;
    va_start(ap, fmt);
    for (int i = 0; i < argnum; i++)
    {
        arg[i] = va_arg(ap, int);
    }
    va_end(ap);

    for (int i=0;fmt[i]!=0;i++)
    {
        if(fmt[i]=='%')
        {
            i++;
            switch (fmt[i]) {
                case 'd' :   printint(arg[cnt]);   break;
                case 's' :   printstring(arg[cnt]);   break;
                case 'c' :   printchar(arg[cnt]);   break;
            }
            cnt++;
            continue;
        }
        if(fmt[i]=='\\')
        {
            i++;
            switch(fmt[i]){
                case 'n' : cga_putc('\n'); break;
                case 't' : for (int j=0;j<4;cga_putc(' '), j++); break;
            }
            continue;
        }

        cga_putc(fmt[i]);
    }

    return cnt;
}