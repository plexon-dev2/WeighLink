/**
 * @file Modbus_DateTime.cpp
 * @brief Modbus Date/Time Management Implementation
 *
 * @details
 * Handles epoch time conversion and Modbus register updates. Converts
 * weighing machine date/time (IST) to Unix epoch time (UTC) and manages
 * storage in Modbus holding registers using Little Endian byte order.
 *
 * Register Layout (Little Endian - matches existing float implementation):
 * - Register[0]: Bits 0-15 (LSW - Least Significant Word)
 * - Register[1]: Bits 16-31 (MSW - Most Significant Word)
 *
 * @version 1.0.0
 * @date 2025-12-04
 *
 * @author 
 * @company Plexon Devices LLP
 *
 * @note
 * - Uses 32-bit epoch time (valid until year 2106)
 * - IST to UTC conversion subtracts 5:30 hours
 * - No dynamic memory allocation used
 *
 * @copyright Copyright (c) 2025 Plexon Devices LLP
 * @license Proprietary - All Rights Reserved
 */

/*==============================================================================
 *                              INCLUDES
 *============================================================================*/
#include "Modbus_DateTime.h"
#include "Modbus_DateTime_Cfg.h"
#include "Modbus.h"
#include "Arduino.h"
#include <time.h>

/*==============================================================================
 *                          LOCAL DEFINES
 *============================================================================*/

/** IST to UTC offset in seconds (5 hours 30 minutes) */
#define IST_OFFSET_SECONDS                  (19800U)

/** Maximum days in February for leap year */
#define FEBRUARY_LEAP_YEAR_DAYS             (29U)

/** Maximum days in February for non-leap year */
#define FEBRUARY_NORMAL_DAYS                (28U)

/** February month number */
#define MONTH_FEBRUARY                      (2U)

/** Number of months in a year */
#define MONTHS_IN_YEAR                      (12U)

/** Epoch base year */
#define EPOCH_BASE_YEAR                     (1900U)

/*==============================================================================
 *                          LOCAL TYPES
 *============================================================================*/

/* No local types required */

/*==============================================================================
 *                          LOCAL CONSTANTS
 *============================================================================*/

/** Days in each month (non-leap year) */
static const uint8_t DaysInMonth[MONTHS_IN_YEAR] = 
{
    31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
};

/*==============================================================================
 *                          LOCAL VARIABLES
 *============================================================================*/

/** Module initialization status */
static bool moduleInitialized = false;

/** Last converted epoch time */
static uint32_t lastEpochSeconds = 0U;

/** Conversion statistics counter */
static uint32_t conversionCount = 0U;

/** Conversion error counter */
static uint32_t conversionErrors = 0U;

/*==============================================================================
 *                          LOCAL FUNCTION PROTOTYPES
 *============================================================================*/

static bool Modbus_DateTime_ValidateDateTime(const Modbus_DateTime_t *datetime);
static bool Modbus_DateTime_IsLeapYear(uint16_t year);
static uint8_t Modbus_DateTime_GetDaysInMonth(uint8_t month, uint16_t year);

/*==============================================================================
 *                          PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize Modbus DateTime Module
 *
 * @details
 * Initializes module variables and status. Resets all counters and
 * prepares module for operation.
 *
 * @param None
 *
 * @return None
 */
void Modbus_DateTime__Init(void) 
{
    /* Initialize variables */
    lastEpochSeconds = 0U;
    conversionCount = 0U;
    conversionErrors = 0U;
    moduleInitialized = true;

    Serial.println("[MODBUS_DATETIME] Module initialized");
    Serial.println("[MODBUS_DATETIME] Byte order: Little Endian (LE)");
}

/**
 * @brief Periodic handler for Modbus DateTime Module
 *
 * @details
 * Reserved for future periodic processing. Currently not implemented
 * as module operates on-demand through API calls.
 *
 * @param None
 *
 * @return None
 */
void Modbus_DateTime__Handler(void)
{
    /* Reserved for future periodic processing */
    /* Currently no periodic tasks required */
}

/**
 * @brief Convert DateTime to Epoch Time
 *
 * @details
 * Converts IST date/time to UTC epoch time. Validates input parameters
 * and date/time values before conversion. Subtracts IST offset (5:30)
 * to get UTC time.
 *
 * @param[in] datetime Pointer to DateTime structure
 * @param[out] epoch__seconds Pointer to store epoch time in seconds
 *
 * @return Modbus_DateTime_Status_t
 */
