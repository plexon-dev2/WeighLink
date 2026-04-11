// /**
//  * @file LED_HMI_Cfg.h
//  * @brief LED HMI Configuration File
//  * @details Configuration for LED hardware and timing specifications
//  *
//  * LED SPECIFICATIONS:
//  * ==================
//  * 1. GREEN LED  - Power indicator (hardwired to VCC/GND, no GPIO)
//  * 2. BLUE LED   - Module test (GPIO 7): ON 2500ms, OFF 500ms (continuous)
//  * 3. RED LED    - RS232 data (GPIO 2): Blink once 200ms when data received
//  * 4. YELLOW LED - RS485 dual behavior (GPIO 3):
//  *    - Poll mode: Blink continuously 100ms ON / 100ms OFF
//  *    - Data mode: Solid ON for 1000ms when valid data received
//  * 
//  * @author 
//  * @date 2025-12-19
//  * @version 2.0.0
//  */

// #ifndef LED_HMI_CFG_H
// #define LED_HMI_CFG_H

// /*==============================================================================
//  *                              INCLUDES
//  *==============================================================================*/
// #include "LED_HMI.h"

// /*==============================================================================
//  *                              DEFINES
//  *==============================================================================*/

// /**
//  * @defgroup LED_Pins LED Pin Definitions
//  * @brief GPIO pin assignments for ESP32-C3
//  * @{
//  */
// #define LED_BLUE_PIN (7U)   /**< Module test indicator */
// #define LED_RED_PIN (6U)    /**< RS232 data indication (Active Low) */
// #define LED_YELLOW_PIN (0U) /**< RS485 indication (Active Low) */
// /** @} */

// /**
//  * @defgroup LED_Timing LED Timing Configurations
//  * @brief Timing parameters in milliseconds
//  * @{
//  */

// /* BLUE LED - Module Testing (Continuous ON/OFF cycle) */
// #define LED_BLUE_ON_MS (2500U)  /**< ON duration: 2500ms */
// #define LED_BLUE_OFF_MS (500U)  /**< OFF duration: 500ms */
// #define LED_BLUE_ACTIVE_LOW (0U) /**< Active Low = 0 (inverted) */

// /* RED LED - RS232 data reception (Single blink per data received) */
// #define LED_RED_BLINK_ON_MS (2000U)  /**< Visible ON time: 200ms */
// #define LED_RED_BLINK_OFF_MS (600U) /**< OFF time between blinks */
// #define LED_RED_ACTIVE_LOW (0U)     /**< Active Low = 0 (inverted) */

// /* YELLOW LED - RS485 indication (Dual behavior) */
// #define LED_YELLOW_POLL_BLINK_ON_MS (100U)  /**< Poll blink ON: 100ms */
// #define LED_YELLOW_POLL_BLINK_OFF_MS (100U) /**< Poll blink OFF: 100ms */
// #define LED_YELLOW_DATA_ON_MS (1000U)       /**< Data solid ON: 1000ms */
// #define LED_YELLOW_ACTIVE_LOW (0U)          /**< Active Low = 0 (inverted) */

// /** @} */

// /*==============================================================================
//  *                          LED CONFIGURATION STRUCTURES
//  *==============================================================================*/

// /**
//  * @brief Blue LED configuration
//  */
// static const LED_Config LED_Blue_Config =
// {
//     .pin = LED_BLUE_PIN,
//     .blink_on_ms = LED_BLUE_ON_MS,
//     .blink_off_ms = LED_BLUE_OFF_MS,
//     .active_low = LED_BLUE_ACTIVE_LOW
// };

// /**
//  * @brief Red LED configuration
//  */
// static const LED_Config LED_Red_Config =
// {
//     .pin = LED_RED_PIN,
//     .blink_on_ms = LED_RED_BLINK_ON_MS,
//     .blink_off_ms = LED_RED_BLINK_OFF_MS,
//     .active_low = LED_RED_ACTIVE_LOW
// };

// /**
//  * @brief Yellow LED configuration
//  */
// static const LED_Config LED_Yellow_Config =
// {
//     .pin = LED_YELLOW_PIN,
//     .blink_on_ms = LED_YELLOW_POLL_BLINK_ON_MS,
//     .blink_off_ms = LED_YELLOW_POLL_BLINK_OFF_MS,
//     .active_low = LED_YELLOW_ACTIVE_LOW
// };

// /*==============================================================================
//  *                          VALIDATION MACROS
//  *==============================================================================*/

// #if (LED_BLUE_PIN > 21U)
// #error "LED_BLUE_PIN must be between 0 and 21 (ESP32-C3 range)"
// #endif

// #if (LED_RED_PIN > 21U)
// #error "LED_RED_PIN must be between 0 and 21 (ESP32-C3 range)"
// #endif

// #if (LED_YELLOW_PIN > 21U)
// #error "LED_YELLOW_PIN must be between 0 and 21 (ESP32-C3 range)"
// #endif

// #if (LED_BLUE_ON_MS < 100U) || (LED_BLUE_ON_MS > 10000U)
// #error "LED_BLUE_ON_MS must be between 100 and 10000 milliseconds"
// #endif

// #if (LED_BLUE_OFF_MS < 100U) || (LED_BLUE_OFF_MS > 10000U)
// #error "LED_BLUE_OFF_MS must be between 100 and 10000 milliseconds"
// #endif

// #endif /* LED_HMI_CFG_H */

