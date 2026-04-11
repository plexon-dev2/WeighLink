/**
 * @file CmdParser_cfg.h
 * @brief Configuration header file for Sansui Command Parser module
 * @details This file contains all configurable parameters for the Sansui Command Parser
 *          module including UART settings, buffer configurations, and data processing parameters.
 *
 * @author 
 * @date 2025-11-03
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025
 */

#ifndef CMDPARSER_CFG_H
#define CMDPARSER_CFG_H

/*==============================================================================
 *                              INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 *                           UART CONFIGURATION
 *============================================================================*/

/**
 * @defgroup CmdParser_UART UART Configuration Parameters
 * @brief Configuration parameters for UART communication with Sansui weighing scale
 * @{
 */

/**
 * @brief UART baud rate configuration
 * @details Standard baud rates supported by Sansui weighing scale
 * @range 9600, 19200, 38400, 57600, 115200
 */
#define CMDPARSER_UART_BAUDRATE 9600U

/**
 * @brief UART data bits configuration
 * @details Number of data bits per frame
 * @note 7: 7 data bits, 8: 8 data bits
 */
#define CMDPARSER_UART_DATABITS 8U

/**
 * @brief UART stop bits configuration
 * @details Number of stop bits
 * @note 1: 1 stop bit, 2: 2 stop bits
 */
#define CMDPARSER_UART_STOPBITS 1U

/**
 * @brief UART parity configuration
 * @details Parity checking mode
 * @note 0: No parity, 1: Even parity, 2: Odd parity
 */
#define CMDPARSER_UART_PARITY 0U

/**
 * @brief UART flow control configuration
 * @details Hardware flow control enable/disable
 * @note false: Disabled, true: Enabled
 */
#define CMDPARSER_UART_FLOW_CONTROL_ENABLE false

/**
 * @brief UART channel selection for ESP32
 * @details Which Arduino Serial port to use for communication
 * @note 0: Serial (USB), 1: Serial1, 2: Serial2
 */
#define CMDPARSER_UART_CHANNEL 1U

/**
 * @brief UART RX pin configuration for ESP32
 * @details GPIO pin number for UART RX (receive) line
 * @note ESP32-C3 specific - GPIO 4 for Sansui scale connection
 */
#define CMDPARSER_UART_RX_PIN 4

/**
 * @brief UART TX pin configuration for ESP32
 * @details GPIO pin number for UART TX (transmit) line
 * @note Set to -1 if TX not needed (RX only configuration)
 */
#define CMDPARSER_UART_TX_PIN 5

/** @} */

/*==============================================================================
 *                           BUFFER CONFIGURATION
 *============================================================================*/

/**
 * @defgroup CmdParser_Buffers Buffer Configuration
 * @brief Configuration parameters for data buffers
 * @{
 */

/**
 * @brief Receive buffer size
 * @details Size of the circular buffer for incoming UART data from Sansui scale
 * @note Should be large enough to handle multiple messages
 * @range 512U to 4096U bytes
 */
#define CMDPARSER_RX_BUFFER_SIZE 1024U

/** @} */

/*==============================================================================
 *                           TIMING CONFIGURATION
 *============================================================================*/

/**
 * @defgroup CmdParser_Timing Timing Configuration
 * @brief Configuration parameters for timing and intervals
 * @{
 */

/**
 * @brief Inter-character timeout in milliseconds
 * @details Maximum time between characters to detect end of Sansui message
 * @note Calculated for 9600 baud rate: 1.04ms per character × 5 margin = 5ms
 * @note Used for asynchronous message boundary detection
 */
#define CMDPARSER_CHAR_TIMEOUT_MS 5U

/** @} */

/*==============================================================================
 *                      SANSUI WEIGHING SCALE CONFIGURATION
 *============================================================================*/

/**
 * @defgroup CmdParser_Sansui Sansui Weighing Scale Protocol Configuration
 * @brief Configuration parameters specific to Sansui weighing scale protocol
 * @{
 */

