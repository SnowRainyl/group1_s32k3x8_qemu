#include <stdint.h>

/* 主函数 - 简化版本，仅做基本初始化 */
int main(void)
{
    /* 一个简单的循环，不访问任何外设 */
    volatile uint32_t counter = 0;
    
    /* 无限循环 */
    while(1) {
        /* 简单计数，防止编译器优化 */
        counter++;
        
        /* 每计数一定次数后重置 */
        if (counter > 1000000) {
            counter = 0;
        }
    }
    
    /* 永远不会到达这里 */
    return 0;
}
