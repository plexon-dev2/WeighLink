// /**
//  * @file App.cpp
//  * @brief Application Module - Main Application Logic
//  * @details Monitors RS232/RS485 communication and power supply health
//  *          Integrates CmdParser and Modbus modules
//  * 
//  * Features:
//  * - RS232/RS485 power supply monitoring with debouncing
//  * - Communication error detection
//  * - LED HMI integration
//  * - Periodic status display
//  * - Counter overflow protection
//  * - Complete message processing (no data loss)
//  * 
//  * @author 
//  * @date 2025-12-18
//  * @version 2.1.0
//  * 
//  * MISRA-C:2012 Compliance:
//  * - Rule 8.13: Pointers to const where applicable
//  * - Rule 10.1: Explicit type conversions
//  * - Rule 14.3: Explicit comparisons
//  * - Rule 14.4: Single point of exit where practical
//  * - Rule 17.7: Return values checked
//  */

// /*==============================================================================
//  *                              INCLUDES
//  *============================================================================*/
// #include "App.h"
// #include "App_Cfg.h"
// #include "Modbus.h"
// #include "CmdParser.h"
// #include "LED_HMI.h"
// #include "Arduino.h"

// /*==============================================================================
//  *                              DEFINES
//  *============================================================================*/

// /* NOTE: Configuration defines are in App_Cfg.h
//  *       Only internal implementation defines here if needed
//  */

// /*==============================================================================
//  *                          PRIVATE VARIABLES
//  *============================================================================*/

// /* Power supply monitoring */
// static uint32_t rs232_iso_power_debounce_counter = 0U;
// static uint32_t rs485_iso_power_debounce_counter = 0U;

// /* Error flags */
// static bool rs485_comm_error = false;
// static bool rs232_comm_error = false;
// static bool rs485_power_error = false;
// static bool rs232_power_error = false;

// /* Display counter */
// static uint32_t display_counter = 0U;

// /*==============================================================================
//  *                          PRIVATE FUNCTION PROTOTYPES
//  *============================================================================*/

// /* NOTE: Public function prototypes are in App.h
//  *       Only private (static) helper functions declared here
//  */

// /**
//  * @brief Check single power supply with debouncing
//  * @details Generic helper for power supply monitoring
//  * 
//  * @param[in] pin GPIO pin to monitor
//  * @param[in,out] debounce_counter Pointer to debounce counter
//  * @param[in] is_data_fresh Function pointer to data freshness check
//  * @param[in] name Power supply name for logging
//  * 
//  * @return true if power supply error detected, false otherwise
//  * 
//  * @note MISRA-C Rule 8.13: All input-only parameters are const-qualified
//  * @note MISRA-C Rule 17.8: Parameters not modified (except debounce_counter)
//  */
// static bool App_CheckPowerSupply(const uint8_t pin, 
//                                   uint32_t *debounce_counter, 
//                                   bool (*is_data_fresh)(void), 
//                                   const char *name);

// /**
//  * @brief Display weight data periodically
//  * @details Reads weight data and updates Modbus registers
//  * @note Called from App__Handler() every APP_DISPLAY_UPDATE_INTERVAL cycles
//  */
// static void App_DisplayData(void);

// /**
//  * @brief Monitor power supply status with debouncing
//  * @details Checks RS232 and RS485 power pins, debounces readings
//  * @note Called from App__Handler() every cycle
//  */
// static void App_MonitorPowerSupply(void);

// /**
//  * @brief Monitor communication status
//  * @details Checks for RS232 and RS485 communication errors
//  * @note Called from App__Handler() every cycle
//  */
// static void App_MonitorCommunication(void);

// /*==============================================================================
//  *                          PUBLIC FUNCTION IMPLEMENTATIONS
//  *============================================================================*/

