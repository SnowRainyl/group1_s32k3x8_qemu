#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    /* 栈溢出处理 */
    for(;;);
}

void vApplicationMallocFailedHook(void)
{
    /* 内存分配失败处理 */
    for(;;);
}

// LED 闪烁任务
void vLEDTask(void *pvParameters)
{
    while(1) {
        // 模拟 LED 闪烁
        printf("LED Toggle\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// UART 打印任务
void vUARTTask(void *pvParameters)
{
    while(1) {
        printf("UART Task Running\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

int main(void)
{
    // 创建任务
    xTaskCreate(vLEDTask, "LED Task", 128, NULL, 1, NULL);
    xTaskCreate(vUARTTask, "UART Task", 128, NULL, 1, NULL);
    
    // 启动调度器
    vTaskStartScheduler();
    
    // 正常情况下不会到达这里
    while(1);
}
