/**
 * @file CmdParser.h
 * @brief Sansui Command Parser module header file
 * @details This file contains the public interface for the Sansui weighing scale
 *          command parser module including initialization, handler functions,
 *          and data access APIs.
 *
 * @author 
 * @date 2025-11-03
 * @version 1.1.0 - Added Epoch Time Support
 *
 * @copyright Copyright (c) 2025
 */

#ifndef CMDPARSER_H
#define CMDPARSER_H

/*==============================================================================
 *                              INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "CmdParser_cfg.h"

/*==============================================================================
 *                              DEFINES
 *============================================================================*/

/**
 * @brief Module version information
 */
#define CMDPARSER_MODULE_VERSION_MAJOR 1U
#define CMDPARSER_MODULE_VERSION_MINOR 1U
#define CMDPARSER_MODULE_VERSION_PATCH 0U

/*==============================================================================
 *                              TYPEDEFS
 *============================================================================*/

/**
 * @brief Sansui protocol field positions
 * @details Field positions in the comma-separated data format from Sansui weighing scale
 */
typedef enum
{
    SANSUI_FIELD_DATE = 0U,         /**< Date stamp (DD/MM/YYYY) */
    SANSUI_FIELD_TIME = 1U,         /**< Time stamp (HH:MM:SS 24hr format) */
    SANSUI_FIELD_GROSS_WEIGHT = 2U, /**< Gross weight value in kg */
    SANSUI_FIELD_TARE_WEIGHT = 3U,  /**< Tare weight value in kg */
    SANSUI_FIELD_NET_WEIGHT = 4U,   /**< Net weight value in kg */
    SANSUI_FIELD_STATUS_CODE = 5U,  /**< Status code (e.g., 32792) */
    SANSUI_FIELD_MODE = 6U,         /**< Mode indicator (e.g., 2) */
    SANSUI_FIELD_FLAGS = 7U,        /**< Flag bits (e.g., 0) */
    SANSUI_FIELD_SAMPLE_INFO = 8U,  /**< Sample information text */
    SANSUI_FIELD_RANGE_MIN = 9U,    /**< Minimum range value */
    SANSUI_FIELD_RANGE_MAX = 10U,   /**< Maximum range value */
    SANSUI_FIELD_COUNT = 11U        /**< Total number of expected fields */
} Sansui_FieldPosition_t;

/**
 * @brief Command Parser return status
 * @details Return codes for all CmdParser API functions
 */
typedef enum
{
    CMDPARSER_STATUS_OK = 0U,              /**< Operation successful */
    CMDPARSER_STATUS_ERROR = 1U,           /**< General error */
    CMDPARSER_STATUS_NOT_INITIALIZED = 2U, /**< Module not initialized */
    CMDPARSER_STATUS_INVALID_PARAM = 3U,   /**< Invalid parameter passed */
    CMDPARSER_STATUS_NO_DATA = 4U,         /**< No data available */
    CMDPARSER_STATUS_DATA_STALE = 5U,      /**< Data is outdated */
    CMDPARSER_STATUS_BUSY = 6U,            /**< Operation in progress */
    CMDPARSER_STATUS_TIMEOUT = 7U          /**< Operation timed out */
} CmdParser_StatusType_t;

/**
 * @brief Error types for Sansui weighing machine data validation
 * @details Specific error conditions for Sansui weighing scale data processing
 */
typedef enum
{
    CMDPARSER_ERROR_NONE = 0U,                  /**< No error */
    CMDPARSER_ERROR_INSUFFICIENT_BYTES = 1U,    /**< Insufficient bytes received in message */
    CMDPARSER_ERROR_INVALID_DATA = 2U,          /**< Data format is not valid */
    CMDPARSER_ERROR_INVALID_TIMESTAMP = 3U,     /**< Date/time format is wrong */
    CMDPARSER_ERROR_INVALID_DATE = 4U,          /**< Date field format invalid (not DD/MM/YYYY) */
    CMDPARSER_ERROR_INVALID_TIME = 5U,          /**< Time field format invalid (not HH:MM:SS) */
    CMDPARSER_ERROR_INVALID_GROSS_WEIGHT = 6U,  /**< Gross weight value out of range */
    CMDPARSER_ERROR_INVALID_TARE_WEIGHT = 7U,   /**< Tare weight value out of range */
    CMDPARSER_ERROR_INVALID_NET_WEIGHT = 8U,    /**< Net weight value out of range */
    CMDPARSER_ERROR_WEIGHT_CALCULATION = 9U,    /**< Net ≠ Gross - Tare validation failed */
    CMDPARSER_ERROR_FIELD_COUNT_MISMATCH = 10U, /**< Field count doesn't match expected */
    CMDPARSER_ERROR_CHECKSUM_FAIL = 11U,        /**< Data integrity check failed */
    CMDPARSER_ERROR_BUFFER_OVERFLOW = 12U,      /**< Receive buffer overflow */
    CMDPARSER_ERROR_UART_FRAME = 13U,           /**< UART framing error */
    CMDPARSER_ERROR_UART_OVERRUN = 14U,         /**< UART overrun error */
    CMDPARSER_ERROR_EPOCH_CONVERSION = 15U      /**< Epoch time conversion failed */
} CmdParser_ErrorType_t;