/**
 * @file LED_HMI_Cfg.h
 * @brief LED HMI Configuration File
 * @details Configuration for LED hardware and timing specifications
 *
 * LED SPECIFICATIONS:
 * ==================
 * 1. GREEN LED  - Power indicator (hardwired to VCC/GND, no GPIO)
 * 2. BLUE LED   - Module test (GPIO 7): ON 2500ms, OFF 500ms (continuous)
 * 3. RED LED    - RS232 data (GPIO 2): Blink once 200ms when data received
 * 4. YELLOW LED - RS485 dual behavior (GPIO 3):
 *    - Poll mode: Blink continuously 100ms ON / 100ms OFF
 *    - Data mode: Solid ON for 1000ms when valid data received
 * 
 * @author 
 * @date 2025-12-19
 * @version 2.0.0
 */

#ifndef LED_HMI_CFG_H
#define LED_HMI_CFG_H

/*==============================================================================
 *                              INCLUDES
 *==============================================================================*/
#include "LED_HMI.h"

/*==============================================================================
 *                              DEFINES
 *==============================================================================*/

/**
 * @defgroup LED_Pins LED Pin Definitions
 * @brief GPIO pin assignments for ESP32-C3
 * @{
 */
#define LED_BLUE_PIN (7U)   /**< Module test indicator */
#define LED_RED_PIN (6U)    /**< RS232 data indication (Active Low) */
#define LED_YELLOW_PIN (0U) /**< RS485 indication (Active Low) */
/** @} */

/**
 * @defgroup LED_Timing LED Timing Configurations
 * @brief Timing parameters in milliseconds
 * @{
 */

/* BLUE LED - Module Testing (Continuous ON/OFF cycle) */
#define LED_BLUE_ON_MS (2500U)  /**< ON duration: 2500ms */
#define LED_BLUE_OFF_MS (500U)  /**< OFF duration: 500ms */
#define LED_BLUE_ACTIVE_LOW (0U) /**< Active Low = 0 (inverted) */

/* RED LED - RS232 data reception (Single blink per data received) */
#define LED_RED_BLINK_ON_MS (2000U)  /**< Visible ON time: 200ms */
#define LED_RED_BLINK_OFF_MS (600U) /**< OFF time between blinks */
#define LED_RED_ACTIVE_LOW (0U)     /**< Active Low = 0 (inverted) */

/* RED LED - RS232 continuous blink (no data / fault indication) */
#define LED_RED_CONTINUOUS_BLINK_ON_MS  (100U) /**< Continuous blink ON:  100ms */
#define LED_RED_CONTINUOUS_BLINK_OFF_MS (800U) /**< Continuous blink OFF: 100ms */

/* YELLOW LED - RS485 indication (Dual behavior) */
#define LED_YELLOW_POLL_BLINK_ON_MS (100U)  /**< Poll blink ON: 100ms */
#define LED_YELLOW_POLL_BLINK_OFF_MS (100U) /**< Poll blink OFF: 100ms */
#define LED_YELLOW_DATA_ON_MS (1000U)       /**< Data solid ON: 1000ms */
#define LED_YELLOW_ACTIVE_LOW (0U)          /**< Active Low = 0 (inverted) */

/** @} */

/*==============================================================================
 *                          LED CONFIGURATION STRUCTURES
 *==============================================================================*/

/**
 * @brief Blue LED configuration
 */
static const LED_Config LED_Blue_Config =
{
    .pin = LED_BLUE_PIN,
    .blink_on_ms = LED_BLUE_ON_MS,
    .blink_off_ms = LED_BLUE_OFF_MS,
    .active_low = LED_BLUE_ACTIVE_LOW
};

/**
 * @brief Red LED configuration
 */
static const LED_Config LED_Red_Config =
{
    .pin = LED_RED_PIN,
    .blink_on_ms = LED_RED_BLINK_ON_MS,
    .blink_off_ms = LED_RED_BLINK_OFF_MS,
    .active_low = LED_RED_ACTIVE_LOW
};

/**
 * @brief Yellow LED configuration
 */
static const LED_Config LED_Yellow_Config =
{
    .pin = LED_YELLOW_PIN,
    .blink_on_ms = LED_YELLOW_POLL_BLINK_ON_MS,
    .blink_off_ms = LED_YELLOW_POLL_BLINK_OFF_MS,
    .active_low = LED_YELLOW_ACTIVE_LOW
};

/*==============================================================================
 *                          VALIDATION MACROS
 *==============================================================================*/

#if (LED_BLUE_PIN > 21U)
#error "LED_BLUE_PIN must be between 0 and 21 (ESP32-C3 range)"
#endif

#if (LED_RED_PIN > 21U)
#error "LED_RED_PIN must be between 0 and 21 (ESP32-C3 range)"
#endif

#if (LED_YELLOW_PIN > 21U)
#error "LED_YELLOW_PIN must be between 0 and 21 (ESP32-C3 range)"
#endif

#if (LED_BLUE_ON_MS < 100U) || (LED_BLUE_ON_MS > 10000U)
#error "LED_BLUE_ON_MS must be between 100 and 10000 milliseconds"
#endif

#if (LED_BLUE_OFF_MS < 100U) || (LED_BLUE_OFF_MS > 10000U)
#error "LED_BLUE_OFF_MS must be between 100 and 10000 milliseconds"
#endif

#endif /* LED_HMI_CFG_H */