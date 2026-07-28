
/**
 * @file LED_HMI.cpp
 * @brief LED HMI Implementation
 * @details LED management system with state machine control
 *          BLUE LED: Manual state machine for 2500ms ON / 500ms OFF
 *          YELLOW LED: Single blink per poll, solid 1000ms for data
 * 
 * @author 
 * @date 2025-12-19
 * @version 2.0.0
 */

/*==============================================================================
 *                              INCLUDES
 *============================================================================*/
#include "Arduino.h"
#include "LED_HMI.h"
#include "LED_HMI_cfg.h"

/*==============================================================================
 *                              PRIVATE TYPES
 *============================================================================*/

/**
 * @brief Blue LED state machine states
 */
typedef enum
{
    BLUE_STATE_OFF = 0,
    BLUE_STATE_ON = 1
} BlueLed_StateType;

/**
 * @brief Red LED state machine states
 */
typedef enum
{
    RED_STATE_BLINK_ON  = 0,
    RED_STATE_BLINK_OFF = 1
} RedLed_StateType;

/*==============================================================================
 *                          PRIVATE VARIABLES
 *============================================================================*/

/* LED instances */
static LED_Instance LED_Blue;
static LED_Instance LED_Red;
static LED_Instance LED_Yellow;

/* Blue LED state machine */
static BlueLed_StateType LED_BlueState = BLUE_STATE_OFF;
static uint32_t LED_BlueStateStartTime = 0U;

/* Red LED state machine */
static RedLed_StateType LED_RedState = RED_STATE_BLINK_ON;
static uint32_t LED_RedStateStartTime = 0U;
static uint8_t LED_RedBlinkRunning = 0U; /* 0 = SM needs (re)start, 1 = SM running */

/*==============================================================================
 *                          PRIVATE FUNCTION PROTOTYPES
 *============================================================================*/
static void LED_SetPin(uint8_t pin, uint8_t state);

/*==============================================================================
 *                          PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize LED instance
 * @details Configures LED pin and initial state
 * @param led Pointer to LED instance
 * @param config Pointer to LED configuration
 * @return LED_Status LED_STATUS_OK if successful, LED_STATUS_ERROR otherwise
 */
LED_Status LED_Init(LED_Instance *led, const LED_Config *config)
{
    if ((led == NULL) || (config == NULL))
    {
        return LED_STATUS_ERROR;
    }

    led->config = *config;
    led->state = LED_OFF;
    led->last_toggle_time = millis();
    led->timed_on_start = 0U;
    led->timed_on_duration = 0U;
    led->blink_count = 0U;
    led->is_on = 0U;

    pinMode(config->pin, OUTPUT);
    LED_Off(led);

    return LED_STATUS_OK;
}

/**
 * @brief Turn LED on
 * @details Sets LED to continuous ON state
 * @param led Pointer to LED instance
 */
void LED_On(LED_Instance *led)
{
    if (led == NULL)
    {
        return;
    }

    led->state = LED_ON;
    led->blink_count = 0U;
    led->timed_on_duration = 0U;

    if (led->config.active_low == 1U)
    {
        led->is_on = 0U; /* LOW = ON for active-low */
    }
    else
    {
        led->is_on = 1U; /* HIGH = ON */
    }

    LED_SetPin(led->config.pin, led->is_on);
}

/**
 * @brief Turn LED off
 * @details Sets LED to OFF state
 * @param led Pointer to LED instance
 */
void LED_Off(LED_Instance *led)
{
    if (led == NULL)
    {
        return;
    }

    led->state = LED_OFF;
    led->blink_count = 0U;
    led->timed_on_duration = 0U;

    if (led->config.active_low == 1U)
    {
        led->is_on = 1U; /* HIGH = OFF for active-low */
    }
    else
    {
        led->is_on = 0U; /* LOW = OFF */
    }

    LED_SetPin(led->config.pin, led->is_on);
}

/**
 * @brief Start LED blinking
 * @details Initiates blink sequence with specified count
 * @param led Pointer to LED instance
 * @param count Number of blinks
 */
void LED_Blink(LED_Instance *led, uint8_t count)
{
    if ((led == NULL) || (count == 0U))
    {
        return;
    }

    /* Don't restart if already blinking */
    if ((led->state == LED_BLINK) && (led->blink_count > 0U))
    {
        return;
    }

    led->state = LED_BLINK;
    led->blink_count = count * 2U; /* ON + OFF phases */
    led->last_toggle_time = millis();

    /* Start with ON */
    if (led->config.active_low == 1U)
    {
        led->is_on = 0U;
    }
    else
    {
        led->is_on = 1U;
    }

    LED_SetPin(led->config.pin, led->is_on);

    Serial.printf("[LED] Blink started: pin=%d count=%d\n", 
                  (int)led->config.pin, (int)count);
}