Modbus_DateTime_Status_t Modbus_DateTime__ConvertToEpoch(
    const Modbus_DateTime_t *datetime,
    uint32_t *epoch__seconds) 
{
    Modbus_DateTime_Status_t returnStatus = MODBUS_DATETIME_STATUS_OK;
    struct tm timeinfo;
    time_t epochAsLocal = 0;
    uint32_t epochCorrected = 0U;

    /* Validate input parameters */
    if ((datetime == NULL) || (epoch__seconds == NULL)) 
    {
        Serial.println("[MODBUS_DATETIME] ERROR: NULL pointer");
        returnStatus = MODBUS_DATETIME_STATUS_INVALID_PARAM;
    }
    else
    {
        /* Validate date & time values */
        if (Modbus_DateTime_ValidateDateTime(datetime) == false) 
        {
            Serial.println("[MODBUS_DATETIME] ERROR: Invalid DateTime values");
            returnStatus = MODBUS_DATETIME_STATUS_CONVERSION_ERROR;
        }
        else
        {
            /* Fill struct tm with IST time (from weighing machine) */
            memset(&timeinfo, 0, sizeof(struct tm));
            timeinfo.tm_year = (int)(datetime->year - EPOCH_BASE_YEAR);
            timeinfo.tm_mon = (int)(datetime->month - 1U);
            timeinfo.tm_mday = (int)datetime->day;
            timeinfo.tm_hour = (int)datetime->hour;
            timeinfo.tm_min = (int)datetime->minute;
            timeinfo.tm_sec = (int)datetime->second;
            timeinfo.tm_isdst = -1;  /* India has no DST */

            /* Convert IST time to epoch (local) */
            epochAsLocal = mktime(&timeinfo);

            if (epochAsLocal == (time_t)(-1)) 
            {
                Serial.println("[MODBUS_DATETIME] ERROR: mktime failed");
                returnStatus = MODBUS_DATETIME_STATUS_CONVERSION_ERROR;
            }
            else
            {
                /* IST → UTC (subtract 5:30) */
                epochCorrected = (uint32_t)epochAsLocal - IST_OFFSET_SECONDS;

                /* Save output */
                *epoch__seconds = epochCorrected;

                /* Debug print of UTC time */
                time_t epochVal = (time_t)(*epoch__seconds);
                struct tm *utcTm = gmtime(&epochVal);

                if (utcTm != NULL)
                {
                    Serial.printf("[DEBUG] After IST->UTC Conversion: %04d-%02d-%02d %02d:%02d:%02d\n",
                                  utcTm->tm_year + EPOCH_BASE_YEAR,
                                  utcTm->tm_mon + 1,
                                  utcTm->tm_mday,
                                  utcTm->tm_hour,
                                  utcTm->tm_min,
                                  utcTm->tm_sec);
                }

                Serial.printf(
                    "[MODBUS_DATETIME] IST %04d-%02d-%02d %02d:%02d:%02d -> UTC Epoch: %lu\n",
                    datetime->year, datetime->month, datetime->day,
                    datetime->hour, datetime->minute, datetime->second,
                    epochCorrected);

                returnStatus = MODBUS_DATETIME_STATUS_OK;
            }
        }
    }

    return returnStatus;
}

/**
 * @brief Convert Epoch Time to DateTime
 *
 * @details
 * Converts UTC epoch time to human-readable date/time structure.
 * Output is in local system timezone.
 *
 * @param[in] epoch__seconds Epoch time in seconds
 * @param[out] datetime Pointer to store converted DateTime
 *
 * @return Modbus_DateTime_Status_t
 */