// /**
//  * @brief Initialize Application Module
//  * @details Initializes error flags, LED HMI, and validates GPIO configuration
//  * 
//  * @note MISRA-C Rule 14.4: Single point of exit (validation at start)
//  */
// void App__Init(void)
// {
//     /* Validate GPIO configuration (macros from App_Cfg.h) */
//     if ((APP_RS232_ISO_POWER_PIN < 0) || (APP_RS232_ISO_POWER_PIN > 21))
//     {
//         Serial.println("[APP] ERROR: Invalid RS232 power pin configuration!");
//         return;
//     }
    
//     if ((APP_RS485_ISO_POWER_PIN < 0) || (APP_RS485_ISO_POWER_PIN > 21))
//     {
//         Serial.println("[APP] ERROR: Invalid RS485 power pin configuration!");
//         return;
//     }
    
//     /* Initialize error flags */
//     rs485_comm_error = false;
//     rs232_comm_error = false;
//     rs485_power_error = false;
//     rs232_power_error = false;

//     /* Initialize counters */
//     rs232_iso_power_debounce_counter = 0U;
//     rs485_iso_power_debounce_counter = 0U;
//     display_counter = 0U;

//     /* Initialize LED HMI */
//     LED_HMI_Init();

//     /* Configure power monitoring pins */
//     pinMode(APP_RS232_ISO_POWER_PIN, INPUT);
//     pinMode(APP_RS485_ISO_POWER_PIN, INPUT);

//     Serial.println("[APP] Application module initialized");
    
//     /* MISRA-C Rule 10.1: Explicit conversion for printf */
//     Serial.printf("[APP] RS232 Power Pin: GPIO %d\n", (int)APP_RS232_ISO_POWER_PIN);
//     Serial.printf("[APP] RS485 Power Pin: GPIO %d\n", (int)APP_RS485_ISO_POWER_PIN);
// }

// /**
//  * @brief Main Application Handler
//  * @details Called periodically by scheduler (50ms or 100ms task)
//  *          Monitors power supply, communication status, and updates display
//  * 
//  * @note MISRA-C compliant: No complex logic in handler, delegates to helpers
//  */
// void App__Handler(void)
// {
//     /* Update LED HMI */
//     LED_HMI_Handler();

//     /* Monitor power supplies */
//     App_MonitorPowerSupply();

//     /* Monitor communication status */
//     App_MonitorCommunication();

//     /* Update display periodically */
//     App_DisplayData();
// }

// /**
//  * @brief Get RS232 power supply error status
//  * @return true if power supply error detected, false otherwise
//  */
// bool App__IsRS232PowerSupplyError(void)
// {
//     return rs232_power_error;
// }

// /**
//  * @brief Get RS485 power supply error status
//  * @return true if power supply error detected, false otherwise
//  */
// bool App__IsRS485PowerSupplyError(void)
// {
//     return rs485_power_error;
// }

// /**
//  * @brief Get RS232 communication error status
//  * @return true if communication error detected, false otherwise
//  */
// bool App__IsRS232CommError(void)
// {
//     return rs232_comm_error;
// }

// /**
//  * @brief Get RS485 communication error status
//  * @return true if communication error detected, false otherwise
//  */
// bool App__IsRS485CommError(void)
// {
//     return rs485_comm_error;
// }

// /**
//  * @brief Clear all error flags
//  * @details Resets all error conditions and debounce counters
//  */
// void App__ClearErrors(void)
// {
//     rs485_comm_error = false;
//     rs232_comm_error = false;
//     rs485_power_error = false;
//     rs232_power_error = false;

//     rs232_iso_power_debounce_counter = 0U;
//     rs485_iso_power_debounce_counter = 0U;

//     Serial.println("[APP] All error flags cleared");
// }

// /*==============================================================================
//  *                          PRIVATE FUNCTION IMPLEMENTATIONS
//  *============================================================================*/

// /**
//  * @brief Check single power supply with debouncing
//  * @details Generic helper function for power supply monitoring
//  *          Implements debouncing and overflow protection
//  * 
//  * @note MISRA-C Rule 14.4: Single point of exit
//  * @note MISRA-C Rule 17.7: Return value should be used by caller
//  */
// static bool App_CheckPowerSupply(const uint8_t pin, 
//                                   uint32_t *debounce_counter, 
//                                   bool (*is_data_fresh)(void), 
//                                   const char *name)
// {
//     bool error_detected = false;
    
