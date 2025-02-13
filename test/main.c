#include <stdint.h>
void startup_go_to_user_mode(void)
{
    // 空实现即可
    return;
}
void SystemInit(void)
{
    // 在这个阶段我们不需要做任何系统初始化
    // CPU 的寄存器在 startup_cm7.s 中已经被清零
}

int main(void)
{
    while(1) {
        // 空循环
        __asm("nop");
    }
    return 0;
}
