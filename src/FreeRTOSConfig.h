#pragma once

#include <libopencm3/stm32/rcc.h>

#define pdTICKS_TO_MS(xTicks) ((uint32_t) (xTicks) * 1000 / configTICK_RATE_HZ)

#define configUSE_PREEMPTION					1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE					1
#define configCPU_CLOCK_HZ						(rcc_ahb_frequency)
#define configTICK_RATE_HZ						1000
#define configMAX_PRIORITIES					7
#define configMINIMAL_STACK_SIZE				128
#define configTOTAL_HEAP_SIZE					3000
#define configMAX_TASK_NAME_LEN					16
#define configUSE_16_BIT_TICKS					0
#define configIDLE_SHOULD_YIELD					1
#define configUSE_TASK_NOTIFICATIONS			1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#define configUSE_MUTEXES						1
#define configUSE_RECURSIVE_MUTEXES				1
#define configUSE_COUNTING_SEMAPHORES			0
#define configQUEUE_REGISTRY_SIZE				8
#define configUSE_QUEUE_SETS					0
#define configUSE_TIME_SLICING					0
#define configUSE_NEWLIB_REENTRANT				0
#define configENABLE_BACKWARD_COMPATIBILITY		0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5
#define configSTACK_DEPTH_TYPE					uint16_t
#define configMESSAGE_BUFFER_LENGTH_TYPE		size_t

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION			1
#define configSUPPORT_DYNAMIC_ALLOCATION		0

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK						0
#define configUSE_TICK_HOOK						0
#define configCHECK_FOR_STACK_OVERFLOW			0
#define configUSE_MALLOC_FAILED_HOOK			0
#define configUSE_DAEMON_TASK_STARTUP_HOOK		0

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS			0
#define configUSE_TRACE_FACILITY				0
#define configUSE_STATS_FORMATTING_FUNCTIONS	0

/* Co-routine related definitions. */
#define configUSE_CO_ROUTINES					0
#define configMAX_CO_ROUTINE_PRIORITIES			2

/* Software timer related definitions. */
#define configUSE_TIMERS						1
#define configTIMER_TASK_PRIORITY				2
#define configTIMER_QUEUE_LENGTH				10
#define configTIMER_TASK_STACK_DEPTH			configMINIMAL_STACK_SIZE

/* Interrupt nesting behaviour configuration. */
#define configPRIO_BITS									2
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY			3
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY	3
#define configKERNEL_INTERRUPT_PRIORITY					(configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY			(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_API_CALL_INTERRUPT_PRIORITY			configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY

/* Define to trap errors during development. */
#define configASSERT_ENABLE						0

#if configASSERT_ENABLE
	void vAssertCalled(const char *filename, uint32_t line);
	#define configASSERT(x) do { if ((x) == 0) { taskDISABLE_INTERRUPTS(); vAssertCalled(__FILE__, __LINE__); } } while (0);
#endif

/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet				0
#define INCLUDE_uxTaskPriorityGet				0
#define INCLUDE_vTaskDelete						0
#define INCLUDE_vTaskSuspend					0
#define INCLUDE_xResumeFromISR					0
#define INCLUDE_vTaskDelayUntil					0
#define INCLUDE_vTaskDelay						1
#define INCLUDE_xTaskGetSchedulerState			0
#define INCLUDE_xTaskGetCurrentTaskHandle		0
#define INCLUDE_uxTaskGetStackHighWaterMark		0
#define INCLUDE_xTaskGetIdleTaskHandle			0
#define INCLUDE_eTaskGetState					0
#define INCLUDE_xEventGroupSetBitFromISR		0
#define INCLUDE_xTimerPendFunctionCall			0
#define INCLUDE_xTaskAbortDelay					0
#define INCLUDE_xTaskGetHandle					0
#define INCLUDE_xTaskResumeFromISR				0

/* Handlers */
#define vPortSVCHandler			sv_call_handler
#define xPortPendSVHandler		pend_sv_handler
#define xPortSysTickHandler		sys_tick_handler