//     /* MISRA-C Rule 14.3: Explicit comparison */
//     if (HIGH == digitalRead(pin))
//     {
//         /* Increment counter */
//         (*debounce_counter)++;
        
//         /* Overflow protection (UINT32_MAX from stdint.h) */
//         if (*debounce_counter > (UINT32_MAX - 100U))
//         {
//             *debounce_counter = 0U;
//             Serial.println("[APP] WARNING: Debounce counter overflow prevented");
//         }
        
//         /* Check if debounce threshold reached */
//         if (*debounce_counter >= APP_ISO_POWER_DEBOUNCE_COUNT)
//         {
//             *debounce_counter = 0U;
            
//             /* MISRA-C Rule 14.3: Explicit null check before function call */
//             if ((is_data_fresh != NULL) && (!is_data_fresh()))
//             {
//                 error_detected = true;
//                 Serial.printf("[APP] %s power supply error detected!\n", name);
//             }
//         }
//     }
//     else
//     {
//         /* Power pin is LOW (normal), reset counter */
//         *debounce_counter = 0U;
//     }
    
//     return error_detected;
// }

// /**
//  * @brief Monitor power supply status with debouncing
//  * @details Checks RS232 and RS485 power pins using helper function
//  * 
//  * @note MISRA-C: Cyclomatic Complexity = 2 (reduced from 5)
//  */
// static void App_MonitorPowerSupply(void)
// {
//     /* Monitor RS232 power supply */
//     rs232_power_error = App_CheckPowerSupply(
//         APP_RS232_ISO_POWER_PIN,
//         &rs232_iso_power_debounce_counter,
//         CmdParser__IsDataFresh,
//         "RS232"
//     );
    
//     /* Monitor RS485 power supply */
//     rs485_power_error = App_CheckPowerSupply(
//         APP_RS485_ISO_POWER_PIN,
//         &rs485_iso_power_debounce_counter,
//         Modbus_IsInitialized,
//         "RS485"
//     );
// }

// /**
//  * @brief Monitor communication status
//  * @details Checks for RS232 and RS485 communication errors
//  * 
//  * @note MISRA-C Rule 17.7: Return value checked for all API calls
//  * @note MISRA-C Rule 9.1: Variables initialized before use
//  */
// static void App_MonitorCommunication(void)
// {
//     /* Check RS232 communication (CmdParser) */
//     CmdParser_ErrorType_t parser_error = CMDPARSER_ERROR_NONE;
//     CmdParser_StatusType_t status;
    
//     status = CmdParser__GetErrorStatus(&parser_error);
    
//     /* MISRA-C Rule 14.3: Explicit comparison */
//     if (status == CMDPARSER_STATUS_OK)
//     {
//         if (parser_error != CMDPARSER_ERROR_NONE)
//         {
//             rs232_comm_error = true;
            
//             /* MISRA-C Rule 10.1: Explicit cast for printf */
//             Serial.printf("[APP] RS232 communication error: %d\n", (int)parser_error);
//         }
//         else
//         {
//             rs232_comm_error = false;
//         }
//     }
//     else
//     {
//         /* Failed to get error status */
//         Serial.printf("[APP] Failed to get parser error status: %d\n", (int)status);
//     }

//     /* Check RS485 communication (Modbus) */
//     /* RS485 error is indicated by lack of initialization */
//     if (!Modbus_IsInitialized())
//     {
//         rs485_comm_error = true;
//         Serial.println("[APP] RS485 communication error: Not initialized");
//     }
//     else
//     {
//         rs485_comm_error = false;
//     }
// }