/**
 * @brief Weight data structure
 * @details Contains all weight measurements from Sansui scale
 */
typedef struct
{
    float gross_weight; /**< Gross weight in kg */
    float tare_weight;  /**< Tare weight in kg */
    float net_weight;   /**< Net weight in kg */
    bool valid;         /**< Data validity flag */
} CmdParser_WeightData_t;

/**
 * @brief Date structure
 * @details Date information from Sansui scale
 */
typedef struct
{
    uint8_t day;   /**< Day (1-31) */
    uint8_t month; /**< Month (1-12) */
    uint16_t year; /**< Year (e.g., 2025) */
    bool valid;    /**< Date validity flag */
} CmdParser_Date_t;

/**
 * @brief Time structure
 * @details Time information from Sansui scale (24-hour format)
 */
typedef struct
{
    uint8_t hour;   /**< Hour (0-23) */
    uint8_t minute; /**< Minute (0-59) */
    uint8_t second; /**< Second (0-59) */
    bool valid;     /**< Time validity flag */
} CmdParser_Time_t;

/**
 * @brief Timestamp structure
 * @details Combined date and time information
 */
typedef struct
{
    CmdParser_Date_t date; /**< Date information */
    CmdParser_Time_t time; /**< Time information */
    uint32_t timestamp_ms; /**< System timestamp when data was received */
    bool valid;            /**< Timestamp validity flag */
} CmdParser_Timestamp_t;

/**
 * @brief Sansui scale status information
 * @details Additional status and mode information from the scale
 */
typedef struct
{
    uint32_t status_code; /**< Scale status code */
    uint8_t mode;         /**< Operating mode */
    uint8_t flags;        /**< Status flags */
    float range_min;      /**< Minimum range value */
    float range_max;      /**< Maximum range value */
    char sample_info[64]; /**< Sample information string */
    bool valid;           /**< Status validity flag */
} CmdParser_ScaleStatus_t;

/**
 * @brief Data resolution information
 * @details Information about data precision and resolution
 */
typedef struct
{
    uint8_t weight_decimal_places; /**< Number of decimal places for weight */
    float weight_resolution;       /**< Smallest weight increment */
    uint8_t time_resolution_ms;    /**< Time resolution in milliseconds */
    bool valid;                    /**< Resolution info validity */
} CmdParser_DataResolution_t;

/*==============================================================================
 *                          FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * @defgroup CmdParser_SchedulerAPIs Scheduler Interface APIs
 * @brief APIs called by the scheduler for module operation
 * @{
 */

/**
 * @brief Initialize the Command Parser module
 * @details This function initializes the Sansui command parser module including
 *          UART configuration, buffer setup, and internal state initialization.
 *          Must be called once during system startup.
 *
 * @param None
 *
 * @return None
 *
 * @note This API is called by the scheduler during system initialization
 * @note UART channel and pin configuration taken from CmdParser_cfg.h
 * @note UART parameters (baud, parity, etc.) taken from CmdParser_cfg.h
 *
 * @par Example:
 * @code
 * // UART channel configured in CmdParser_cfg.h
 * CmdParser__Init();
 * @endcode
 */
void CmdParser__Init(void);

/**
 * @brief Command Parser main handler function
 * @details This function handles UART data reception, message parsing,
 *          data validation, and internal state updates. Should be called
 *          periodically by the scheduler.
 *
 * @param None
 * @return None
 *
 * @note This API is called by the scheduler in the main execution loop
 * @note Execution time should be kept minimal for real-time performance
 * @note Function is non-blocking and returns immediately
 *
 * @par Example:
 * @code
 * // Called by scheduler every 10ms
 * void SchedulerTask_10ms(void) {
 *     CmdParser__Handler();
 * }
 * @endcode
 */
