#include"types.h"
#include"sbi.h"
#include"stddef.h"

// QEMU中时钟的频率是10MHz, 也就是1秒钟相当于10000000个时钟周期。
unsigned long TIMECLOCK = 10000000;

unsigned long get_cycles() {
    // 使用 rdtime 编写内联汇编，获取 time 寄存器中 (也就是mtime 寄存器 )的值并返回
    // YOUR CODE HERE
    unsigned long time;
    __asm__ volatile
    (
        "rdtime t1\n"
        "mv %[time], t1\n"

        : [time] "=r" (time)
        :
        : "memory" 
    );

    return time;
}

void clock_set_next_event() {
    // 下一次 时钟中断 的时间点
    unsigned long next = get_cycles() + TIMECLOCK;

    // 使用 sbi_ecall 来完成对下一次时钟中断的设置
    // YOUR CODE HERE
    //struct sbi_ret ret;
    sbi_ecall(0x0, 0x0, next, 0, 0, 0, 0, 0);
}