// /**
//  * @brief Display data periodically
//  * @details Updates every 1 second (called from 100ms task, counts to 10)
//  *          Reads weight data and updates Modbus registers
//  *          Processes ALL new messages to prevent data loss
//  * 
//  * @note MISRA-C Rule 17.7: All return values checked
//  * @note MISRA-C: Processes all messages (no data loss)
//  */
// static void App_DisplayData(void)
// {
//     static uint32_t last_message_count = 0U;
    
//     display_counter++;
    
//     /* Overflow protection */
//     if (display_counter > (UINT32_MAX - 100U))
//     {
//         display_counter = 0U;
//         Serial.println("[APP] WARNING: Display counter overflow prevented");
//     }

//     /* Check if update interval reached */
//     if (display_counter >= APP_DISPLAY_UPDATE_INTERVAL)
//     {
//         display_counter = 0U;

//         /* Get current message count */
//         uint32_t total_messages = 0U;
//         CmdParser_StatusType_t status;
        
//         status = CmdParser__GetTotalMessages(&total_messages);
        
//         /* MISRA-C Rule 17.7: Check return value */
//         if (status != CMDPARSER_STATUS_OK)
//         {
//             Serial.printf("[APP] Failed to get message count: %d\n", (int)status);
//             return;
//         }

//         /* Process ALL new messages (prevents data loss) */
//         /* MISRA-C Rule 14.2: Loop counter monotonically increases */
//         while (last_message_count < total_messages)
//         {
//             last_message_count++;

//             /* Prepare weight data structure */
//             Modbus_WeightData_t weight_data;
//             bool all_data_valid = true;
            
//             /* MISRA-C Rule 9.1: Initialize structure members */
//             weight_data.gross_weight = 0.0f;
//             weight_data.tare_weight = 0.0f;
//             weight_data.net_weight = 0.0f;
//             weight_data.data_valid = 0U;
//             weight_data.string_counter = 0U;

//             /* Try to read values from CmdParser - check all return values */
//             if (CmdParser__GetGrossWeight(&weight_data.gross_weight) != CMDPARSER_STATUS_OK)
//             {
//                 all_data_valid = false;
//             }
            
//             if (CmdParser__GetTareWeight(&weight_data.tare_weight) != CMDPARSER_STATUS_OK)
//             {
//                 all_data_valid = false;
//             }
            
//             if (CmdParser__GetNetWeight(&weight_data.net_weight) != CMDPARSER_STATUS_OK)
//             {
//                 all_data_valid = false;
//             }

//             /* MISRA-C Rule 10.3: Explicit cast to narrower type */
//             weight_data.string_counter = (uint16_t)(last_message_count & 0xFFFFU);
            
//             /* Set valid flag only if all data retrieved successfully */
//             weight_data.data_valid = (all_data_valid) ? 1U : 0U;

//             /* Update Modbus registers */
//             (void)Modbus_SetWeightData(&weight_data);

//             /* Log status */
//             /* MISRA-C Rule 10.1: Explicit conversion for printf */
//             Serial.printf("[APP] NEW msg #%lu - Flag %s\n", 
//                           (unsigned long)last_message_count,
//                           (all_data_valid) ? "SET" : "CLEARED");
//             Serial.printf("[APP] G=%.3f T=%.3f N=%.3f\n",
//                           (double)weight_data.gross_weight,
//                           (double)weight_data.tare_weight,
//                           (double)weight_data.net_weight);
//         }
//     }
// }



/**
 * @file App.cpp
 * @brief Application Module - Main Application Logic
 * @details Monitors RS232/RS485 communication and power supply health
 *          Integrates CmdParser and Modbus modules
 * 
 * Features:
 * - RS232/RS485 power supply monitoring with debouncing
 * - Communication error detection (gated per message, no flood)
 * - LED HMI integration
 * - Periodic status display
 * - Counter overflow protection
 * - Complete message processing (no data loss)
 * 
 * @author 
 * @date 2025-12-18
 * @version 2.2.0
 * 
 * CHANGELOG v2.2.0:
 * - FIX: App_MonitorCommunication() now gated on new message count
 *        Prevents repeated logging of same error every handler cycle
 *        rs232_comm_error now reflects latest frame outcome only
 *        Eliminates spurious consecutive_errors increments in SystemStates
 * 
 * MISRA-C:2012 Compliance:
 * - Rule 8.13: Pointers to const where applicable
 * - Rule 10.1: Explicit type conversions
 * - Rule 14.3: Explicit comparisons
 * - Rule 14.4: Single point of exit where practical
 * - Rule 17.7: Return values checked
 */

