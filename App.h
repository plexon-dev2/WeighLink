/**
 * @file App.h
 * @brief Application Module Interface
 * @details Public interface for main application logic
 *
 * @author 
 * @date 2025-12-02
 * @version 2.0.0
 */

#ifndef APP_H
#define APP_H

/*==============================================================================
 *                              INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "App_Cfg.h"

/*==============================================================================
 *                          FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * @defgroup App_SchedulerAPIs Scheduler Interface APIs
 * @brief APIs called by the scheduler for module operation
 * @{
 */

/**
 * @brief Initialize Application Module
 * @details Must be called once during system initialization
 *
 * @note This API is called by the scheduler during system initialization
 */
void App__Init(void);

/**
 * @brief Main Application Handler
 * @details Should be called periodically by the scheduler (e.g., every 50ms)
 *
 * @note This API is called by the scheduler in the main execution loop
 */
void App__Handler(void);

/** @} */

/**
 * @defgroup App_StatusAPIs Status Query APIs
 * @brief APIs for querying system status
 * @{
 */

/**
 * @brief Get RS232 power supply error status
 *
 * @return true if power supply error detected
 * @return false if power supply is normal
 */
bool App__IsRS232PowerSupplyError(void);

/**
 * @brief Get RS485 power supply error status
 *
 * @return true if power supply error detected
 * @return false if power supply is normal
 */
bool App__IsRS485PowerSupplyError(void);

/**
 * @brief Get RS232 communication error status
 *
 * @return true if communication error detected
 * @return false if communication is normal
 */
bool App__IsRS232CommError(void);

/**
 * @brief Get RS485 communication error status
 *
 * @return true if communication error detected
 * @return false if communication is normal
 */
bool App__IsRS485CommError(void);

/** @} */

/**
 * @defgroup App_UtilityAPIs Utility APIs
 * @brief Utility functions for module control
 * @{
 */

/**
 * @brief Clear all error flags
 * @details Resets all error conditions and debounce counters
 */
void App__ClearErrors(void);

/** @} */

#endif // APP_H