Modbus_DateTime_Status_t Modbus_DateTime__ConvertFromEpoch(
    uint32_t epoch__seconds,
    Modbus_DateTime_t *datetime) 
{
    Modbus_DateTime_Status_t returnStatus = MODBUS_DATETIME_STATUS_OK;
    time_t epochTime = 0;
    struct tm *timeinfo = NULL;

    /* Validate input parameter */
    if (datetime == NULL) 
    {
        Serial.println("[MODBUS_DATETIME] ERROR: NULL pointer");
        returnStatus = MODBUS_DATETIME_STATUS_INVALID_PARAM;
    }
    else
    {
        /* Convert epoch to tm structure */
        epochTime = (time_t)epoch__seconds;
        timeinfo = localtime(&epochTime);

        if (timeinfo == NULL)
        {
            Serial.println("[MODBUS_DATETIME] ERROR: localtime conversion failed");
            returnStatus = MODBUS_DATETIME_STATUS_CONVERSION_ERROR;
        }
        else
        {
            /* Fill DateTime structure */
            datetime->year = (uint16_t)(timeinfo->tm_year + EPOCH_BASE_YEAR);
            datetime->month = (uint8_t)(timeinfo->tm_mon + 1U);
            datetime->day = (uint8_t)timeinfo->tm_mday;
            datetime->hour = (uint8_t)timeinfo->tm_hour;
            datetime->minute = (uint8_t)timeinfo->tm_min;
            datetime->second = (uint8_t)timeinfo->tm_sec;

            Serial.printf("[MODBUS_DATETIME] Converted: %lu seconds -> %04d-%02d-%02d %02d:%02d:%02d\n",
                          epoch__seconds,
                          datetime->year, datetime->month, datetime->day,
                          datetime->hour, datetime->minute, datetime->second);

            returnStatus = MODBUS_DATETIME_STATUS_OK;
        }
    }

    return returnStatus;
}

/**
 * @brief Update Modbus Registers with Epoch Time
 *
 * @details
 * Splits 32-bit epoch time into 2x16-bit Modbus registers using Little
 * Endian byte order. Writes registers sequentially to Modbus.
 *
 * @param[in] epoch__seconds Epoch time in seconds
 * @param[in] base_address Starting Modbus register address
 *
 * @return Modbus_DateTime_Status_t
 */
Modbus_DateTime_Status_t Modbus_DateTime_UpdateRegisters(
    uint32_t epoch__seconds,
    uint16_t base_address)
{
    Modbus_DateTime_Status_t returnStatus = MODBUS_DATETIME_STATUS_OK;
    uint16_t registers[MODBUS_DATETIME_NUM_REGISTERS];
    Modbus_StatusType_t modbusStatus = MODBUS_STATUS_OK;
    uint8_t i = 0U;

    /* Convert epoch to registers (Little Endian) */
    Modbus_EpochToRegisters_LE(epoch__seconds, registers); 

    Serial.printf("[MODBUS_DATETIME] Updating registers at address 0x%04X\n", base_address);
    Serial.printf("[MODBUS_DATETIME] Epoch: %lu seconds\n", epoch__seconds);
    Serial.printf("[MODBUS_DATETIME] Registers (LE): [0]=0x%04X [1]=0x%04X\n",
                  registers[0], registers[1]);

    /* Write registers to Modbus (write each register individually) */
    for (i = 0U; i < MODBUS_DATETIME_NUM_REGISTERS; i++)
    {
        modbusStatus = Modbus_SetHoldingRegister((uint16_t)(base_address + i), registers[i]);

        if (modbusStatus != MODBUS_STATUS_OK) 
        {
            Serial.printf("[MODBUS_DATETIME] ERROR: Failed to write register %d (status: %d)\n",
                          i, (int)modbusStatus);
            returnStatus = MODBUS_DATETIME_STATUS_ERROR;
            break;
        }
    }

    if (returnStatus == MODBUS_DATETIME_STATUS_OK)
    {
        Serial.println("[MODBUS_DATETIME] Registers updated successfully");
    }

    return returnStatus;
}

/**
 * @brief Read Epoch Time from Modbus Registers
 *
 * @details
 * Reconstructs 32-bit epoch time from 2x16-bit Modbus registers using
 * Little Endian byte order. Reads registers sequentially.
 *
 * @param[in] base_address Starting Modbus register address
 * @param[out] epoch__seconds Pointer to store reconstructed epoch time
 *
 * @return Modbus_DateTime_Status_t
 */