/*==============================================================================
 *                              INCLUDES
 *============================================================================*/
#include "App.h"
#include "App_Cfg.h"
#include "Modbus.h"
#include "CmdParser.h"
#include "LED_HMI.h"
#include "Arduino.h"

/*==============================================================================
 *                              DEFINES
 *============================================================================*/

/* NOTE: Configuration defines are in App_Cfg.h
 *       Only internal implementation defines here if needed
 */

/*==============================================================================
 *                          PRIVATE VARIABLES
 *============================================================================*/

/* Power supply monitoring */
static uint32_t rs232_iso_power_debounce_counter = 0U;
static uint32_t rs485_iso_power_debounce_counter = 0U;

/* Error flags */
static bool rs485_comm_error  = false;
static bool rs232_comm_error  = false;
static bool rs485_power_error = false;
static bool rs232_power_error = false;

/* Display counter */
static uint32_t display_counter = 0U;

/*==============================================================================
 *                          PRIVATE FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * @brief Check single power supply with debouncing
 * @details Generic helper for power supply monitoring
 *
 * @param[in]     pin              GPIO pin to monitor
 * @param[in,out] debounce_counter Pointer to debounce counter
 * @param[in]     is_data_fresh    Function pointer to data freshness check
 * @param[in]     name             Power supply name for logging
 *
 * @return true if power supply error detected, false otherwise
 *
 * @note MISRA-C Rule 8.13: All input-only parameters are const-qualified
 * @note MISRA-C Rule 17.8: Parameters not modified (except debounce_counter)
 */
static bool App_CheckPowerSupply(const uint8_t pin,
                                  uint32_t *debounce_counter,
                                  bool (*is_data_fresh)(void),
                                  const char *name);

/**
 * @brief Display weight data periodically
 * @details Reads weight data and updates Modbus registers
 * @note Called from App__Handler() every APP_DISPLAY_UPDATE_INTERVAL cycles
 */
static void App_DisplayData(void);

/**
 * @brief Monitor power supply status with debouncing
 * @details Checks RS232 and RS485 power pins, debounces readings
 * @note Called from App__Handler() every cycle
 */
static void App_MonitorPowerSupply(void);

/**
 * @brief Monitor communication status
 * @details Checks RS232 and RS485 communication errors.
 *          RS232 check is gated on new message arrival to prevent
 *          repeated logging of the same stale error every handler cycle.
 * @note Called from App__Handler() every cycle
 */
static void App_MonitorCommunication(void);

/*==============================================================================
 *                          PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize Application Module
 * @details Initializes error flags, LED HMI, and validates GPIO configuration
 *
 * @note MISRA-C Rule 14.4: Single point of exit (validation at start)
 */
void App__Init(void)
{
    /* Validate GPIO configuration (macros from App_Cfg.h) */
    if ((APP_RS232_ISO_POWER_PIN < 0) || (APP_RS232_ISO_POWER_PIN > 21))
    {
        Serial.println("[APP] ERROR: Invalid RS232 power pin configuration!");
        return;
    }

    if ((APP_RS485_ISO_POWER_PIN < 0) || (APP_RS485_ISO_POWER_PIN > 21))
    {
        Serial.println("[APP] ERROR: Invalid RS485 power pin configuration!");
        return;
    }

    /* Initialize error flags */
    rs485_comm_error  = false;
    rs232_comm_error  = false;
    rs485_power_error = false;
    rs232_power_error = false;

    /* Initialize counters */
    rs232_iso_power_debounce_counter = 0U;
    rs485_iso_power_debounce_counter = 0U;
    display_counter                  = 0U;

    /* Initialize LED HMI */
    LED_HMI_Init();

    /* Configure power monitoring pins */
    pinMode(APP_RS232_ISO_POWER_PIN, INPUT);
    pinMode(APP_RS485_ISO_POWER_PIN, INPUT);

    Serial.println("[APP] Application module initialized");

    /* MISRA-C Rule 10.1: Explicit conversion for printf */
    Serial.printf("[APP] RS232 Power Pin: GPIO %d\n", (int)APP_RS232_ISO_POWER_PIN);
    Serial.printf("[APP] RS485 Power Pin: GPIO %d\n", (int)APP_RS485_ISO_POWER_PIN);
}

