#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

/* S32E8 LPUART寄存器定义 - 与QEMU模拟代码保持一致 */
#define LPUART0_BASE         0x40000000   /* 根据你的板子可能需要调整 */



#define VERID_OFFSET      0x00
#define PARAM_OFFSET      0x04
#define GLOBAL_OFFSET     0x08
#define PINCFG_OFFSET     0x0C
#define BAUD_OFFSET       0x10
#define STAT_OFFSET       0x14
#define CTRL_OFFSET       0x18
#define DATA_OFFSET       0x1C

/* 寄存器位定义 */
#define GLOBAL_RST        (1 << 0)
#define GLOBAL_ENABLE     (1 << 1)
#define CTRL_TE           (1 << 19)  // 发送使能位正确位置
#define CTRL_RE           (1 << 18)  // 接收使能位正确位置

/* 主函数 */
int main(void)
{
    volatile uint32_t *lpuart_global = (volatile uint32_t *)(LPUART0_BASE + GLOBAL_OFFSET);
    volatile uint32_t *lpuart_baud = (volatile uint32_t *)(LPUART0_BASE + BAUD_OFFSET);
    volatile uint32_t *lpuart_ctrl = (volatile uint32_t *)(LPUART0_BASE + CTRL_OFFSET);
    volatile uint32_t *lpuart_data = (volatile uint32_t *)(LPUART0_BASE + DATA_OFFSET);
    
    // 重置LPUART
    *lpuart_global = GLOBAL_RST;
    
    // 设置波特率寄存器
    *lpuart_baud = 0x0F000034;  // 115200波特率，假设时钟为160MHz
    
    // 启用LPUART
    *lpuart_global = GLOBAL_ENABLE;
    
    // 配置LPUART (启用发送)
    *lpuart_ctrl = CTRL_TE | CTRL_RE;
    
    // 发送字符串
    const char *msg = "Hello from S32K3X8EVB!\r\n";
    const char *p = msg;
    while (*p) {
        *lpuart_data = *p++;
        
        // 简单延迟
        for (volatile int i = 0; i < 10000; i++);
    }
    
    while(1); // 无限循环
}
