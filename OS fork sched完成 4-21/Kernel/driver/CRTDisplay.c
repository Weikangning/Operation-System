//
// Created by wkn on 2021/3/10.
//
#include <CRTDisplay.h>



static uint16_t *crt_buf;
static uint16_t crt_pos;
static uint16_t addr_6845;

/* stupid I/O delay routine necessitated by historical PC design flaws */
 void delay(void) {
    inb(0x84);
    inb(0x84);
    inb(0x84);
    inb(0x84);
}

 void cga_init(void) {
    //VGA显示模式的缓冲地址
    volatile uint16_t *DISPLAY_BUF = (uint16_t *)(CGA_BUF);
    uint16_t recordDISPLAY_BUF = *DISPLAY_BUF;
    //0xA55A是一个测试字符，检测能否写入DISPLAY_BUF，如二次读取一致，则当前VGA模式，否则为MONO模式，并修改相关IO端口
    *DISPLAY_BUF = (uint16_t) 0xA55A;
    if (*DISPLAY_BUF != 0xA55A) {
        DISPLAY_BUF = (uint16_t*)(MONO_BUF );
        addr_6845 = MONO_BASE;
    } else {
        *DISPLAY_BUF = recordDISPLAY_BUF;
        addr_6845 = CGA_BASE;
    }

    //获取光标位置 通过索引寄存器及 0xe 0xf端口来读写
    uint32_t pos;
    outb(addr_6845, 14);
    pos = inb(addr_6845 + 1) << 8;
    outb(addr_6845, 15);
    pos |= inb(addr_6845 + 1);

    crt_buf = (uint16_t*) DISPLAY_BUF;
    crt_pos = pos;
}

/* cga_putc - print character to console */
 void cga_putc(int c) {
    //字符是一个字节，字符显示模式下，一个字符要两个字节，一个字节存储字体颜色及背景颜色，一个字节存储ascii
    // 设置字体为白色
    if (!(c & ~0xFF)) {
        c |= 0x0700;
    }

    switch (c & 0xff) {
        case '\b':
            if (crt_pos > 0) {
                crt_pos --;
                crt_buf[crt_pos] = (c & ~0xff) | ' ';
            }
            break;
        case '\n':
            crt_pos += CRT_COLS;
        case '\r':
            crt_pos -= (crt_pos % CRT_COLS);
            break;
        default:
            crt_buf[crt_pos ++] = c;     // write the character
            break;
    }

    if(crt_pos>=(CRT_ROWS*CRT_COLS))
    {
        int * crt_mov=(int * )(crt_buf+CRT_COLS);
        for (crt_pos=0;crt_pos<(CRT_ROWS-1)*CRT_COLS;crt_pos++)
        {
            crt_buf[crt_pos]=crt_mov[crt_pos];
            outb(addr_6845, 14);
            outb(addr_6845 + 1, crt_pos >> 8);
            outb(addr_6845, 15);
            outb(addr_6845 + 1, crt_pos);
        }
        return;
    }

    outb(addr_6845, 14);
    outb(addr_6845 + 1, crt_pos >> 8);
    outb(addr_6845, 15);
    outb(addr_6845 + 1, crt_pos);
}