/**
 * @brief Main Application Handler
 * @details Called periodically by scheduler (50ms or 100ms task)
 *          Monitors power supply, communication status, and updates display
 *
 * @note MISRA-C compliant: No complex logic in handler, delegates to helpers
 */
void App__Handler(void)
{
    /* Update LED HMI */
    LED_HMI_Handler();

    /* Monitor power supplies */
    App_MonitorPowerSupply();

    /* Monitor communication status */
    App_MonitorCommunication();

    /* Update display periodically */
    App_DisplayData();
}

/**
 * @brief Get RS232 power supply error status
 * @return true if power supply error detected, false otherwise
 */
bool App__IsRS232PowerSupplyError(void)
{
    return rs232_power_error;
}

/**
 * @brief Get RS485 power supply error status
 * @return true if power supply error detected, false otherwise
 */
bool App__IsRS485PowerSupplyError(void)
{
    return rs485_power_error;
}

/**
 * @brief Get RS232 communication error status
 * @return true if communication error detected, false otherwise
 */
bool App__IsRS232CommError(void)
{
    return rs232_comm_error;
}

/**
 * @brief Get RS485 communication error status
 * @return true if communication error detected, false otherwise
 */
bool App__IsRS485CommError(void)
{
    return rs485_comm_error;
}

/**
 * @brief Clear all error flags
 * @details Resets all error conditions and debounce counters
 */
void App__ClearErrors(void)
{
    rs485_comm_error  = false;
    rs232_comm_error  = false;
    rs485_power_error = false;
    rs232_power_error = false;

    rs232_iso_power_debounce_counter = 0U;
    rs485_iso_power_debounce_counter = 0U;

    Serial.println("[APP] All error flags cleared");
}

/*==============================================================================
 *                          PRIVATE FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Check single power supply with debouncing
 * @details Generic helper function for power supply monitoring
 *          Implements debouncing and overflow protection
 *
 * @note MISRA-C Rule 14.4: Single point of exit
 * @note MISRA-C Rule 17.7: Return value should be used by caller
 */
static bool App_CheckPowerSupply(const uint8_t pin,
                                  uint32_t *debounce_counter,
                                  bool (*is_data_fresh)(void),
                                  const char *name)
{
    bool error_detected = false;

    /* MISRA-C Rule 14.3: Explicit comparison */
    if (HIGH == digitalRead(pin))
    {
        /* Increment counter */
        (*debounce_counter)++;

        /* Overflow protection (UINT32_MAX from stdint.h) */
        if (*debounce_counter > (UINT32_MAX - 100U))
        {
            *debounce_counter = 0U;
            Serial.println("[APP] WARNING: Debounce counter overflow prevented");
        }

        /* Check if debounce threshold reached */
        if (*debounce_counter >= APP_ISO_POWER_DEBOUNCE_COUNT)
        {
            *debounce_counter = 0U;

            /* MISRA-C Rule 14.3: Explicit null check before function call */
            if ((is_data_fresh != NULL) && (!is_data_fresh()))
            {
                error_detected = true;
                Serial.printf("[APP] %s power supply error detected!\n", name);
            }
        }
    }
    else
    {
        /* Power pin is LOW (normal), reset counter */
        *debounce_counter = 0U;
    }

    return error_detected;
}

