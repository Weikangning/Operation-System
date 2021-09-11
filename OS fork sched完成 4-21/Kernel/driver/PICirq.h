//
// Created by wkn on 2021/4/14.
//

#ifndef OS_PICIRQ_H
#define OS_PICIRQ_H
void pic_init(void);
void pic_enable(unsigned int irq);

#define IRQ_OFFSET      32
#endif //OS_PICIRQ_H