void CmdParser__Handler(void);

/** @} */

/**
 * @defgroup CmdParser_DataAPIs Data Access APIs
 * @brief APIs for accessing parsed weighing scale data
 * @{
 */

/**
 * @brief Get net weight value
 * @details Retrieves the latest net weight measurement from Sansui scale
 *
 * @param[out] net_weight Pointer to store net weight value in kg (must not be NULL)
 *
 * @return CmdParser_StatusType_t
 * @retval CMDPARSER_STATUS_OK              Data retrieved successfully
 * @retval CMDPARSER_STATUS_NOT_INITIALIZED Module not initialized
 * @retval CMDPARSER_STATUS_INVALID_PARAM   Null pointer parameter
 * @retval CMDPARSER_STATUS_NO_DATA         No valid data available
 * @retval CMDPARSER_STATUS_DATA_STALE      Data is older than timeout threshold
 */
CmdParser_StatusType_t CmdParser__GetNetWeight(float *net_weight);

/**
 * @brief Get gross weight value
 * @details Retrieves the latest gross weight measurement from Sansui scale
 *
 * @param[out] gross_weight Pointer to store gross weight value in kg (must not be NULL)
 *
 * @return CmdParser_StatusType_t
 * @retval CMDPARSER_STATUS_OK              Data retrieved successfully
 * @retval CMDPARSER_STATUS_NOT_INITIALIZED Module not initialized
 * @retval CMDPARSER_STATUS_INVALID_PARAM   Null pointer parameter
 * @retval CMDPARSER_STATUS_NO_DATA         No valid data available
 * @retval CMDPARSER_STATUS_DATA_STALE      Data is older than timeout threshold
 */
CmdParser_StatusType_t CmdParser__GetGrossWeight(float *gross_weight);

/**
 * @brief Get tare weight value
 * @details Retrieves the latest tare weight measurement from Sansui scale
 *
 * @param[out] tare_weight Pointer to store tare weight value in kg (must not be NULL)
 *
 * @return CmdParser_StatusType_t
 * @retval CMDPARSER_STATUS_OK              Data retrieved successfully
 * @retval CMDPARSER_STATUS_NOT_INITIALIZED Module not initialized
 * @retval CMDPARSER_STATUS_INVALID_PARAM   Null pointer parameter
 * @retval CMDPARSER_STATUS_NO_DATA         No valid data available
 * @retval CMDPARSER_STATUS_DATA_STALE      Data is older than timeout threshold
 */
CmdParser_StatusType_t CmdParser__GetTareWeight(float *tare_weight);

/**
 * @brief Get timestamp information
 * @details Retrieves the latest timestamp (date and time) from Sansui scale
 *
 * @param[out] timestamp Pointer to timestamp structure (must not be NULL)
 *
 * @return CmdParser_StatusType_t
 * @retval CMDPARSER_STATUS_OK              Data retrieved successfully
 * @retval CMDPARSER_STATUS_NOT_INITIALIZED Module not initialized
 * @retval CMDPARSER_STATUS_INVALID_PARAM   Null pointer parameter
 * @retval CMDPARSER_STATUS_NO_DATA         No valid data available
 * @retval CMDPARSER_STATUS_DATA_STALE      Data is older than timeout threshold
 */
CmdParser_StatusType_t CmdParser__GetTimestamp(CmdParser_Timestamp_t *timestamp);

/**
 * @brief Get data resolution information
 * @details Retrieves information about data precision and resolution
 *
 * @param[out] resolution Pointer to data resolution structure (must not be NULL)
 *
 * @return CmdParser_StatusType_t
 * @retval CMDPARSER_STATUS_OK              Data retrieved successfully
 * @retval CMDPARSER_STATUS_NOT_INITIALIZED Module not initialized
 * @retval CMDPARSER_STATUS_INVALID_PARAM   Null pointer parameter
 */
CmdParser_StatusType_t CmdParser__GetDataResolution(CmdParser_DataResolution_t *resolution);

/**
 * @brief Get scale status information
 * @details Retrieves additional status information from Sansui scale
 *
 * @param[out] status Pointer to scale status structure (must not be NULL)
 *
 * @return CmdParser_StatusType_t
 * @retval CMDPARSER_STATUS_OK              Data retrieved successfully
 * @retval CMDPARSER_STATUS_NOT_INITIALIZED Module not initialized
 * @retval CMDPARSER_STATUS_INVALID_PARAM   Null pointer parameter
 * @retval CMDPARSER_STATUS_NO_DATA         No valid data available
 * @retval CMDPARSER_STATUS_DATA_STALE      Data is older than timeout threshold
 */