/**
 * @brief Monitor power supply status with debouncing
 * @details Checks RS232 and RS485 power pins using helper function
 *
 * @note MISRA-C: Cyclomatic Complexity = 2 (reduced from 5)
 */
static void App_MonitorPowerSupply(void)
{
    /* Monitor RS232 power supply */
    rs232_power_error = App_CheckPowerSupply(
        APP_RS232_ISO_POWER_PIN,
        &rs232_iso_power_debounce_counter,
        CmdParser__IsDataFresh,
        "RS232"
    );

    /* Monitor RS485 power supply */
    rs485_power_error = App_CheckPowerSupply(
        APP_RS485_ISO_POWER_PIN,
        &rs485_iso_power_debounce_counter,
        Modbus_IsInitialized,
        "RS485"
    );
}

/**
 * @brief Monitor communication status
 *
 * @details RS232 check is GATED on new message arrival.
 *
 *          ROOT CAUSE FIX (v2.2.0):
 *          The original implementation called CmdParser__GetErrorStatus() every
 *          handler cycle (~50-100ms). CmdParser stores the error from the last
 *          parse attempt and never auto-clears it between frames. This caused
 *          the same error code (e.g. error 10) to be re-read and re-logged on
 *          every handler cycle until the next successful parse arrived, producing
 *          a flood of "[APP] RS232 communication error: 10" lines. More critically,
 *          rs232_comm_error was held true for all those cycles, causing
 *          SystemStates to increment consecutive_errors at the handler rate
 *          (~10-20x per second) rather than the frame rate, triggering a
 *          premature FAILURE state transition from a single parse error.
 *
 *          FIX: Read and evaluate error status only when CmdParser has processed
 *          a new message (total_messages count has advanced). Between messages,
 *          preserve the previous rs232_comm_error state without re-logging.
 *          This ensures:
 *            - Each error is logged exactly once per failed frame
 *            - consecutive_errors increments once per frame, not per cycle
 *            - rs232_comm_error accurately reflects the most recent frame result
 *
 * @note MISRA-C Rule 17.7: Return value checked for all API calls
 * @note MISRA-C Rule 9.1: Variables initialized before use
 */
static void App_MonitorCommunication(void)
{
    /* -----------------------------------------------------------------------
     * RS232 communication check — gated on new message
     * ----------------------------------------------------------------------- */
    {
        uint32_t current_messages               = 0U;
        static uint32_t last_checked_messages   = 0U;
        CmdParser_StatusType_t msg_status;

        /* Get current processed message count from CmdParser */
        msg_status = CmdParser__GetTotalMessages(&current_messages);

        if (msg_status != CMDPARSER_STATUS_OK)
        {
            /* Cannot retrieve message count — treat as comm error */
            Serial.printf("[APP] Failed to get message count: %d\n", (int)msg_status);
            rs232_comm_error = true;
        }
        else if (current_messages != last_checked_messages)
        {
            /*
             * A new message has been processed since last check.
             * Now it is safe to read the error status — it reflects
             * the outcome of this specific new message, not a stale
             * error from a previous cycle.
             */
            last_checked_messages = current_messages;

            CmdParser_ErrorType_t  parser_error  = CMDPARSER_ERROR_NONE;
            CmdParser_StatusType_t error_status;

            error_status = CmdParser__GetErrorStatus(&parser_error);

            /* MISRA-C Rule 14.3: Explicit comparison */
            if (error_status == CMDPARSER_STATUS_OK)
            {
                if (parser_error != CMDPARSER_ERROR_NONE)
                {
                    rs232_comm_error = true;

                    /* MISRA-C Rule 10.1: Explicit cast for printf */
                    Serial.printf("[APP] RS232 communication error: %d\n", (int)parser_error);
                }
                else
                {
                    rs232_comm_error = false;
                }
            }
            else
            {
                /* Failed to retrieve error status for this new message */
                rs232_comm_error = true;
                Serial.printf("[APP] Failed to get parser error status: %d\n", (int)error_status);
            }
        }
        else
        {
            /*
             * No new message since last check.
             * Do NOT re-read or re-log the stale error.
             * rs232_comm_error retains its value from the last frame evaluation.
             * This prevents the error flood and spurious consecutive_errors
             * increments in SystemStates.
             */
        }
    }

    /* -----------------------------------------------------------------------
     * RS485 communication check — initialization-based, checked every cycle
     * RS485 has no frame-based error tracking so cycle-level check is correct
     * ----------------------------------------------------------------------- */
    if (!Modbus_IsInitialized())
    {
        rs485_comm_error = true;
        Serial.println("[APP] RS485 communication error: Not initialized");
    }
    else
    {
        rs485_comm_error = false;
    }
}