/**
 * @brief Turn LED on for specified duration
 * @details Sets LED to timed ON state
 * @param led Pointer to LED instance
 * @param durationMs Duration in milliseconds
 */
void LED_TimedOn(LED_Instance *led, uint16_t durationMs)
{
    if ((led == NULL) || (durationMs == 0U))
    {
        return;
    }

    led->state = LED_TIMED_ON;
    led->timed_on_start = millis();
    led->timed_on_duration = durationMs;
    led->blink_count = 0U;

    if (led->config.active_low == 1U)
    {
        led->is_on = 0U;
    }
    else
    {
        led->is_on = 1U;
    }

    LED_SetPin(led->config.pin, led->is_on);

    Serial.printf("[LED] TimedOn: pin=%d duration=%dms\n", 
                  (int)led->config.pin, (int)durationMs);
}

/**
 * @brief Update LED state
 * @details Handles blink and timed operations
 * @param led Pointer to LED instance
 * @param currentTimeMs Current time in milliseconds
 */
void LED_Update(LED_Instance *led, uint32_t currentTimeMs)
{
    uint32_t elapsed = 0U;
    uint16_t toggleInterval = 0U;

    if (led == NULL)
    {
        return;
    }

    if ((led->state == LED_OFF) || (led->state == LED_ON))
    {
        return;
    }

    /* Handle TIMED_ON */
    if (led->state == LED_TIMED_ON)
    {
        elapsed = currentTimeMs - led->timed_on_start;

        if (elapsed >= led->timed_on_duration)
        {
            LED_Off(led);
            Serial.printf("[LED] TimedOn expired: pin=%d\n", (int)led->config.pin);
        }
        return;
    }

    /* Handle BLINK */
    if ((led->state == LED_BLINK) && (led->blink_count > 0U))
    {
        elapsed = currentTimeMs - led->last_toggle_time;

        if (led->is_on == 1U)
        {
            toggleInterval = led->config.blink_on_ms;
        }
        else
        {
            toggleInterval = led->config.blink_off_ms;
        }

        if (elapsed >= toggleInterval)
        {
            led->last_toggle_time = currentTimeMs;
            led->blink_count--;

            led->is_on = (led->is_on == 1U) ? 0U : 1U;
            LED_SetPin(led->config.pin, led->is_on);

            if (led->blink_count == 0U)
            {
                LED_Off(led);
                Serial.printf("[LED] Blink complete: pin=%d\n", (int)led->config.pin);
            }
        }
    }
}

/**
 * @brief Initialize LED HMI module
 * @details Initializes all LED instances and starts blue LED state machine
 */
void LED_HMI_Init(void)
{
    Serial.println("\n[LED_HMI] ========== INITIALIZING ==========");
    Serial.println("[LED_HMI] GREEN LED - Hardwired (Power)");

    if (LED_Init(&LED_Blue, &LED_Blue_Config) == LED_STATUS_OK)
    {
        Serial.printf("[LED_HMI] BLUE LED - Pin %d (2500ms ON / 500ms OFF)\n", 
                      (int)LED_BLUE_PIN);
    }

    if (LED_Init(&LED_Red, &LED_Red_Config) == LED_STATUS_OK)
    {
        Serial.printf("[LED_HMI] RED LED - Pin %d (200ms blink)\n", 
                      (int)LED_RED_PIN);
    }

    if (LED_Init(&LED_Yellow, &LED_Yellow_Config) == LED_STATUS_OK)
    {
        Serial.printf("[LED_HMI] YELLOW LED - Pin %d (100ms blink / 1000ms data)\n", 
                      (int)LED_YELLOW_PIN);
    }

    /* Start blue LED in ON state */
    LED_BlueState = BLUE_STATE_ON;
    LED_BlueStateStartTime = millis();
    LED_On(&LED_Blue);

    /* Start Red LED continuous blink from boot */
    LED_RedState = RED_STATE_BLINK_ON;
    LED_RedStateStartTime = millis();
    LED_SetPin(LED_RED_PIN, 1U);

    Serial.println("[LED_HMI] ========== INIT COMPLETE ==========\n");
}

/**
 * @brief LED HMI periodic handler
 * @details Updates all LED states and manages blue LED state machine
 *          Should be called every 50ms
 */
