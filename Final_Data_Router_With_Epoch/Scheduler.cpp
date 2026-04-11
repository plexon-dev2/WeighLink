/**
 * @file Scheduler.c
 * @brief FreeRTOS Preemptive Task Scheduler Implementation
 * @details Creates and manages periodic FreeRTOS tasks
 *
 * Compatible with ESP32 Arduino Core 3.x
 *
 * *** READ ONLY - DO NOT MODIFY ***
 */

#include <Arduino.h>
#include <string.h>
#include "Scheduler.h"
#include "Scheduler_Cfg.h"
#include "SystemStates.h"

// Note: Serial is automatically provided by ESP32 Arduino Core 3.x

/* ============================================================================
 * SCHEDULER INITIALIZATION
 * ============================================================================ */

void Scheduler_ExecuteInit(void)
{
#if SCHEDULER_CFG_SERIAL_DEBUG
    Serial.println("[SCHEDULER] Executing initialization functions...");
#endif

    uint8_t init_index = 0;

    // Execute all initialization functions in order
    while ((NULL != Init_Functions[init_index]) && (init_index < SCHEDULER_CFG_MAX_INIT_FUNCTIONS))
    {
#if SCHEDULER_CFG_SERIAL_DEBUG
        Serial.print("[SCHEDULER] Init function ");
        Serial.print(init_index + 1);
        Serial.print("... ");
#endif

        Init_Functions[init_index]();

#if SCHEDULER_CFG_SERIAL_DEBUG
        Serial.println("Done");
#endif
        init_index++;
    }

#if SCHEDULER_CFG_SERIAL_DEBUG
    Serial.print("[SCHEDULER] Total ");
    Serial.print(init_index);
    Serial.println(" initialization functions executed");
    Serial.println();
#endif
}