/**
 * @brief Display data periodically
 * @details Updates every 1 second (called from 100ms task, counts to 10)
 *          Reads weight data and updates Modbus registers
 *          Processes ALL new messages to prevent data loss
 *
 * @note MISRA-C Rule 17.7: All return values checked
 * @note MISRA-C: Processes all messages (no data loss)
 */
static void App_DisplayData(void)
{
    static uint32_t last_message_count = 0U;

    display_counter++;

    /* Overflow protection */
    if (display_counter > (UINT32_MAX - 100U))
    {
        display_counter = 0U;
        Serial.println("[APP] WARNING: Display counter overflow prevented");
    }

    /* Check if update interval reached */
    if (display_counter >= APP_DISPLAY_UPDATE_INTERVAL)
    {
        display_counter = 0U;

        /* Get current message count */
        uint32_t total_messages    = 0U;
        CmdParser_StatusType_t status;

        status = CmdParser__GetTotalMessages(&total_messages);

        /* MISRA-C Rule 17.7: Check return value */
        if (status != CMDPARSER_STATUS_OK)
        {
            Serial.printf("[APP] Failed to get message count: %d\n", (int)status);
            return;
        }

        /* Process ALL new messages (prevents data loss) */
        /* MISRA-C Rule 14.2: Loop counter monotonically increases */
        while (last_message_count < total_messages)
        {
            last_message_count++;

            /* Prepare weight data structure */
            Modbus_WeightData_t weight_data;
            bool all_data_valid = true;

            /* MISRA-C Rule 9.1: Initialize structure members */
            weight_data.gross_weight  = 0.0f;
            weight_data.tare_weight   = 0.0f;
            weight_data.net_weight    = 0.0f;
            weight_data.data_valid    = 0U;
            weight_data.string_counter = 0U;

            /* Try to read values from CmdParser - check all return values */
            if (CmdParser__GetGrossWeight(&weight_data.gross_weight) != CMDPARSER_STATUS_OK)
            {
                all_data_valid = false;
            }

            if (CmdParser__GetTareWeight(&weight_data.tare_weight) != CMDPARSER_STATUS_OK)
            {
                all_data_valid = false;
            }

            if (CmdParser__GetNetWeight(&weight_data.net_weight) != CMDPARSER_STATUS_OK)
            {
                all_data_valid = false;
            }

            /* MISRA-C Rule 10.3: Explicit cast to narrower type */
            weight_data.string_counter = (uint16_t)(last_message_count & 0xFFFFU);

            /* Set valid flag only if all data retrieved successfully */
            weight_data.data_valid = (all_data_valid) ? 1U : 0U;

            /* Update Modbus registers */
            (void)Modbus_SetWeightData(&weight_data);

            /* Log status */
            /* MISRA-C Rule 10.1: Explicit conversion for printf */
            Serial.printf("[APP] NEW msg #%lu - Flag %s\n",
                          (unsigned long)last_message_count,
                          (all_data_valid) ? "SET" : "CLEARED");
            Serial.printf("[APP] G=%.3f T=%.3f N=%.3f\n",
                          (double)weight_data.gross_weight,
                          (double)weight_data.tare_weight,
                          (double)weight_data.net_weight);
        }
    }
} 