void LED_HMI_Handler(void)
{
    static uint32_t lastDebug = 0U;
    uint32_t currentTime = 0U;
    uint32_t elapsed = 0U;

    currentTime = millis();

    /* Debug every 3 seconds */
    if ((currentTime - lastDebug) > 3000U)
    {
        Serial.printf("[LED_HMI] Blue_state=%d Yellow_state=%d elapsed=%lu\n",
                      (int)LED_BlueState, (int)LED_Yellow.state,
                      (unsigned long)(currentTime - LED_BlueStateStartTime));
        lastDebug = currentTime;
    }

    /* Update all LED instances */
    LED_Update(&LED_Blue, currentTime);
    LED_Update(&LED_Red, currentTime);
    LED_Update(&LED_Yellow, currentTime);

    /* Blue LED state machine (manual control) */
    elapsed = currentTime - LED_BlueStateStartTime;

    switch (LED_BlueState)
    {
        case BLUE_STATE_ON:
        {
            /* Check if ON period (2500ms) is complete */
            if (elapsed >= LED_BLUE_ON_MS)
            {
                LED_BlueState = BLUE_STATE_OFF;
                LED_BlueStateStartTime = currentTime;
                LED_Off(&LED_Blue);
                Serial.println("[LED_HMI] Blue: ON->OFF");
            }
            break;
        }

        case BLUE_STATE_OFF:
        {
            /* Check if OFF period (500ms) is complete */
            if (elapsed >= LED_BLUE_OFF_MS)
            {
                LED_BlueState = BLUE_STATE_ON;
                LED_BlueStateStartTime = currentTime;
                LED_On(&LED_Blue);
                Serial.println("[LED_HMI] Blue: OFF->ON");
            }
            break;
        }

        default:
        {
            break;
        }
    }

    /* Red LED state machine - RS232 continuous blink */
    /* Skipped when LED_TimedOn() is active (data received = solid 1sec)  */
    if (LED_Red.state != LED_TIMED_ON)
    {
        /* After timed-on expires, LED_Update sets state to LED_OFF.
         * Restart continuous blink ONCE using a flag to avoid
         * resetting the timer every frame while state stays LED_OFF.     */
        if ((LED_Red.state == LED_OFF) && (LED_RedBlinkRunning == 0U))
        {
            LED_RedBlinkRunning = 1U;
            LED_RedState = RED_STATE_BLINK_ON;
            LED_RedStateStartTime = currentTime;
            LED_SetPin(LED_RED_PIN, 1U);
        }

        elapsed = currentTime - LED_RedStateStartTime;

        switch (LED_RedState)
        {
            case RED_STATE_BLINK_ON:
            {
                if (elapsed >= LED_RED_CONTINUOUS_BLINK_ON_MS)
                {
                    LED_RedState = RED_STATE_BLINK_OFF;
                    LED_RedStateStartTime = currentTime;
                    LED_SetPin(LED_RED_PIN, 0U);
                }
                break;
            }

            case RED_STATE_BLINK_OFF:
            {
                if (elapsed >= LED_RED_CONTINUOUS_BLINK_OFF_MS)
                {
                    LED_RedState = RED_STATE_BLINK_ON;
                    LED_RedStateStartTime = currentTime;
                    LED_SetPin(LED_RED_PIN, 1U);
                }
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

/**
 * @brief Blue LED module test (not used)
 * @details Blue LED is controlled by state machine in handler
 */
void LED_Blue_ModuleTest(void)
{
    /* Not used - Blue LED is controlled by state machine in handler */
}

/**
 * @brief Indicate RS232 data received
 * @details Blinks red LED once
 */
void LED_Red_RS232_Received(void)
{
    /* Clear flag so continuous blink restarts after solid-ON expires */
    LED_RedBlinkRunning = 0U;
    LED_TimedOn(&LED_Red, 1000U);
    Serial.println("[APP] Red LED: RS232 data received - solid 1000ms");
}

/**
 * @brief Indicate PLC poll
 * @details Blinks yellow LED once if not showing data
 */
void LED_Yellow_PLC_Poll(void)
{
    /* Only blink if not currently showing data (TIMED_ON) */
    if (LED_Yellow.state != LED_TIMED_ON)
    {
        LED_Blink(&LED_Yellow, 1U);
    	Serial.println("[APP] Yellow LED: PLC poll blink");
    }
}

/**
 * @brief Indicate RS485 data received
 * @details Turns yellow LED solid for 1000ms
 */
void LED_Yellow_RS485_DataReceived(void)
{
    LED_TimedOn(&LED_Yellow, LED_YELLOW_DATA_ON_MS);
    Serial.println("[APP] Yellow LED: RS485 data solid ON");
}

/*==============================================================================
 *                          PRIVATE FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Set LED pin state
 * @details Hardware-specific pin control
 * @param pin GPIO pin number
 * @param state Pin state (0=LOW, 1=HIGH)
 */
static void LED_SetPin(uint8_t pin, uint8_t state)
{
    digitalWrite(pin, state);
}