bool Scheduler_Init(void)
{
#if SCHEDULER_CFG_SERIAL_DEBUG
    Serial.println("[SCHEDULER] Creating FreeRTOS tasks...");
    Serial.print("[SCHEDULER] Free heap before tasks: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
#endif

    uint8_t task_index = 0;
    uint8_t tasks_created = 0;

    // Create all tasks from configuration array
    while (NULL != Task_Configurations[task_index].name)
    {
        TaskConfig_t *task = &Task_Configurations[task_index];

#if SCHEDULER_CFG_SERIAL_DEBUG
        Serial.print("[SCHEDULER] Creating task: ");
        Serial.print(task->name);
        Serial.print(" (Period: ");
        Serial.print(task->period_ms);
        Serial.print("ms, Priority: ");
        Serial.print(task->priority);
        Serial.print(", Stack: ");
        Serial.print(task->stack_size);
        Serial.print(" bytes) ... ");
        Serial.flush();
#endif

        // Stack size in words (divide by 4) - common for ESP32
        BaseType_t result = xTaskCreate(
            task->function,       // Task function
            task->name,           // Task name
            task->stack_size / 4, // Stack size in WORDS (4 bytes each)
            (void *)task,         // Pass task config as parameter
            task->priority,       // Priority
            &(task->handle)       // Task handle
        );

        if (pdPASS == result)
        {
            tasks_created++;
#if SCHEDULER_CFG_SERIAL_DEBUG
            Serial.println("OK");
            Serial.print("    Free heap after: ");
            Serial.print(ESP.getFreeHeap());
            Serial.println(" bytes");
            Serial.flush();
#endif
        }
        else
        {
#if SCHEDULER_CFG_SERIAL_DEBUG
            Serial.println("FAILED!");
            Serial.print("    Error code: ");
            Serial.println(result);
            Serial.print("    Free heap: ");
            Serial.print(ESP.getFreeHeap());
            Serial.println(" bytes");
            Serial.flush();
#endif
            return false;
        }

        // Small delay between task creation
        delay(10);
        task_index++;
    }

#if SCHEDULER_CFG_SERIAL_DEBUG
    Serial.println();
    Serial.print("[SCHEDULER] Successfully created ");
    Serial.print(tasks_created);
    Serial.println(" tasks");
    Serial.print("[SCHEDULER] Final free heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.println("[SCHEDULER] FreeRTOS preemptive scheduler ready");
    Serial.println();
    Serial.flush();
#endif

    return true;
}

void Scheduler_Start(void)
{
#if SCHEDULER_CFG_SERIAL_DEBUG
    Serial.println("[SCHEDULER] FreeRTOS scheduler already running in Arduino Core 3.x");
    Serial.println("[SCHEDULER] Tasks will start executing now");
    Serial.println("===============================================");
    Serial.println();
    Serial.flush();
#endif

    // NOTE: In Arduino ESP32 Core 3.x, FreeRTOS scheduler is already running!
    // The setup() function runs in a task, and loop() runs in another task.
    // We don't need to call vTaskStartScheduler() - it's already started!
    // Our tasks will start executing immediately after they're created.
}

uint32_t Scheduler_GetUptime(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

TaskHandle_t Scheduler_GetTaskHandle(const char *task_name)
{
    if (NULL == task_name)
    {
        return NULL;
    }

    uint8_t task_index = 0;

    while (NULL != Task_Configurations[task_index].name)
    {
        if (0 == strcmp(Task_Configurations[task_index].name, task_name))
        {
            return Task_Configurations[task_index].handle;
        }
        task_index++;
    }

    return NULL;
}

void Scheduler_PrintTaskStats(void)
{
#if SCHEDULER_CFG_SERIAL_DEBUG
    Serial.println("===============================================");
    Serial.println("Task Statistics:");
    Serial.println("===============================================");

    uint8_t task_index = 0;

    while (NULL != Task_Configurations[task_index].name)
    {
        TaskConfig_t *task = &Task_Configurations[task_index];

        if (NULL != task->handle)
        {
            // Get stack high water mark (minimum free stack)
            UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(task->handle);

            Serial.print("Task: ");
            Serial.println(task->name);
            Serial.print("  Period: ");
            Serial.print(task->period_ms);
            Serial.println(" ms");
            Serial.print("  Priority: ");
            Serial.println(task->priority);
            Serial.print("  Stack Allocated: ");
            Serial.print(task->stack_size);
            Serial.println(" bytes");
            Serial.print("  Stack Free: ");
            Serial.print(stackLeft);
            Serial.println(" bytes");
            Serial.print("  Stack Usage: ");
            Serial.print((task->stack_size - stackLeft) * 100 / task->stack_size);
            Serial.println("%");
            Serial.println();
        }

        task_index++;
    }

    Serial.print("System Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.println("===============================================");
#endif
}

/* ============================================================================
 * TASK UTILITY FUNCTIONS
 * ============================================================================ */

void Scheduler_ExecuteFunctions(TaskFunctionPtr_t *function_array)
{
    if (NULL == function_array)
    {
        return;
    }

    uint8_t func_index = 0;

    // Execute all functions in the array until NULL is encountered
    while ((NULL != function_array[func_index]) && (func_index < SCHEDULER_CFG_MAX_FUNCTIONS_PER_TASK))
    {
        // Check if function pointer is valid before calling
        if (NULL != function_array[func_index])
        {
            function_array[func_index]();
        }
        func_index++;
    }
}

/* ============================================================================
 * TASK IMPLEMENTATIONS
 * ============================================================================ */
void Task_10ms(void *pvParameters)
{
    TaskConfig_t *config = (TaskConfig_t *)pvParameters;

    if (NULL == config)
    {
        vTaskDelete(NULL);
        return;
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();

#if SCHEDULER_CFG_SERIAL_DEBUG
    Serial.print("[TASK] ");
    Serial.print(config->name);
    Serial.println(" started");
    Serial.flush();
#endif

    while (1)
    {
        Scheduler_ExecuteFunctions(Task_10ms_Functions);

#if SCHEDULER_CFG_PREEMPTION_TEST_MODE
        delayMicroseconds(200);
#endif

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(config->period_ms));
    }
}

void Task_50ms(void *pvParameters)
{
    TaskConfig_t *config = (TaskConfig_t *)pvParameters;

    if (NULL == config)
    {
        vTaskDelete(NULL);
        return;
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();

#if SCHEDULER_CFG_SERIAL_DEBUG
    Serial.print("[TASK] ");
    Serial.print(config->name);
    Serial.println(" started");
    Serial.flush();
#endif

    while (1)
    {
        Scheduler_ExecuteFunctions(Task_50ms_Functions);

#if SCHEDULER_CFG_PREEMPTION_TEST_MODE
        delayMicroseconds(1000);
#endif

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(config->period_ms));
    }
}

void Task_100ms(void *pvParameters)
{
    TaskConfig_t *config = (TaskConfig_t *)pvParameters;

    if (NULL == config)
    {
        vTaskDelete(NULL);
        return;
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();

#if SCHEDULER_CFG_SERIAL_DEBUG
    Serial.print("[TASK] ");
    Serial.print(config->name);
    Serial.println(" started");
    Serial.flush();
#endif

    static uint32_t counter = 0;

    while (1)
    {
        Scheduler_ExecuteFunctions(Task_100ms_Functions);

        counter++;

        // Heartbeat every 10 seconds
        if (0 == (counter % 100))
        {
#if SCHEDULER_CFG_SERIAL_DEBUG
            Serial.print("[HEARTBEAT] Uptime: ");
            Serial.print(Scheduler_GetUptime());
            Serial.println(" ms");
            Serial.flush();
#endif

            // Print task statistics every 10 seconds
            // Scheduler_PrintTaskStats();  // Uncomment to enable
        }

#if SCHEDULER_CFG_PREEMPTION_TEST_MODE
        delayMicroseconds(2000);
#endif

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(config->period_ms));
    }
}