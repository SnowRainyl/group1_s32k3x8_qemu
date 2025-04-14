/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
* Application specific definitions for S32K3X8
*----------------------------------------------------------*/

/* Cortex-M7 specific definitions. */
#define configPRIO_BITS                          4   /* 4 priority bits for S32K3X8 */

/* Basic configurations */
#define configUSE_PREEMPTION                     1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1   /* Use Cortex-M7 optimized selection */
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configCPU_CLOCK_HZ                       ( ( unsigned long ) 240000000 )  /* 240MHz max for S32K3X8 */
#define configTICK_RATE_HZ                       ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                     ( 9UL )
#define configMINIMAL_STACK_SIZE                 ( ( unsigned short ) 128 )  /* Larger stack for Cortex-M7 */
#define configTOTAL_HEAP_SIZE                    ( ( size_t ) ( 128 * 1024 ) )  /* 128KB heap */
#define configMAX_TASK_NAME_LEN                  ( 16 )
#define configUSE_16_BIT_TICKS                   0
#define configIDLE_SHOULD_YIELD                  1
#define configUSE_TASK_NOTIFICATIONS             1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES    3

/* Memory allocation related definitions */
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1

/* Hook function related definitions */
#define configUSE_MALLOC_FAILED_HOOK             1
#define configCHECK_FOR_STACK_OVERFLOW           2

/* Run time and task stats gathering related definitions */
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* Co-routine related definitions */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         ( 2 )

/* Software timer related definitions */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* Interrupt nesting behavior configuration */
#define configKERNEL_INTERRUPT_PRIORITY         ( 15 << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( 5 << (8 - configPRIO_BITS) )

/* Define to trap errors during development */
#define configASSERT( x )                       if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* Optional functions - most linkers will remove unused functions anyway */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_xTimerGetTimerDaemonTaskHandle  1
#define INCLUDE_xQueueGetMutexHolder            1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1

/* Other features configuration */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    1
#define configQUEUE_REGISTRY_SIZE               10

/* Backward compatibility */
#define configENABLE_BACKWARD_COMPATIBILITY     0



#endif /* FREERTOS_CONFIG_H */
