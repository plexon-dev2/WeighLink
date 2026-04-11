/**
 * @file Modbus_DateTime_Cfg.h
 * @brief Modbus Date/Time Module Configuration
 *
 * @details
 * Configuration parameters for the Modbus DateTime module. Defines register
 * addresses, timing parameters, validation ranges, and feature enables.
 * All configuration parameters can be modified to adapt the module for
 * different hardware platforms and requirements.
 *
 * @version 1.0.0
 * @date 2025-12-04
 *
 * @author 
 * @company Plexon Devices LLP
 *
 * @note
 * - Modify parameters according to system requirements
 * - Validation checks ensure configuration integrity
 * - IST offset is configurable for different timezones
 *
 * @copyright Copyright (c) 2025 Plexon Devices LLP
 * @license Proprietary - All Rights Reserved
 */

#ifndef MODBUS_DATETIME_CFG_H
#define MODBUS_DATETIME_CFG_H

/*==============================================================================
 *                              INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 *                          REGISTER CONFIGURATION
 *============================================================================*/

/** Base address for date/time registers in Modbus memory map */
#define MODBUS_DATETIME_CFG_BASE_ADDRESS            (0x0008U)

/** Number of registers required for epoch time storage (2 for uint32_t) */
#define MODBUS_DATETIME_CFG_NUM_REGISTERS           (2U)

/*==============================================================================
 *                          TIME ZONE CONFIGURATION
 *============================================================================*/

/** Enable IST to UTC conversion (1 = enabled, 0 = disabled) */
#define MODBUS_DATETIME_CFG_ENABLE_IST_TO_UTC       (1U)

/** IST offset in seconds (5 hours 30 minutes = 19800 seconds) */
#define MODBUS_DATETIME_CFG_IST_OFFSET_SECONDS      (19800U)

/** Enable daylight saving time handling (1 = enabled, 0 = disabled) */
#define MODBUS_DATETIME_CFG_ENABLE_DST              (0U)

/*==============================================================================
 *                          VALIDATION CONFIGURATION
 *============================================================================*/

/** Minimum valid year */
#define MODBUS_DATETIME_CFG_MIN_YEAR                (1970U)

/** Maximum valid year */
#define MODBUS_DATETIME_CFG_MAX_YEAR                (2100U)

/** Minimum valid month (January) */
#define MODBUS_DATETIME_CFG_MIN_MONTH               (1U)

/** Maximum valid month (December) */
#define MODBUS_DATETIME_CFG_MAX_MONTH               (12U)

/** Minimum valid day */
#define MODBUS_DATETIME_CFG_MIN_DAY                 (1U)

/** Maximum valid day (depends on month, validated separately) */
#define MODBUS_DATETIME_CFG_MAX_DAY                 (31U)

/** Maximum valid hour (0-23) */
#define MODBUS_DATETIME_CFG_MAX_HOUR                (23U)

/** Maximum valid minute (0-59) */
#define MODBUS_DATETIME_CFG_MAX_MINUTE              (59U)

/** Maximum valid second (0-59) */
#define MODBUS_DATETIME_CFG_MAX_SECOND              (59U)

/*==============================================================================
 *                          DEBUG CONFIGURATION
 *============================================================================*/

/** Enable debug printing (1 = enabled, 0 = disabled) */
#define MODBUS_DATETIME_CFG_ENABLE_DEBUG_PRINT      (1U)

/** Enable verbose conversion logging (1 = enabled, 0 = disabled) */
#define MODBUS_DATETIME_CFG_ENABLE_VERBOSE_LOG      (1U)

/** Enable UTC time debug output (1 = enabled, 0 = disabled) */
#define MODBUS_DATETIME_CFG_ENABLE_UTC_DEBUG        (1U)

/** Enable register update logging (1 = enabled, 0 = disabled) */
#define MODBUS_DATETIME_CFG_ENABLE_REGISTER_LOG     (1U)

/*==============================================================================
 *                          BYTE ORDER CONFIGURATION
 *============================================================================*/

/** 
 * Byte order for register storage
 * 0 = Little Endian (matches existing float implementation)
 * 1 = Big Endian
 */
#define MODBUS_DATETIME_CFG_USE_BIG_ENDIAN          (0U)

/*==============================================================================
 *                          FEATURE CONFIGURATION
 *============================================================================*/

/** Enable statistics tracking (1 = enabled, 0 = disabled) */
#define MODBUS_DATETIME_CFG_ENABLE_STATISTICS       (1U)

/** Enable automatic register update on conversion (1 = enabled, 0 = disabled) */
#define MODBUS_DATETIME_CFG_AUTO_UPDATE_REGISTERS   (0U)

/** Enable epoch time caching (1 = enabled, 0 = disabled) */
#define MODBUS_DATETIME_CFG_ENABLE_EPOCH_CACHE      (1U)

/*==============================================================================
 *                          SCHEDULER CONFIGURATION
 *============================================================================*/

/** Handler execution period in milliseconds (0 = not periodic) */
#define MODBUS_DATETIME_CFG_HANDLER_PERIOD_MS       (0U)

/** Enable periodic handler execution (1 = enabled, 0 = disabled) */
#define MODBUS_DATETIME_CFG_ENABLE_HANDLER          (0U)

/*==============================================================================
 *                          CONFIGURATION VALIDATION
 *============================================================================*/

/* Validate register configuration */
#if (MODBUS_DATETIME_CFG_NUM_REGISTERS != 2U)
    #error "MODBUS_DATETIME_CFG_NUM_REGISTERS must be 2 for uint32_t epoch time"
#endif

/* Validate year range */
#if (MODBUS_DATETIME_CFG_MIN_YEAR >= MODBUS_DATETIME_CFG_MAX_YEAR)
    #error "MODBUS_DATETIME_CFG_MIN_YEAR must be less than MODBUS_DATETIME_CFG_MAX_YEAR"
#endif

/* Validate month range */
#if ((MODBUS_DATETIME_CFG_MIN_MONTH != 1U) || (MODBUS_DATETIME_CFG_MAX_MONTH != 12U))
    #error "Month range must be 1-12"
#endif

/* Validate time ranges */
#if (MODBUS_DATETIME_CFG_MAX_HOUR > 23U)
    #error "MODBUS_DATETIME_CFG_MAX_HOUR must be 23 or less"
#endif

#if (MODBUS_DATETIME_CFG_MAX_MINUTE > 59U)
    #error "MODBUS_DATETIME_CFG_MAX_MINUTE must be 59 or less"
#endif

#if (MODBUS_DATETIME_CFG_MAX_SECOND > 59U)
    #error "MODBUS_DATETIME_CFG_MAX_SECOND must be 59 or less"
#endif

/*==============================================================================
 *                          BACKWARD COMPATIBILITY
 *============================================================================*/

/** Backward compatibility: Map old define to new config */
#ifndef MODBUS_DATETIME_NUM_REGISTERS
    #define MODBUS_DATETIME_NUM_REGISTERS   MODBUS_DATETIME_CFG_NUM_REGISTERS
#endif

/** Backward compatibility: Map old define to new config */
#ifndef MODBUS_DATETIME_BASE_ADDRESS
    #define MODBUS_DATETIME_BASE_ADDRESS    MODBUS_DATETIME_CFG_BASE_ADDRESS
#endif

#endif /* MODBUS_DATETIME_CFG_H */