Modbus_DateTime_Status_t Modbus_DateTime__ReadRegisters(
    uint16_t base_address,
    uint32_t *epoch__seconds) 
{
    Modbus_DateTime_Status_t returnStatus = MODBUS_DATETIME_STATUS_OK;
    uint16_t registers[MODBUS_DATETIME_NUM_REGISTERS];
    Modbus_StatusType_t modbusStatus = MODBUS_STATUS_OK;
    uint8_t i = 0U;

    /* Validate input parameter */
    if (epoch__seconds == NULL) 
    {
        Serial.println("[MODBUS_DATETIME] ERROR: NULL pointer");
        returnStatus = MODBUS_DATETIME_STATUS_INVALID_PARAM;
    }
    else
    {
        /* Read registers from Modbus */
        for (i = 0U; i < MODBUS_DATETIME_NUM_REGISTERS; i++) 
        {
            modbusStatus = Modbus_GetHoldingRegister((uint16_t)(base_address + i), &registers[i]);

            if (modbusStatus != MODBUS_STATUS_OK) 
            {
                Serial.printf("[MODBUS_DATETIME] ERROR: Failed to read register %d (status: %d)\n",
                              i, (int)modbusStatus);
                returnStatus = MODBUS_DATETIME_STATUS_ERROR;
                break;
            }
        }

        if (returnStatus == MODBUS_DATETIME_STATUS_OK)
        {
            /* Reconstruct epoch from registers (Little Endian) */
            *epoch__seconds = Modbus_RegistersToEpoch_LE(registers); 

            Serial.printf("[MODBUS_DATETIME] Read registers from address 0x%04X\n", base_address);
            Serial.printf("[MODBUS_DATETIME] Registers (LE): [0]=0x%04X [1]=0x%04X\n",
                          registers[0], registers[1]);
            Serial.printf("[MODBUS_DATETIME] Epoch: %lu seconds\n", *epoch__seconds);
        }
    }

    return returnStatus;
}

/**
 * @brief Update Modbus Registers with DateTime Structure
 *
 * @details
 * Convenience function combining conversion and register update.
 * Converts DateTime to epoch then updates Modbus registers.
 *
 * @param[in] datetime Pointer to DateTime structure
 * @param[in] base_address Starting Modbus register address
 *
 * @return Modbus_DateTime_Status_t
 */
Modbus_DateTime_Status_t Modbus_DateTime__UpdateFromDateTime(
    const Modbus_DateTime_t *datetime,
    uint16_t base_address) 
{
    Modbus_DateTime_Status_t returnStatus = MODBUS_DATETIME_STATUS_OK;
    uint32_t epochSeconds = 0U;

    /* Convert DateTime to epoch */
    returnStatus = Modbus_DateTime__ConvertToEpoch(datetime, &epochSeconds);

    if (returnStatus == MODBUS_DATETIME_STATUS_OK) 
    {
        /* Update registers with epoch time */
        returnStatus = Modbus_DateTime__UpdateRegisters(epochSeconds, base_address);
    }

    return returnStatus;
}

/**
 * @brief Validate DateTime Structure
 *
 * @details
 * Public wrapper for internal validation function. Checks if
 * date/time values are within valid ranges.
 *
 * @param[in] datetime Pointer to DateTime structure
 *
 * @return bool
 */
bool Modbus_DateTime__IsValid(const Modbus_DateTime_t *datetime) 
{
    bool isValid = false;

    if (datetime != NULL) 
    {
        isValid = Modbus_DateTime_ValidateDateTime(datetime);
    }

    return isValid;
}

/**
 * @brief Get Current System Epoch Time
 *
 * @details
 * Returns current system time as Unix epoch seconds using
 * standard time() function.
 *
 * @param[out] epoch__seconds Pointer to store current epoch time
 *
 * @return Modbus_DateTime_Status_t
 */
Modbus_DateTime_Status_t Modbus_DateTime__GetCurrentEpoch(uint32_t *epoch__seconds) 
{
    Modbus_DateTime_Status_t returnStatus = MODBUS_DATETIME_STATUS_OK;
    time_t now = 0;

    if (epoch__seconds == NULL) 
    {
        returnStatus = MODBUS_DATETIME_STATUS_INVALID_PARAM;
    }
    else
    {
        /* Get current time */
        now = time(NULL);
        *epoch__seconds = (uint32_t)now;

        Serial.printf("[MODBUS_DATETIME] Current epoch: %lu seconds\n", *epoch__seconds);

        returnStatus = MODBUS_DATETIME_STATUS_OK;
    }

    return returnStatus;
}

