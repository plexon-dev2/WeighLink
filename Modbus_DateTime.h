/**
 * @file Modbus_DateTime.h
 * @brief Modbus Date/Time Management Interface
 *
 * @details
 * Public interface for epoch time conversion and Modbus register updates.
 * Uses Little Endian byte order to match existing Modbus implementation.
 * Converts weighing machine date/time to Unix epoch time and manages
 * storage in Modbus holding registers.
 *
 * @version 1.0.0
 * @date 2025-12-04
 *
 * @author Generated Module
 * @company Plexon Devices LLP
 *
 * @note
 * - Requires C99 or later
 * - Uses 32-bit epoch time (valid until year 2106)
 * - IST to UTC conversion enabled by default
 *
 * @copyright Copyright (c) 2025 Plexon Devices LLP
 * @license Proprietary - All Rights Reserved
 */

#ifndef MODBUS_DATETIME_H
#define MODBUS_DATETIME_H

/*==============================================================================
 *                              INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "Modbus_DateTime_Cfg.h"

/*==============================================================================
 *                              PUBLIC DEFINES
 *============================================================================*/

/* Configuration parameters are defined in Modbus_DateTime_Cfg.h */

/*==============================================================================
 *                              PUBLIC TYPES
 *============================================================================*/

/**
 * @brief Date and Time structure
 *
 * @details
 * Holds raw date and time values from weighing machine in IST timezone.
 * All fields must be validated before conversion to epoch time.
 */
typedef struct
{
    uint16_t year;      /**< Year (1970-2100) */
    uint8_t month;      /**< Month (1-12) */
    uint8_t day;        /**< Day (1-31) */
    uint8_t hour;       /**< Hour (0-23) */
    uint8_t minute;     /**< Minute (0-59) */
    uint8_t second;     /**< Second (0-59) */
} Modbus_DateTime_t;

/**
 * @brief Status codes for date/time operations
 */
typedef enum
{
    MODBUS_DATETIME_STATUS_OK = 0,                  /**< Operation successful */
    MODBUS_DATETIME_STATUS_ERROR,                   /**< General error */
    MODBUS_DATETIME_STATUS_INVALID_PARAM,           /**< Invalid parameter */
    MODBUS_DATETIME_STATUS_CONVERSION_ERROR,        /**< Epoch conversion error */
    MODBUS_DATETIME_STATUS_INVALID                  /**< Invalid status (always last) */
} Modbus_DateTime_Status_t;

/*==============================================================================
 *                          PUBLIC FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @defgroup Modbus_DateTime_SchedulerAPIs Scheduler Interface APIs
 * @brief APIs called by the scheduler for module operation
 * @{
 */

/**
 * @brief Initialize Modbus DateTime Module
 *
 * @details
 * Initializes module variables, resets conversion statistics, and sets up
 * the module for operation. Must be called once during system initialization
 * before any other module functions are used.
 *
 * @param None
 *
 * @return None
 *
 * @pre None
 * @post Module is initialized and ready for operation
 *
 * @note This API is called by the scheduler during system initialization
 */
void Modbus_DateTime__Init(void);

/**
 * @brief Periodic handler for Modbus DateTime Module
 *
 * @details
 * Reserved for future periodic processing if needed. Currently not used
 * as module operates on-demand through API calls.
 *
 * @param None
 *
 * @return None
 *
 * @note This API can be called by the scheduler periodically if needed
 */
void Modbus_DateTime__Handler(void);

/** @} */

/**
 * @defgroup Modbus_DateTime_ConversionAPIs Conversion APIs
 * @brief APIs for epoch time conversion
 * @{
 */

/**
 * @brief Convert DateTime to Epoch Time
 *
 * @details
 * Converts raw date/time structure to Unix epoch time (seconds since 
 * January 1, 1970 00:00:00 UTC). Input time is assumed to be in IST 
 * (Indian Standard Time) and is automatically converted to UTC by 
 * subtracting 5 hours 30 minutes offset.
 *
 * @param[in] datetime Pointer to DateTime structure (must not be NULL)
 * @param[out] epoch__seconds Pointer to store epoch time in seconds
 *
 * @return Modbus_DateTime_Status_t
 * @retval MODBUS_DATETIME_STATUS_OK Conversion successful
 * @retval MODBUS_DATETIME_STATUS_INVALID_PARAM NULL pointer provided
 * @retval MODBUS_DATETIME_STATUS_CONVERSION_ERROR Invalid date/time values
 *
 * @pre Module must be initialized via Modbus_DateTime__Init()
 * @post epoch__seconds contains valid Unix epoch time in UTC
 *
 * @note Output epoch time is in seconds as uint32_t (valid until 2106)
 * @warning Input datetime values must be validated before conversion
 */
Modbus_DateTime_Status_t Modbus_DateTime__ConvertToEpoch(
    const Modbus_DateTime_t *datetime,
    uint32_t *epoch__seconds);

/**
 * @brief Convert Epoch Time to DateTime
 *
 * @details
 * Converts Unix epoch time back to human-readable date/time structure.
 * Output time is in local system timezone (typically IST).
 *
 * @param[in] epoch__seconds Epoch time in seconds (uint32_t)
 * @param[out] datetime Pointer to store converted DateTime (must not be NULL)
 *
 * @return Modbus_DateTime_Status_t
 * @retval MODBUS_DATETIME_STATUS_OK Conversion successful
 * @retval MODBUS_DATETIME_STATUS_INVALID_PARAM NULL pointer provided
 * @retval MODBUS_DATETIME_STATUS_CONVERSION_ERROR Conversion failed
 *
 * @pre Module must be initialized via Modbus_DateTime__Init()
 * @post datetime contains valid date/time representation
 */
