/**
 * @file App_Cfg.h
 * @brief Application Module Configuration
 * @details Configuration parameters for application module
 *
 * @author 
 * @date 2025-12-18
 * @version 2.1.0
 */

#ifndef APP_CFG_H
#define APP_CFG_H

/*==============================================================================
 *                              INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 *                              DEFINES
 *============================================================================*/

/**
 * @defgroup App_GPIO GPIO Pin Configuration
 * @brief GPIO pin assignments for power supply monitoring
 * @{
 */

/**
 * @brief RS232 isolated power supply monitor pin
 * @details GPIO pin to monitor RS232 power supply status
 * @note HIGH = Power supply error, LOW = Normal
 * @note Valid range: 0-21 (ESP32-C3)
 */
#define APP_RS232_ISO_POWER_PIN 8

/**
 * @brief RS485 isolated power supply monitor pin
 * @details GPIO pin to monitor RS485 power supply status
 * @note HIGH = Power supply error, LOW = Normal
 * @note Valid range: 0-21 (ESP32-C3)
 */
#define APP_RS485_ISO_POWER_PIN 9

/** @} */

/**
 * @defgroup App_Timing Timing Configuration
 * @brief Timing and debounce parameters
 * @{
 */

/**
 * @brief Power supply debounce count
 * @details Number of consecutive error readings required before flagging error
 * @note At 50ms task rate: 10 counts = 500ms debounce time
 * @note At 100ms task rate: 10 counts = 1000ms debounce time
 */
#define APP_ISO_POWER_DEBOUNCE_COUNT 10U

/**
 * @brief Display update interval
 * @details Number of handler calls before updating display
 * @note At 100ms task rate: 10 counts = 1 second update interval
 * @note At 50ms task rate: 20 counts = 1 second update interval
 */
#define APP_DISPLAY_UPDATE_INTERVAL 10U

/** @} */

/**
 * @defgroup App_Features Feature Configuration
 * @brief Enable/disable application features
 * @{
 */

/**
 * @brief Enable debug output
 * @details Set to 1 to enable debug messages, 0 to disable
 */
#define APP_DEBUG_ENABLE 1

/**
 * @brief Enable power supply monitoring
 * @details Set to 1 to enable power monitoring, 0 to disable
 */
#define APP_POWER_MONITORING_ENABLE 1

/**
 * @brief Enable communication monitoring
 * @details Set to 1 to enable communication monitoring, 0 to disable
 */
#define APP_COMM_MONITORING_ENABLE 1

/**
 * @brief Enable heap monitoring
 * @details Set to 1 to periodically log free heap, 0 to disable
 * @note Only logged every 100 messages to reduce serial spam
 */
#define APP_HEAP_MONITORING_ENABLE 1

/**
 * @brief Heap monitoring interval
 * @details Log heap every N messages
 */
#define APP_HEAP_LOG_INTERVAL 100U

/** @} */

/*==============================================================================
 *                          VALIDATION MACROS
 *============================================================================*/

/* Validate GPIO pin ranges for ESP32-C3 */
#if (APP_RS232_ISO_POWER_PIN < 0) || (APP_RS232_ISO_POWER_PIN > 21)
#error "APP_RS232_ISO_POWER_PIN must be between 0 and 21 (ESP32-C3 range)"
#endif

#if (APP_RS485_ISO_POWER_PIN < 0) || (APP_RS485_ISO_POWER_PIN > 21)
#error "APP_RS485_ISO_POWER_PIN must be between 0 and 21 (ESP32-C3 range)"
#endif

/* Validate timing parameters */
#if (APP_ISO_POWER_DEBOUNCE_COUNT < 1U) || (APP_ISO_POWER_DEBOUNCE_COUNT > 100U)
#error "APP_ISO_POWER_DEBOUNCE_COUNT must be between 1 and 100"
#endif

#if (APP_DISPLAY_UPDATE_INTERVAL < 1U) || (APP_DISPLAY_UPDATE_INTERVAL > 100U)
#error "APP_DISPLAY_UPDATE_INTERVAL must be between 1 and 100"
#endif

/* Validate heap monitoring interval */
#if (APP_HEAP_LOG_INTERVAL < 1U)
#error "APP_HEAP_LOG_INTERVAL must be at least 1"
#endif

/*==============================================================================
 *                          HELPER MACROS
 *============================================================================*/

/**
 * @brief Check if debug output is enabled
 * @details Used for conditional compilation of debug code
 */
#if (APP_DEBUG_ENABLE == 1)
    #define APP_DEBUG_PRINT(x) Serial.print(x)
    #define APP_DEBUG_PRINTLN(x) Serial.println(x)
    #define APP_DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
    #define APP_DEBUG_PRINT(x)
    #define APP_DEBUG_PRINTLN(x)
    #define APP_DEBUG_PRINTF(...)
#endif

#endif /* APP_CFG_H */