/*==============================================================================
 *                          LOCAL FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Validate DateTime values
 *
 * @details
 * Validates all date and time fields. Checks ranges for year, month,
 * day (accounting for month and leap year), hour, minute, and second.
 *
 * @param[in] datetime Pointer to DateTime structure
 *
 * @return bool
 */
static bool Modbus_DateTime_ValidateDateTime(const Modbus_DateTime_t *datetime)
{
    bool isValid = true;
    uint8_t maxDays = 0U;

    /* Validate year (1970-2100) */
    if ((datetime->year < MODBUS_DATETIME_CFG_MIN_YEAR) || 
        (datetime->year > MODBUS_DATETIME_CFG_MAX_YEAR)) 
    {
        Serial.printf("[MODBUS_DATETIME] Invalid year: %d\n", datetime->year);
        isValid = false;
    }
    /* Validate month (1-12) */
    else if ((datetime->month < MODBUS_DATETIME_CFG_MIN_MONTH) || 
             (datetime->month > MODBUS_DATETIME_CFG_MAX_MONTH)) 
    {
        Serial.printf("[MODBUS_DATETIME] Invalid month: %d\n", datetime->month);
        isValid = false;
    }
    else
    {
        /* Validate day (1-31, depends on month) */
        maxDays = Modbus_DateTime_GetDaysInMonth(datetime->month, datetime->year);
        
        if ((datetime->day < MODBUS_DATETIME_CFG_MIN_DAY) || 
            (datetime->day > maxDays)) 
        {
            Serial.printf("[MODBUS_DATETIME] Invalid day: %d (max: %d)\n",
                          datetime->day, maxDays);
            isValid = false;
        }
        /* Validate hour (0-23) */
        else if (datetime->hour > MODBUS_DATETIME_CFG_MAX_HOUR) 
        {
            Serial.printf("[MODBUS_DATETIME] Invalid hour: %d\n", datetime->hour);
            isValid = false;
        }
        /* Validate minute (0-59) */
        else if (datetime->minute > MODBUS_DATETIME_CFG_MAX_MINUTE) 
        {
            Serial.printf("[MODBUS_DATETIME] Invalid minute: %d\n", datetime->minute);
            isValid = false;
        }
        /* Validate second (0-59) */
        else if (datetime->second > MODBUS_DATETIME_CFG_MAX_SECOND) 
        {
            Serial.printf("[MODBUS_DATETIME] Invalid second: %d\n", datetime->second);
            isValid = false;
        }
        else
        {
            /* All validations passed */
            isValid = true;
        }
    }

    return isValid;
}

/**
 * @brief Check if year is a leap year
 *
 * @details
 * Implements leap year calculation: divisible by 4, except century
 * years unless divisible by 400.
 *
 * @param[in] year Year to check
 *
 * @return bool
 */
static bool Modbus_DateTime_IsLeapYear(uint16_t year) 
{
    bool isLeap = false;

    if ((year % 4U) != 0U) 
    {
        isLeap = false;
    } 
    else if ((year % 100U) != 0U) 
    {
        isLeap = true;
    } 
    else if ((year % 400U) != 0U) 
    {
        isLeap = false;
    } 
    else 
    {
        isLeap = true;
    }

    return isLeap;
}

/**
 * @brief Get number of days in a month
 *
 * @details
 * Returns days in specified month, accounting for leap years in
 * February. Uses lookup table for efficiency.
 *
 * @param[in] month Month (1-12)
 * @param[in] year Year (for leap year calculation)
 *
 * @return uint8_t Number of days in month (0 if invalid)
 */
static uint8_t Modbus_DateTime_GetDaysInMonth(uint8_t month, uint16_t year) 
{
    uint8_t days = 0U;

    if ((month < 1U) || (month > MONTHS_IN_YEAR)) 
    {
        days = 0U;
    }
    else
    {
        days = DaysInMonth[month - 1U];

        /* Adjust for February in leap years */
        if ((month == MONTH_FEBRUARY) && (Modbus_DateTime_IsLeapYear(year) == true)) 
        {
            days = FEBRUARY_LEAP_YEAR_DAYS;
        }
    }

    return days;
}