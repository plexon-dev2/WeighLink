/**
 * @file LED_HMI.h
 * @brief LED HMI Interface
 * @details LED management system with state machine support
 * 
 * @author 
 * @date 2025-12-19
 * @version 2.0.0
 */

#ifndef LED_HMI_H
#define LED_HMI_H

/*==============================================================================
 *                              INCLUDES
 *==============================================================================*/
#include <stdint.h>

/*==============================================================================
 *                              PUBLIC TYPES
 *==============================================================================*/

/**
 * @brief LED state enumeration
 */
typedef enum 
{
    LED_OFF = 0,
    LED_ON = 1,
    LED_BLINK = 2,
    LED_TIMED_ON = 3
} LED_State;

/**
 * @brief LED status enumeration
 */
typedef enum 
{
    LED_STATUS_OK = 0,
    LED_STATUS_ERROR = -1
} LED_Status;

/**
 * @brief LED configuration structure
 */
typedef struct 
{
    uint8_t pin;
    uint16_t blink_on_ms;
    uint16_t blink_off_ms;
    uint8_t active_low;
} LED_Config;

/**
 * @brief LED instance structure
 */
typedef struct 
{
    LED_Config config;
    LED_State state;
    uint32_t last_toggle_time;
    uint32_t timed_on_start;
    uint16_t timed_on_duration;
    uint8_t blink_count;
    uint8_t is_on;
} LED_Instance;

/*==============================================================================
 *                          PUBLIC FUNCTION DECLARATIONS
 *==============================================================================*/

/**
 * @brief Initialize LED instance
 * @param led Pointer to LED instance
 * @param config Pointer to LED configuration
 * @return LED_Status
 */
LED_Status LED_Init(LED_Instance *led, const LED_Config *config);

/**
 * @brief Turn LED on
 * @param led Pointer to LED instance
 */
void LED_On(LED_Instance *led);

/**
 * @brief Turn LED off
 * @param led Pointer to LED instance
 */
void LED_Off(LED_Instance *led);

/**
 * @brief Start LED blinking
 * @param led Pointer to LED instance
 * @param count Number of blinks
 */
void LED_Blink(LED_Instance *led, uint8_t count);

/**
 * @brief Turn LED on for specified duration
 * @param led Pointer to LED instance
 * @param durationMs Duration in milliseconds
 */
void LED_TimedOn(LED_Instance *led, uint16_t durationMs);

/**
 * @brief Update LED state
 * @param led Pointer to LED instance
 * @param currentTimeMs Current time in milliseconds
 */
void LED_Update(LED_Instance *led, uint32_t currentTimeMs);

/**
 * @brief Initialize LED HMI module
 */
void LED_HMI_Init(void);

/**
 * @brief LED HMI periodic handler
 */
void LED_HMI_Handler(void);

/**
 * @brief Blue LED module test (not used)
 */
void LED_Blue_ModuleTest(void);

/**
 * @brief Indicate RS232 data received
 */
void LED_Red_RS232_Received(void);

/**
 * @brief Indicate PLC poll
 */
void LED_Yellow_PLC_Poll(void);

/**
 * @brief Indicate RS485 data received
 */
void LED_Yellow_RS485_DataReceived(void);

#endif /* LED_HMI_H */