/**
 * @brief Sansui message length
 * @details Fixed length of Sansui weighing scale data message including special characters
 * @note Verified from actual Sansui data: 167 bytes total
 */
#define SANSUI_MESSAGE_LENGTH 167U

/**
 * @brief Expected number of fields in Sansui data frame
 * @details Total fields expected in a complete Sansui weighing scale message
 * @note Fields: Date, Time, Gross, Tare, Net, Status, Mode, Flags, Sample, Range_Min, Range_Max
 */
#define SANSUI_EXPECTED_FIELD_COUNT 11U

/**
 * @brief Date format specification
 * @details Sansui date format: DD/MM/YYYY
 */
#define SANSUI_DATE_FORMAT "DD/MM/YYYY"

/**
 * @brief Time format specification
 * @details Sansui time format: HH:MM:SS (24-hour format)
 */
#define SANSUI_TIME_FORMAT "HH:MM:SS"

/**
 * @brief Weight measurement unit
 * @details Unit of weight measurements from Sansui scale
 */
#define SANSUI_WEIGHT_UNIT "kg"

/**
 * @brief Maximum weight value
 * @details Maximum expected weight value from Sansui scale
 * @range Adjust based on scale capacity
 */
#define SANSUI_MAX_WEIGHT_VALUE 50

/**
 * @brief Minimum weight value
 * @details Minimum expected weight value from Sansui scale (can be negative for tare)
 */
#define SANSUI_MIN_WEIGHT_VALUE -5

/**
 * @brief Weight precision
 * @details Number of decimal places for Sansui weight values
 */
#define SANSUI_WEIGHT_PRECISION 3U

/** @} */

/*==============================================================================
 *                      DATA PROCESSING CONFIGURATION
 *============================================================================*/

/**
 * @defgroup CmdParser_DataProcessing Data Processing Configuration
 * @brief Configuration parameters for Sansui data processing and parsing
 * @{
 */

/**
 * @brief Enable HEX conversion logging
 * @details Enable/disable logging of ASCII to HEX conversion process
 * @note false: Disabled, true: Enabled - useful for debugging Sansui protocol issues
 */
#define CMDPARSER_ENABLE_HEX_LOGGING false

/**
 * @brief Enable data cleaning
 * @details Enable removal of unwanted characters (0x20, 0xE2, 0x80, 0x80)
 * @note true: Required for proper Sansui data processing
 */
#define CMDPARSER_ENABLE_DATA_CLEANING true

/**
 * @brief Command delimiter character
 * @details Character that marks the end of a Sansui data frame
 */
#define CMDPARSER_CMD_DELIMITER '\n'

/**
 * @brief Field separator character
 * @details Character used to separate fields in Sansui data frame
 */
#define CMDPARSER_FIELD_SEPARATOR ','

/**
 * @brief Maximum number of fields per Sansui message
 * @details Based on Sansui weighing scale protocol specification
 */
#define CMDPARSER_MAX_FIELDS SANSUI_EXPECTED_FIELD_COUNT

/**
 * @brief Float precision for Sansui weight values
 * @details Number of decimal places for weight measurements
 */
#define CMDPARSER_WEIGHT_PRECISION SANSUI_WEIGHT_PRECISION

/** @} */

/*==============================================================================
 *                        ERROR HANDLING CONFIGURATION
 *============================================================================*/

/**
 * @defgroup CmdParser_ErrorHandling Error Handling Configuration
 * @brief Configuration parameters for error handling and recovery
 * @{
 */

/**
 * @brief Maximum consecutive errors before reset
 * @details Number of consecutive errors before initiating recovery
 * @range 3U to 10U
 */
#define CMDPARSER_MAX_CONSECUTIVE_ERRORS 5U

/**
 * @brief Enable error recovery
 * @details Enable automatic error recovery mechanisms
 * @note false: Disabled, true: Enabled
 */
#define CMDPARSER_ENABLE_ERROR_RECOVERY true

