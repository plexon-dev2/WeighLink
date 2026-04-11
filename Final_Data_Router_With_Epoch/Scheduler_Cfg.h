/**
 * @file Scheduler_Cfg.h
 * @brief Scheduler Configuration File
 * 
 * ============================================================================
 * SCHEDULER COMPILE-TIME CONFIGURATION FILE
 * ============================================================================
 * Change the values and configurations below to customize the scheduler
 * behavior and configure your application tasks.
 * All settings are applied at compile time.
 * 
 * This is the ONLY scheduler file that users should modify.
 * DO NOT modify Scheduler.h or Scheduler.c
 */

#ifndef SCHEDULER_CFG_H
#define SCHEDULER_CFG_H

#include <stdint.h>
#include <stdbool.h>
#include "CmdParser.h"
#include "Modbus.h"
#include "App.h"
#include "SystemStates.h"

// Note: FreeRTOS headers are included by Scheduler.h

// ============================================================================
// SCHEDULER CONFIGURATION CONSTANTS
// ============================================================================

#define SCHEDULER_CFG_MAX_INIT_FUNCTIONS        10
#define SCHEDULER_CFG_MAX_FUNCTIONS_PER_TASK    10
#define SCHEDULER_CFG_SERIAL_DEBUG              1
#define SCHEDULER_CFG_PREEMPTION_TEST_MODE      0

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

typedef void (*InitFunc_t)(void);
typedef void (*TaskFunctionPtr_t)(void);
typedef void (*TaskFunction_t)(void *);

typedef struct {
    const char*         name;
    TaskFunction_t      function;
    uint32_t            period_ms;
    uint16_t            stack_size;
    uint8_t             priority;
    TaskHandle_t        handle;
} TaskConfig_t;

// ============================================================================
// USER APPLICATION FUNCTION PROTOTYPES
// ============================================================================

// Periodic task functions (FreeRTOS tasks)
//void Task_2ms(void *pvParameters);
void Task_10ms(void *pvParameters);
void Task_50ms(void *pvParameters);
void Task_100ms(void *pvParameters);

// ============================================================================
// INITIALIZATION FUNCTIONS ARRAY
// ============================================================================

static InitFunc_t Init_Functions[] = {    
    CmdParser__Init,
    Modbus_Init,
    App__Init,
    SystemStates__Init,
    NULL  // MUST be NULL terminated - DO NOT REMOVE!
};

// ============================================================================
// TASK FUNCTION ARRAYS
// ============================================================================
static TaskFunctionPtr_t Task_10ms_Functions[]  = {

    NULL  // Moved ADS1115S to 100ms task
 };

static TaskFunctionPtr_t Task_50ms_Functions[] __attribute__((unused)) = {
    CmdParser__Handler,   
    SystemStates__Handler, 
    App__Handler,    
    NULL  // Add your 20ms functions here
};

 static TaskFunctionPtr_t Task_100ms_Functions[] __attribute__((unused)) = {
    // Moved here - less frequent, more stable
    Modbus_Handler,
    NULL  // Add your 100ms functions here
 };

// ============================================================================
// TASK CONFIGURATION ARRAY
// ============================================================================

static TaskConfig_t Task_Configurations[] = {
    // { name,         function,     period_ms,   stack_size,      priority,  handle }
    { "Task_10ms",     Task_10ms,     10,            4096,           3,         NULL },
    { "Task_50ms",     Task_50ms,     50,            12288,          2,         NULL }, 
    { "Task_100ms",    Task_100ms,    100,           10000,          1,         NULL },  // LARGE stack for ADS1115S I2C!    
    // MUST end with NULL entry - DO NOT REMOVE!
    { NULL,           NULL,        0,          0,             0,         NULL }
};

// ============================================================================
// HELPER MACROS
// ============================================================================

#define GET_INIT_FUNC_COUNT() \
    ((sizeof(Init_Functions) / sizeof(InitFunc_t)) - 1)

#define GET_TASK_COUNT() \
    ((sizeof(Task_Configurations) / sizeof(TaskConfig_t)) - 1)

#endif // SCHEDULER_CFG_H