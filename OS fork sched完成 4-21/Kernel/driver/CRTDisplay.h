//
// Created by wkn on 2021/3/10.
//

#ifndef OS_CRTDISPLAY_H
#define OS_CRTDISPLAY_H

#include <defs.h>

#include <x86.h>

#include <MMLayout.h>

#define MONO_BASE       0x3B4
#define MONO_BUF        (0xB0000+KERNBASE)
#define CGA_BASE        0x3D4
#define CGA_BUF         (0xB8000+KERNBASE)
#define CRT_ROWS        25
#define CRT_COLS        80
#define CRT_SIZE        (CRT_ROWS * CRT_COLS)

 void delay(void) ;

 void cga_init(void);

/* cga_putc - print character to console */
 void cga_putc(int c);

#endif //OS_CRTDISPLAY_H