/**
 * @brief Enable weight calculation validation
 * @details Enable validation that Net Weight = Gross Weight - Tare Weight
 * @note false: Disabled, true: Enabled
 */
#define CMDPARSER_ENABLE_WEIGHT_VALIDATION true

/**
 * @brief Weight calculation tolerance
 * @details Acceptable tolerance for weight calculation validation
 * @note Accounts for floating point precision errors
 */
#define CMDPARSER_WEIGHT_TOLERANCE 0.001f

/**
 * @brief Enable error statistics collection
 * @details Enable collection of error occurrence statistics
 * @note false: Disabled, true: Enabled
 */
#define CMDPARSER_ENABLE_ERROR_STATISTICS true

/** @} */

/*==============================================================================
 *                          DEBUG CONFIGURATION
 *============================================================================*/

/**
 * @defgroup CmdParser_Debug Debug Configuration
 * @brief Configuration parameters for debugging and logging
 * @{
 */

/**
 * @brief Debug level configuration
 * @details Level of debug information to be generated
 * @note 0: No debug, 1: Error only, 2: Error+Warning, 3: Info, 4: Verbose
 */
#define CMDPARSER_DEBUG_LEVEL 1U

/**
 * @brief Enable statistics collection
 * @details Enable collection of runtime statistics
 * @note false: Disabled, true: Enabled
 */
#define CMDPARSER_ENABLE_STATISTICS true

/** @} */

/*==============================================================================
 *                        VALIDATION MACROS
 *============================================================================*/

/**
 * @defgroup CmdParser_Validation Configuration Validation
 * @brief Compile-time validation of configuration parameters
 * @{
 */

/* Validate buffer sizes */
#if (CMDPARSER_RX_BUFFER_SIZE < 512U) || (CMDPARSER_RX_BUFFER_SIZE > 4096U)
#error "CMDPARSER_RX_BUFFER_SIZE must be between 512 and 4096 bytes"
#endif

/* Validate UART configuration */
#if (CMDPARSER_UART_DATABITS != 7U) && (CMDPARSER_UART_DATABITS != 8U)
#error "CMDPARSER_UART_DATABITS must be 7 or 8"
#endif

#if (CMDPARSER_UART_STOPBITS != 1U) && (CMDPARSER_UART_STOPBITS != 2U)
#error "CMDPARSER_UART_STOPBITS must be 1 or 2"
#endif

#if (CMDPARSER_UART_PARITY > 2U)
#error "CMDPARSER_UART_PARITY must be 0 (None), 1 (Even), or 2 (Odd)"
#endif

/* Validate timing parameters */
#if (CMDPARSER_CHAR_TIMEOUT_MS < 3U) || (CMDPARSER_CHAR_TIMEOUT_MS > 20U)
#error "CMDPARSER_CHAR_TIMEOUT_MS must be between 3 and 20 milliseconds for 9600 baud rate"
#endif

/* Validate Sansui-specific parameters */
#if (SANSUI_EXPECTED_FIELD_COUNT != 11U)
#error "SANSUI_EXPECTED_FIELD_COUNT must be 11 for current protocol definition"
#endif

#if (SANSUI_MAX_WEIGHT_VALUE <= SANSUI_MIN_WEIGHT_VALUE)
#error "SANSUI_MAX_WEIGHT_VALUE must be greater than SANSUI_MIN_WEIGHT_VALUE"
#endif

#if (CMDPARSER_MAX_FIELDS != SANSUI_EXPECTED_FIELD_COUNT)
#error "CMDPARSER_MAX_FIELDS must match SANSUI_EXPECTED_FIELD_COUNT"
#endif

#if (SANSUI_MESSAGE_LENGTH > CMDPARSER_RX_BUFFER_SIZE)
#error "SANSUI_MESSAGE_LENGTH cannot be greater than CMDPARSER_RX_BUFFER_SIZE"
#endif

/** @} */

#endif /* CMDPARSER_CFG_H */

/**
 * @}
 */