Modbus_DateTime_Status_t Modbus_DateTime__ConvertFromEpoch(
    uint32_t epoch__seconds,
    Modbus_DateTime_t *datetime);

/** @} */

/**
 * @defgroup Modbus_DateTime_RegisterAPIs Register Management APIs
 * @brief APIs for Modbus register operations
 * @{
 */

/**
 * @brief Update Modbus Registers with Epoch Time
 *
 * @details
 * Splits 32-bit epoch time into 2 x 16-bit Modbus registers using Little
 * Endian byte order (matches existing float implementation). Register[0]
 * contains bits 0-15 (LSW), Register[1] contains bits 16-31 (MSW).
 *
 * @param[in] epoch__seconds Epoch time in seconds (uint32_t)
 * @param[in] base_address Starting Modbus register address
 *
 * @return Modbus_DateTime_Status_t
 * @retval MODBUS_DATETIME_STATUS_OK Registers updated successfully
 * @retval MODBUS_DATETIME_STATUS_ERROR Register write failed
 *
 * @pre Module must be initialized via Modbus_DateTime__Init()
 * @post Modbus registers contain epoch time data
 *
 * @note Register layout (Little Endian):
 *       Register[0]: Bits 0-15 (LSW)
 *       Register[1]: Bits 16-31 (MSW)
 */
Modbus_DateTime_Status_t Modbus_DateTime__UpdateRegisters(
    uint32_t epoch__seconds,
    uint16_t base_address);

/**
 * @brief Read Epoch Time from Modbus Registers
 *
 * @details
 * Reconstructs 32-bit epoch time from 2 x 16-bit Modbus registers using
 * Little Endian byte order. Reads registers sequentially and combines
 * them into epoch time value.
 *
 * @param[in] base_address Starting Modbus register address
 * @param[out] epoch__seconds Pointer to store reconstructed epoch time
 *
 * @return Modbus_DateTime_Status_t
 * @retval MODBUS_DATETIME_STATUS_OK Read successful
 * @retval MODBUS_DATETIME_STATUS_ERROR Register read failed
 * @retval MODBUS_DATETIME_STATUS_INVALID_PARAM NULL pointer provided
 *
 * @pre Module must be initialized via Modbus_DateTime__Init()
 * @post epoch__seconds contains reconstructed epoch time
 */
Modbus_DateTime_Status_t Modbus_DateTime__ReadRegisters(
    uint16_t base_address,
    uint32_t *epoch__seconds);

/**
 * @brief Update Modbus Registers with DateTime Structure
 *
 * @details
 * Convenience function that converts DateTime to epoch and updates Modbus
 * registers in a single call. Combines ConvertToEpoch and UpdateRegisters
 * operations.
 *
 * @param[in] datetime Pointer to DateTime structure (must not be NULL)
 * @param[in] base_address Starting Modbus register address
 *
 * @return Modbus_DateTime_Status_t
 * @retval MODBUS_DATETIME_STATUS_OK Operation successful
 * @retval MODBUS_DATETIME_STATUS_INVALID_PARAM NULL pointer provided
 * @retval MODBUS_DATETIME_STATUS_CONVERSION_ERROR Conversion failed
 * @retval MODBUS_DATETIME_STATUS_ERROR Register update failed
 *
 * @pre Module must be initialized via Modbus_DateTime__Init()
 * @post Modbus registers contain epoch time from datetime
 */
Modbus_DateTime_Status_t Modbus_DateTime__UpdateFromDateTime(
    const Modbus_DateTime_t *datetime,
    uint16_t base_address);

/** @} */

/**
 * @defgroup Modbus_DateTime_UtilityAPIs Utility APIs
 * @brief Helper and validation APIs
 * @{
 */

/**
 * @brief Validate DateTime Structure
 *
 * @details
 * Checks if date/time values are within valid ranges. Validates year
 * (1970-2100), month (1-12), day (1-31 depending on month), hour (0-23),
 * minute (0-59), and second (0-59). Accounts for leap years.
 *
 * @param[in] datetime Pointer to DateTime structure
 *
 * @return bool
 * @retval true DateTime values are valid
 * @retval false DateTime values are invalid or pointer is NULL
 *
 * @note This validation is also performed internally during conversion
 */
bool Modbus_DateTime__IsValid(const Modbus_DateTime_t *datetime);

/**
 * @brief Get Current System Epoch Time
 *
 * @details
 * Returns current system time as Unix epoch seconds. Uses system time()
 * function to retrieve current timestamp.
 *
 * @param[out] epoch__seconds Pointer to store current epoch time
 *
 * @return Modbus_DateTime_Status_t
 * @retval MODBUS_DATETIME_STATUS_OK Operation successful
 * @retval MODBUS_DATETIME_STATUS_INVALID_PARAM NULL pointer provided
 *
 * @pre System time must be initialized
 * @post epoch__seconds contains current system time
 */
Modbus_DateTime_Status_t Modbus_DateTime__GetCurrentEpoch(
    uint32_t *epoch__seconds);

/** @} */

#endif /* MODBUS_DATETIME_H */