CmdParser_StatusType_t CmdParser__GetScaleStatus(CmdParser_ScaleStatus_t *status);

/**
 * @brief Get raw received buffer
 * @details Retrieves the last received raw message buffer for debugging and validation
 *
 * @param[out] buffer Pointer to buffer to copy raw data (must not be NULL)
 * @param[out] buffer_length Pointer to store actual buffer length (must not be NULL)
 * @param[in] max_length Maximum length of output buffer
 *
 * @return CmdParser_StatusType_t
 * @retval CMDPARSER_STATUS_OK              Buffer retrieved successfully
 * @retval CMDPARSER_STATUS_NOT_INITIALIZED Module not initialized
 * @retval CMDPARSER_STATUS_INVALID_PARAM   Null pointer parameter
 * @retval CMDPARSER_STATUS_NO_DATA         No buffer data available
 *
 * @note Buffer is erased when new message reception starts
 * @note Useful for comparing parsed data with raw frame data
 */
CmdParser_StatusType_t CmdParser__GetRawBuffer(uint8_t *buffer, uint16_t *buffer_length, uint16_t max_length);

/**
 * @brief Get last epoch time
 * @details Retrieves the last successfully converted epoch time
 *
 * @param[out] epoch_seconds Pointer to store epoch time in seconds (uint64_t)
 *
 * @return CmdParser_StatusType_t
 * @retval CMDPARSER_STATUS_OK              Epoch time retrieved successfully
 * @retval CMDPARSER_STATUS_NOT_INITIALIZED Module not initialized
 * @retval CMDPARSER_STATUS_INVALID_PARAM   Null pointer parameter
 * @retval CMDPARSER_STATUS_NO_DATA         No epoch time available
 *
 * @note Added in v1.1.0 for epoch time support
 */
CmdParser_StatusType_t CmdParser__GetEpochTime(uint64_t *epoch_seconds);

/** @} */

/**
 * @defgroup CmdParser_ErrorAPIs Error and Status APIs
 * @brief APIs for error handling and module status
 * @{
 */

/**
 * @brief Get current error status
 * @details Retrieves the current error status of the command parser
 *
 * @param[out] error_type Pointer to store current error type (must not be NULL)
 *
 * @return CmdParser_StatusType_t
 * @retval CMDPARSER_STATUS_OK              Error status retrieved successfully
 * @retval CMDPARSER_STATUS_NOT_INITIALIZED Module not initialized
 * @retval CMDPARSER_STATUS_INVALID_PARAM   Null pointer parameter
 */
CmdParser_StatusType_t CmdParser__GetErrorStatus(CmdParser_ErrorType_t *error_type);

/**
 * @brief Clear error status
 * @details Clears the current error status and resets error counters
 *
 * @param None
 *
 * @return CmdParser_StatusType_t
 * @retval CMDPARSER_STATUS_OK              Error status cleared successfully
 * @retval CMDPARSER_STATUS_NOT_INITIALIZED Module not initialized
 */
CmdParser_StatusType_t CmdParser__ClearErrorStatus(void);

/**
 * @brief Get module initialization status
 * @details Checks if the module has been properly initialized
 *
 * @param None
 *
 * @return bool
 * @retval true  Module is initialized
 * @retval false Module is not initialized
 */
bool CmdParser__IsInitialized(void);

/**
 * @brief Get data freshness status
 * @details Checks if the latest data is fresh (within timeout threshold)
 *
 * @param None
 *
 * @return bool
 * @retval true  Data is fresh and valid
 * @retval false Data is stale or invalid
 */
bool CmdParser__IsDataFresh(void);

/** @} */

/**
 * @defgroup CmdParser_UtilityAPIs Utility APIs
 * @brief Utility functions for module diagnostics and control
 * @{
 */

/**
 * @brief Reset module
 * @details Resets the module to initial state while keeping initialization
 *
 * @param None
 *
 * @return CmdParser_StatusType_t
 * @retval CMDPARSER_STATUS_OK              Module reset successfully
 * @retval CMDPARSER_STATUS_NOT_INITIALIZED Module not initialized
 */
CmdParser_StatusType_t CmdParser__Reset(void);

/**
 * @brief Get total number of messages received
 * @param count Pointer to store message count
 * @return Status code
 */
CmdParser_StatusType_t CmdParser__GetTotalMessages(uint32_t *count);

/** @} */

#endif /* CMDPARSER_H */

/**
 * @}
 */