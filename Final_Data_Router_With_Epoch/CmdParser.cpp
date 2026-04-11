/**
 * @file CmdParser.cpp
 * @brief Sansui Command Parser module implementation with Epoch Time Support
 * @details Implementation of Sansui weighing scale command parser module with
 *          FreeRTOS integration, Arduino Serial1 library usage, MISRA C compliance,
 *          and epoch time conversion for Modbus integration.
 *
 * @author Generated Module
 * @date 2025-11-03
 * @version 1.1.0 - Added Epoch Time Support
 *
 * @copyright Copyright (c) 2025
 *
 * CHANGELOG v1.1.0:
 * - Added Modbus_DateTime.h integration
 * - Added epoch time conversion after timestamp parsing
 * - Added automatic Modbus register update with epoch time
 * - Added epoch time getter API
 * - Added epoch conversion error handling
 */

/*==============================================================================
 *                              INCLUDES
 *============================================================================*/
#include "CmdParser.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include "Modbus.h"
#include "Modbus_DateTime.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "LED_HMI.h"

/*==============================================================================
 *                              DEFINES
 *============================================================================*/

/**
 * @brief Message timeout in milliseconds
 * @details Maximum time to wait for complete message after first byte
 */
#define MESSAGE_TIMEOUT_MS 250U

/**
 * @brief Maximum sample info string length
 * @details Buffer size for sample information field
 */
#define MAX_SAMPLE_INFO_LENGTH 64U

/**
 * @brief Data freshness timeout in milliseconds
 * @details Maximum age of data before considered stale
 */
#define DATA_FRESHNESS_TIMEOUT_MS 5000U

/**
 * @brief Maximum field string length
 */
#define MAX_FIELD_LENGTH 32U

/*==============================================================================
 *                              TYPEDEFS
 *============================================================================*/

/**
 * @brief Module internal state enumeration
 * @details Internal state machine for message processing
 */
typedef enum
{
    CMDPARSER_STATE_UNINITIALIZED = 0U, /**< Module not initialized */
    CMDPARSER_STATE_INITIALIZED = 1U,   /**< Module initialized, waiting for data */
    CMDPARSER_STATE_RECEIVING = 2U,     /**< Receiving message data */
    CMDPARSER_STATE_PROCESSING = 3U,    /**< Processing complete message */
    CMDPARSER_STATE_DATA_READY = 4U,    /**< Valid data available */
    CMDPARSER_STATE_ERROR = 5U          /**< Error state */
} CmdParser_InternalState_t;

/*==============================================================================
 *                          STATIC VARIABLES
 *============================================================================*/

/* UART channel selection based on configuration */
#if (CMDPARSER_UART_CHANNEL == 0U)
#define UART_PORT Serial
#elif (CMDPARSER_UART_CHANNEL == 1U)
#define UART_PORT Serial1
#elif (CMDPARSER_UART_CHANNEL == 2U)
#define UART_PORT Serial2
#else
#error "Invalid UART channel configuration. Must be 0, 1, or 2"
#endif

/* Module state variables */
static volatile CmdParser_InternalState_t module_state = CMDPARSER_STATE_UNINITIALIZED;
static volatile bool module_initialized = false;
static volatile CmdParser_ErrorType_t current_error = CMDPARSER_ERROR_NONE;

/* Message reception variables */
static uint8_t rx_buffer[SANSUI_MESSAGE_LENGTH];
static uint8_t last_complete_buffer[SANSUI_MESSAGE_LENGTH];
static volatile uint16_t bytes_received = 0U;
static uint16_t last_complete_length = 0U;
static volatile uint32_t message_start_time = 0U;
static volatile uint32_t last_char_time = 0U;
static volatile bool message_complete = false;
static volatile bool new_data_available = false;
static volatile bool raw_buffer_available = false;

/* Parsed data storage */
static CmdParser_WeightData_t current_weights = {0.0f, 0.0f, 0.0f, false};
static CmdParser_Timestamp_t current_timestamp = {{0U, 0U, 0U, false}, {0U, 0U, 0U, false}, 0U, false};
static CmdParser_ScaleStatus_t current_status = {0U, 0U, 0U, 0.0f, 0.0f, {0}, false};
static CmdParser_DataResolution_t resolution_info = {SANSUI_WEIGHT_PRECISION, 0.001f, 0U, true};

/* NEW: Epoch time storage */
static uint64_t current_epoch_seconds = 0U;
static bool epoch_valid = false;
static uint32_t epoch_conversion_count = 0U;
static uint32_t epoch_conversion_errors = 0U;

/* Data freshness tracking */
static volatile uint32_t last_valid_data_time = 0U;

/* Statistics counters */
static uint32_t total_messages_received = 0U;
static uint32_t total_errors_detected = 0U;
static uint32_t consecutive_errors = 0U;

/* Mutex for thread safety */
static SemaphoreHandle_t data_mutex = NULL;
static portMUX_TYPE critical_mux = portMUX_INITIALIZER_UNLOCKED;

/*==============================================================================
 *                          STATIC BUFFERS (MOVED FROM STACK)
 *============================================================================*/

/**
 * @brief Static parsing buffers (thread-safe due to mutex protection)
 * @note These buffers are protected by data_mutex in ParseMessage()
 */
static char s_message_str[SANSUI_MESSAGE_LENGTH + 1U];
static char s_fields[SANSUI_EXPECTED_FIELD_COUNT][MAX_FIELD_LENGTH];
static char s_date_copy[16];
static char s_time_copy[16];

/*==============================================================================
 *                          STATIC FUNCTION PROTOTYPES
 *============================================================================*/
static bool IsValidPointer(const void *ptr);
static void EnterCriticalSection(void);
static void ExitCriticalSection(void);
static void ResetMessageState(void);
static CmdParser_ErrorType_t ParseMessage(void);
static CmdParser_ErrorType_t ParseTimestamp(const char *date_str, const char *time_str);
static CmdParser_ErrorType_t ParseWeight(const char *weight_str, float *weight_value);
static CmdParser_ErrorType_t ValidateWeightCalculation(void);
static void UpdateErrorStatus(CmdParser_ErrorType_t error_type);
static bool IsDataFresh(void);
static bool StringToUint32(const char *str, uint32_t *value);
static bool StringToFloat(const char *str, float *value);
static void TrimString(char *str);
static uint8_t SplitFields(char *message_str, char fields[][MAX_FIELD_LENGTH], uint8_t max_fields);

/*  NEW: Epoch time conversion function */
static CmdParser_ErrorType_t ConvertTimestampToEpoch(void);

/*==============================================================================
 *                          PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize the Command Parser module
 */
void CmdParser__Init(void)
{
    /* Create mutex for thread safety */
    if (data_mutex == NULL)
    {
        data_mutex = xSemaphoreCreateMutex();
    }

    /* Initialize Arduino Serial port with configuration from cfg.h */
    UART_PORT.setRxBufferSize(1024); // Enlarged RX buffer
    UART_PORT.begin(CMDPARSER_UART_BAUDRATE, SERIAL_8N1, CMDPARSER_UART_RX_PIN, CMDPARSER_UART_TX_PIN);

    Serial.printf("[CMDPARSER] UART init: channel=%u baud=%lu rx=%d tx=%d\n",
                  (unsigned)CMDPARSER_UART_CHANNEL,
                  (unsigned long)CMDPARSER_UART_BAUDRATE,
                  (int)CMDPARSER_UART_RX_PIN,
                  (int)CMDPARSER_UART_TX_PIN);

    /* Initialize module state */
    if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE)
    {
        module_state = CMDPARSER_STATE_INITIALIZED;
        module_initialized = true;
        current_error = CMDPARSER_ERROR_NONE;

        /* Reset all data structures */
        ResetMessageState();

        memset(&current_weights, 0, sizeof(current_weights));
        memset(&current_timestamp, 0, sizeof(current_timestamp));
        memset(&current_status, 0, sizeof(current_status));
        memset(last_complete_buffer, 0, sizeof(last_complete_buffer));

        last_complete_length = 0U;
        raw_buffer_available = false;

        /*  NEW: Initialize epoch time variables */
        current_epoch_seconds = 0U;
        epoch_valid = false;
        epoch_conversion_count = 0U;
        epoch_conversion_errors = 0U;

        /* Initialize resolution info */
        resolution_info.weight_decimal_places = SANSUI_WEIGHT_PRECISION;
        resolution_info.weight_resolution = 0.001f;
        resolution_info.time_resolution_ms = 0U;
        resolution_info.valid = true;

        /* Reset statistics */
        total_messages_received = 0U;
        total_errors_detected = 0U;
        consecutive_errors = 0U;
        last_valid_data_time = 0U;

        xSemaphoreGive(data_mutex);
    }

    Serial.println("[CMDPARSER] Module initialized with Epoch Time support");
}

/**
 * @brief Command Parser main handler function
 */
void CmdParser__Handler(void)
{
    uint32_t current_time;
    int available_bytes;
    bool timeout_detected = false;
    bool should_process = false;

    /* Check if module is initialized */
    if (!module_initialized)
    {
        return;
    }

    current_time = millis();

    /* Read available data from UART */
    available_bytes = UART_PORT.available();

    if (available_bytes > 0)
    {
        Serial.printf("[DBG] RX available = %d bytes\n", available_bytes);
    }

    if (available_bytes > 0)
    {
        taskENTER_CRITICAL(&critical_mux);

        /* Update state if starting new message */
        if (bytes_received == 0U)
        {
            module_state = CMDPARSER_STATE_RECEIVING;
            message_start_time = current_time;

            /* Clear raw buffer when starting new message */
            memset(last_complete_buffer, 0, sizeof(last_complete_buffer));
            last_complete_length = 0U;
            raw_buffer_available = false;
        }

        /* Read all available bytes */
        while ((UART_PORT.available() > 0) && (bytes_received < SANSUI_MESSAGE_LENGTH))
        {
            int received_byte = UART_PORT.read();
            if (received_byte >= 0)
            {
                rx_buffer[bytes_received] = (uint8_t)received_byte;
                bytes_received++;
                last_char_time = current_time;

                /* Check for line ending (message complete) */
                if ((received_byte == '\n') || (received_byte == '\r'))
                {
                    message_complete = true;
                    break;
                }
            }
        }

        /* Check if maximum bytes reached */
        if (bytes_received >= SANSUI_MESSAGE_LENGTH)
        {
            message_complete = true;
        }

        taskEXIT_CRITICAL(&critical_mux);
    }

    /* Check for timeout */
    if ((bytes_received > 0U) && ((current_time - message_start_time) > MESSAGE_TIMEOUT_MS))
    {
        timeout_detected = true;
    }

    /* Determine if we should process */
    taskENTER_CRITICAL(&critical_mux);
    should_process = (message_complete || timeout_detected);
    taskEXIT_CRITICAL(&critical_mux);

    /* Process complete message or handle timeout */
    if (should_process)
    {
        if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            module_state = CMDPARSER_STATE_PROCESSING;

            /*  Reset flag when starting to process new message */
            new_data_available = false;

            CmdParser_ErrorType_t parse_result = CMDPARSER_ERROR_NONE;

            if (message_complete && (bytes_received > 0U))
            {
                /* Parse the message */
                parse_result = ParseMessage();

                if (parse_result == CMDPARSER_ERROR_NONE)
                {
                    /* Convert timestamp to epoch time */
                    CmdParser_ErrorType_t epoch_result = ConvertTimestampToEpoch();

                    if (epoch_result == CMDPARSER_ERROR_NONE)
                    {
                        epoch_conversion_count++;
                        /* Modbus already updated inside ConvertTimestampToEpoch() */
                    }
                    else
                    {
                        epoch_conversion_errors++;
                        Serial.printf("[CMDPARSER]  Epoch conversion failed (errors: %lu)\n",
                                      epoch_conversion_errors);
                    }

                    /* Message parsed successfully */
                    module_state = CMDPARSER_STATE_DATA_READY;

                    /*  Blink RED LED for new RS232 data */
                    LED_Red_RS232_Received();
                    Serial.println("[CMDPARSER]  New RS232 data received - Red LED blink");

                    new_data_available = true;
                    last_valid_data_time = current_time;
                    total_messages_received++;
                    consecutive_errors = 0U;
                    UpdateErrorStatus(CMDPARSER_ERROR_NONE);

                    /* Save raw buffer for debugging */
                    memcpy(last_complete_buffer, rx_buffer, bytes_received);
                    last_complete_length = bytes_received;
                    raw_buffer_available = true;
                }
                else
                {
                    /* Parsing error */
                    module_state = CMDPARSER_STATE_ERROR;
                    total_errors_detected++;
                    consecutive_errors++;
                    UpdateErrorStatus(parse_result);
                }
            }
            else
            {
                /* Timeout or incomplete message */
                module_state = CMDPARSER_STATE_ERROR;
                total_errors_detected++;
                consecutive_errors++;

                if (bytes_received == 0U)
                {
                    UpdateErrorStatus(CMDPARSER_ERROR_INSUFFICIENT_BYTES);
                }
                else
                {
                    UpdateErrorStatus(CMDPARSER_ERROR_INVALID_DATA);
                }
            }

            /* Reset for next message */
            ResetMessageState();

            xSemaphoreGive(data_mutex);
        }
    }
}

/**
 * @brief Get net weight value
 */
CmdParser_StatusType_t CmdParser__GetNetWeight(float *net_weight)
{
    CmdParser_StatusType_t status;

    if (!IsValidPointer(net_weight))
    {
        return CMDPARSER_STATUS_INVALID_PARAM;
    }

    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        if (!IsDataFresh())
        {
            status = CMDPARSER_STATUS_DATA_STALE;
        }
        else if (!current_weights.valid)
        {
            status = CMDPARSER_STATUS_NO_DATA;
        }
        else
        {
            *net_weight = current_weights.net_weight;
            status = CMDPARSER_STATUS_OK;
        }
        xSemaphoreGive(data_mutex);
    }
    else
    {
        status = CMDPARSER_STATUS_BUSY;
    }

    return status;
}

/**
 * @brief Get gross weight value
 */
CmdParser_StatusType_t CmdParser__GetGrossWeight(float *gross_weight)
{
    CmdParser_StatusType_t status;

    if (!IsValidPointer(gross_weight))
    {
        return CMDPARSER_STATUS_INVALID_PARAM;
    }

    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        if (!IsDataFresh())
        {
            status = CMDPARSER_STATUS_DATA_STALE;
        }
        else if (!current_weights.valid)
        {
            status = CMDPARSER_STATUS_NO_DATA;
        }
        else
        {
            *gross_weight = current_weights.gross_weight;
            status = CMDPARSER_STATUS_OK;
        }
        xSemaphoreGive(data_mutex);
    }
    else
    {
        status = CMDPARSER_STATUS_BUSY;
    }

    return status;
}

/**
 * @brief Get tare weight value
 */
CmdParser_StatusType_t CmdParser__GetTareWeight(float *tare_weight)
{
    CmdParser_StatusType_t status;

    if (!IsValidPointer(tare_weight))
    {
        return CMDPARSER_STATUS_INVALID_PARAM;
    }

    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        if (!IsDataFresh())
        {
            status = CMDPARSER_STATUS_DATA_STALE;
        }
        else if (!current_weights.valid)
        {
            status = CMDPARSER_STATUS_NO_DATA;
        }
        else
        {
            *tare_weight = current_weights.tare_weight;
            status = CMDPARSER_STATUS_OK;
        }
        xSemaphoreGive(data_mutex);
    }
    else
    {
        status = CMDPARSER_STATUS_BUSY;
    }

    return status;
}

/**
 * @brief Get total number of messages received
 */
CmdParser_StatusType_t CmdParser__GetTotalMessages(uint32_t *count)
{
    if (!IsValidPointer(count))
    {
        return CMDPARSER_STATUS_INVALID_PARAM;
    }

    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        *count = total_messages_received;
        xSemaphoreGive(data_mutex);
        return CMDPARSER_STATUS_OK;
    }

    return CMDPARSER_STATUS_BUSY;
}

/**
 * @brief  NEW: Get epoch time from last parsed timestamp
 */
CmdParser_StatusType_t CmdParser__GetEpochTime(uint32_t *epoch_seconds)
{
    CmdParser_StatusType_t status;

    if (!IsValidPointer(epoch_seconds))
    {
        return CMDPARSER_STATUS_INVALID_PARAM;
    }

    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        if (!epoch_valid)
        {
            status = CMDPARSER_STATUS_NO_DATA;
        }
        else if (!IsDataFresh())
        {
            status = CMDPARSER_STATUS_DATA_STALE;
        }
        else
        {
            *epoch_seconds = current_epoch_seconds;
            status = CMDPARSER_STATUS_OK;
        }
        xSemaphoreGive(data_mutex);
    }
    else
    {
        status = CMDPARSER_STATUS_BUSY;
    }

    return status;
}

/**
 * @brief Get timestamp information
 */
CmdParser_StatusType_t CmdParser__GetTimestamp(CmdParser_Timestamp_t *timestamp)
{
    CmdParser_StatusType_t status;

    if (!IsValidPointer(timestamp))
    {
        return CMDPARSER_STATUS_INVALID_PARAM;
    }

    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        if (!IsDataFresh())
        {
            status = CMDPARSER_STATUS_DATA_STALE;
        }
        else if (!current_timestamp.valid)
        {
            status = CMDPARSER_STATUS_NO_DATA;
        }
        else
        {
            *timestamp = current_timestamp;
            status = CMDPARSER_STATUS_OK;
        }
        xSemaphoreGive(data_mutex);
    }
    else
    {
        status = CMDPARSER_STATUS_BUSY;
    }

    return status;
}

/**
 * @brief Get data resolution information
 */
CmdParser_StatusType_t CmdParser__GetDataResolution(CmdParser_DataResolution_t *resolution)
{
    if (!IsValidPointer(resolution))
    {
        return CMDPARSER_STATUS_INVALID_PARAM;
    }

    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    *resolution = resolution_info;
    return CMDPARSER_STATUS_OK;
}

/**
 * @brief Get scale status information
 */
CmdParser_StatusType_t CmdParser__GetScaleStatus(CmdParser_ScaleStatus_t *status)
{
    CmdParser_StatusType_t result;

    if (!IsValidPointer(status))
    {
        return CMDPARSER_STATUS_INVALID_PARAM;
    }

    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        if (!IsDataFresh())
        {
            result = CMDPARSER_STATUS_DATA_STALE;
        }
        else if (!current_status.valid)
        {
            result = CMDPARSER_STATUS_NO_DATA;
        }
        else
        {
            *status = current_status;
            result = CMDPARSER_STATUS_OK;
        }
        xSemaphoreGive(data_mutex);
    }
    else
    {
        result = CMDPARSER_STATUS_BUSY;
    }

    return result;
}

/**
 * @brief Get current error status
 */
CmdParser_StatusType_t CmdParser__GetErrorStatus(CmdParser_ErrorType_t *error_type)
{
    if (!IsValidPointer(error_type))
    {
        return CMDPARSER_STATUS_INVALID_PARAM;
    }

    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    *error_type = current_error;
    return CMDPARSER_STATUS_OK;
}

/**
 * @brief Clear error status
 */
CmdParser_StatusType_t CmdParser__ClearErrorStatus(void)
{
    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        current_error = CMDPARSER_ERROR_NONE;
        consecutive_errors = 0U;
        xSemaphoreGive(data_mutex);
    }

    return CMDPARSER_STATUS_OK;
}

/**
 * @brief Get module initialization status
 */
bool CmdParser__IsInitialized(void)
{
    return module_initialized;
}

/**
 * @brief Get data freshness status
 */
bool CmdParser__IsDataFresh(void)
{
    bool fresh;

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        fresh = IsDataFresh() && new_data_available;
        xSemaphoreGive(data_mutex);
    }
    else
    {
        fresh = false;
    }

    return fresh;
}

/**
 * @brief Reset module
 */
CmdParser_StatusType_t CmdParser__Reset(void)
{
    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        module_state = CMDPARSER_STATE_INITIALIZED;
        current_error = CMDPARSER_ERROR_NONE;

        ResetMessageState();

        // Clear UART buffer
        while (UART_PORT.available() > 0)
        {
            UART_PORT.read();
        }

        memset(&current_weights, 0, sizeof(current_weights));
        memset(&current_timestamp, 0, sizeof(current_timestamp));
        memset(&current_status, 0, sizeof(current_status));
        memset(last_complete_buffer, 0, sizeof(last_complete_buffer));

        last_complete_length = 0U;
        raw_buffer_available = false;
        consecutive_errors = 0U;
        last_valid_data_time = 0U;
        new_data_available = false;

        /*  NEW: Reset epoch variables */
        current_epoch_seconds = 0U;
        epoch_valid = false;

        xSemaphoreGive(data_mutex);
    }

    return CMDPARSER_STATUS_OK;
}

/**
 * @brief Get raw received buffer
 */
CmdParser_StatusType_t CmdParser__GetRawBuffer(uint8_t *buffer, uint16_t *buffer_length, uint16_t max_length)
{
    CmdParser_StatusType_t status;

    if (!IsValidPointer(buffer) || !IsValidPointer(buffer_length))
    {
        return CMDPARSER_STATUS_INVALID_PARAM;
    }

    if (!module_initialized)
    {
        return CMDPARSER_STATUS_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        if (!raw_buffer_available || (last_complete_length == 0U))
        {
            status = CMDPARSER_STATUS_NO_DATA;
        }
        else
        {
            uint16_t copy_length = (last_complete_length < max_length) ? last_complete_length : max_length;
            memcpy(buffer, last_complete_buffer, copy_length);
            *buffer_length = copy_length;
            status = CMDPARSER_STATUS_OK;
        }
        xSemaphoreGive(data_mutex);
    }
    else
    {
        status = CMDPARSER_STATUS_BUSY;
    }

    return status;
}

/*==============================================================================
 *                          STATIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

static bool IsValidPointer(const void *ptr)
{
    return (ptr != NULL);
}

static void EnterCriticalSection(void)
{
    taskENTER_CRITICAL(&critical_mux);
}

static void ExitCriticalSection(void)
{
    taskEXIT_CRITICAL(&critical_mux);
}

static void ResetMessageState(void)
{
    taskENTER_CRITICAL(&critical_mux);
    bytes_received = 0U;
    message_complete = false;
    message_start_time = 0U;
    last_char_time = 0U;
    memset(rx_buffer, 0, sizeof(rx_buffer));
    taskEXIT_CRITICAL(&critical_mux);
}

static void UpdateErrorStatus(CmdParser_ErrorType_t error_type)
{
    current_error = error_type;
}

static bool IsDataFresh(void)
{
    uint32_t current_time = millis();
    uint32_t elapsed_time;

    if (last_valid_data_time == 0U)
    {
        return false;
    }

    /* Handle timer overflow */
    if (current_time >= last_valid_data_time)
    {
        elapsed_time = current_time - last_valid_data_time;
    }
    else
    {
        elapsed_time = (0xFFFFFFFFU - last_valid_data_time) + current_time + 1U;
    }

    return (elapsed_time < DATA_FRESHNESS_TIMEOUT_MS);
}

static void TrimString(char *str)
{
    uint16_t start = 0U;
    uint16_t end;
    uint16_t i;

    if (!IsValidPointer(str))
    {
        return;
    }

    /* Find start of non-whitespace */
    while ((str[start] != '\0') && isspace((unsigned char)str[start]))
    {
        start++;
    }

    /* String is all whitespace */
    if (str[start] == '\0')
    {
        str[0] = '\0';
        return;
    }

    /* Find end of non-whitespace */
    end = strlen(str) - 1U;
    while ((end > start) && isspace((unsigned char)str[end]))
    {
        end--;
    }

    /* Shift string to beginning if needed */
    if (start > 0U)
    {
        for (i = 0U; i <= (end - start); i++)
        {
            str[i] = str[start + i];
        }
        str[i] = '\0';
    }
    else
    {
        str[end + 1U] = '\0';
    }
}

static bool StringToUint32(const char *str, uint32_t *value)
{
    char *endptr;
    unsigned long result;

    if (!IsValidPointer(str) || !IsValidPointer(value))
    {
        return false;
    }

    if (str[0] == '\0')
    {
        return false;
    }

    result = strtoul(str, &endptr, 10);

    if (endptr == str || *endptr != '\0')
    {
        return false;
    }

    *value = (uint32_t)result;
    return true;
}

static bool StringToFloat(const char *str, float *value)
{
    char *endptr;
    float result;

    if (!IsValidPointer(str) || !IsValidPointer(value))
    {
        return false;
    }

    if (str[0] == '\0')
    {
        return false;
        result = strtof(str, &endptr);
        if (endptr == str || *endptr != '\0')
        {
            return false;
        }
        *value = result;
        return true;
    }

    result = strtof(str, &endptr);

    if (endptr == str || *endptr != '\0')
    {
        return false;
    }

    *value = result;
    return true;
}

/**
 * @brief Split message into fields (thread-safe alternative to strtok)
 */
static uint8_t SplitFields(char *message_str, char fields[][MAX_FIELD_LENGTH], uint8_t max_fields)
{
    uint8_t field_count = 0U;
    uint16_t char_index = 0U;
    uint16_t field_char_index = 0U;
    char current_char;

    if (!IsValidPointer(message_str) || !IsValidPointer(fields))
    {
        return 0U;
    }

    /* Parse through message character by character */
    while ((message_str[char_index] != '\0') && (field_count < max_fields) && (char_index < SANSUI_MESSAGE_LENGTH))
    {
        current_char = message_str[char_index];

        if (current_char == ',')
        {
            /* End of field */
            fields[field_count][field_char_index] = '\0';
            TrimString(fields[field_count]);
            field_count++;
            field_char_index = 0U;
        }
        else if ((current_char != '\r') && (current_char != '\n'))
        {
            /* Add character to current field */
            if (field_char_index < (MAX_FIELD_LENGTH - 1U))
            {
                fields[field_count][field_char_index] = current_char;
                field_char_index++;
            }
        }

        char_index++;
    }

    /* Handle last field if any characters present */
    if (field_char_index > 0U)
    {
        fields[field_count][field_char_index] = '\0';
        TrimString(fields[field_count]);
        field_count++;
    }

    return field_count;
}

static CmdParser_ErrorType_t ParseWeight(const char *weight_str, float *weight_value)
{
    if (!IsValidPointer(weight_str) || !IsValidPointer(weight_value))
    {
        return CMDPARSER_ERROR_INVALID_DATA;
    }

    if (!StringToFloat(weight_str, weight_value))
    {
        return CMDPARSER_ERROR_INVALID_DATA;
    }

    /* Validate weight range */
    if ((*weight_value < (float)SANSUI_MIN_WEIGHT_VALUE) ||
        (*weight_value > (float)SANSUI_MAX_WEIGHT_VALUE))
    {
        return CMDPARSER_ERROR_INVALID_DATA;
    }

    return CMDPARSER_ERROR_NONE;
}

static CmdParser_ErrorType_t ValidateWeightCalculation(void)
{
    float calculated_net;
    float difference;

    calculated_net = current_weights.gross_weight - current_weights.tare_weight;
    difference = fabsf(calculated_net - current_weights.net_weight);

    if (difference > CMDPARSER_WEIGHT_TOLERANCE)
    {
        return CMDPARSER_ERROR_WEIGHT_CALCULATION;
    }

    return CMDPARSER_ERROR_NONE;
}

static CmdParser_ErrorType_t ParseTimestamp(const char *date_str, const char *time_str)
{
    uint8_t part_index;
    uint16_t char_index;
    uint16_t part_char_index;
    char current_char;
    uint32_t temp_value;
    char date_parts[3][6]; /* DD, MM, YYYY */
    char time_parts[3][4]; /* HH, MM, SS */
    uint8_t date_part_count = 0U;
    uint8_t time_part_count = 0U;

    if (!IsValidPointer(date_str) || !IsValidPointer(time_str))
    {
        return CMDPARSER_ERROR_INVALID_TIMESTAMP;
    }

    /* Parse date (DD/MM/YYYY) */
    strncpy(s_date_copy, date_str, sizeof(s_date_copy) - 1U);
    s_date_copy[sizeof(s_date_copy) - 1U] = '\0';

    /* Split date by '/' */
    char_index = 0U;
    part_char_index = 0U;
    part_index = 0U;

    while ((s_date_copy[char_index] != '\0') && (part_index < 3U))
    {
        current_char = s_date_copy[char_index];

        if (current_char == '/')
        {
            date_parts[part_index][part_char_index] = '\0';
            date_part_count++;
            part_index++;
            part_char_index = 0U;
        }
        else if (isdigit((unsigned char)current_char))
        {
            if (part_char_index < (sizeof(date_parts[0]) - 1U))
            {
                date_parts[part_index][part_char_index] = current_char;
                part_char_index++;
            }
        }

        char_index++;
    }

    /* Handle last date part */
    if (part_char_index > 0U)
    {
        date_parts[part_index][part_char_index] = '\0';
        date_part_count++;
    }

    /* Validate date parts count */
    if (date_part_count != 3U)
    {
        return CMDPARSER_ERROR_INVALID_DATE;
    }

    /* Parse day */
    if (!StringToUint32(date_parts[0], &temp_value) || (temp_value < 1U) || (temp_value > 31U))
    {
        return CMDPARSER_ERROR_INVALID_DATE;
    }
    current_timestamp.date.day = (uint8_t)temp_value;

    /* Parse month */
    if (!StringToUint32(date_parts[1], &temp_value) || (temp_value < 1U) || (temp_value > 12U))
    {
        return CMDPARSER_ERROR_INVALID_DATE;
    }
    current_timestamp.date.month = (uint8_t)temp_value;

    /* Parse year */
    if (!StringToUint32(date_parts[2], &temp_value) || (temp_value < 2000U) || (temp_value > 2100U))
    {
        return CMDPARSER_ERROR_INVALID_DATE;
    }
    current_timestamp.date.year = (uint16_t)temp_value;
    current_timestamp.date.valid = true;

    /* Parse time (HH:MM:SS) */
    strncpy(s_time_copy, time_str, sizeof(s_time_copy) - 1U);
    s_time_copy[sizeof(s_time_copy) - 1U] = '\0';

    /* Split time by ':' */
    char_index = 0U;
    part_char_index = 0U;
    part_index = 0U;

    while ((s_time_copy[char_index] != '\0') && (part_index < 3U))
    {
        current_char = s_time_copy[char_index];

        if (current_char == ':')
        {
            time_parts[part_index][part_char_index] = '\0';
            time_part_count++;
            part_index++;
            part_char_index = 0U;
        }
        else if (isdigit((unsigned char)current_char))
        {
            if (part_char_index < (sizeof(time_parts[0]) - 1U))
            {
                time_parts[part_index][part_char_index] = current_char;
                part_char_index++;
            }
        }

        char_index++;
    }

    /* Handle last time part */
    if (part_char_index > 0U)
    {
        time_parts[part_index][part_char_index] = '\0';
        time_part_count++;
    }

    /* Validate time parts count */
    if (time_part_count != 3U)
    {
        return CMDPARSER_ERROR_INVALID_TIME;
    }

    /* Parse hour */
    if (!StringToUint32(time_parts[0], &temp_value) || (temp_value > 23U))
    {
        return CMDPARSER_ERROR_INVALID_TIME;
    }
    current_timestamp.time.hour = (uint8_t)temp_value;

    /* Parse minute */
    if (!StringToUint32(time_parts[1], &temp_value) || (temp_value > 59U))
    {
        return CMDPARSER_ERROR_INVALID_TIME;
    }
    current_timestamp.time.minute = (uint8_t)temp_value;

    /* Parse second */
    if (!StringToUint32(time_parts[2], &temp_value) || (temp_value > 59U))
    {
        return CMDPARSER_ERROR_INVALID_TIME;
    }
    current_timestamp.time.second = (uint8_t)temp_value;
    current_timestamp.time.valid = true;

    return CMDPARSER_ERROR_NONE;
}

/**
 * @brief ✅ NEW: Convert parsed timestamp to Unix epoch time
 * @details Uses Modbus_DateTime helper function for conversion
 * @return CMDPARSER_ERROR_NONE on success, error code otherwise
 */
static CmdParser_ErrorType_t ConvertTimestampToEpoch(void)
{
    /* Validate timestamp is available */
    if (!current_timestamp.valid || !current_timestamp.date.valid || !current_timestamp.time.valid)
    {
        epoch_valid = false;
        return CMDPARSER_ERROR_INVALID_TIMESTAMP;
    }

    /* Create Modbus_DateTime structure */
    Modbus_DateTime_t datetime;
    datetime.year = current_timestamp.date.year;
    datetime.month = current_timestamp.date.month;
    datetime.day = current_timestamp.date.day;
    datetime.hour = current_timestamp.time.hour;
    datetime.minute = current_timestamp.time.minute;
    datetime.second = current_timestamp.time.second;

    Serial.printf("[DEBUG] Timestamp: %04d-%02d-%02d %02d:%02d:%02d\n",
                  datetime.year, datetime.month, datetime.day,
                  datetime.hour, datetime.minute, datetime.second);

    /* ✅ CRITICAL FIX: Use the CORRECT conversion function */
    uint32_t epoch_result = 0;
    Modbus_DateTime_Status_t dt_status;

    // Convert DateTime to Epoch using the helper library
    dt_status = Modbus_DateTime__ConvertToEpoch(&datetime, &epoch_result);

    /* Check for conversion error */
    if (dt_status != MODBUS_DATETIME_STATUS_OK || epoch_result == 0)
    {
        epoch_valid = false;
        Serial.printf("[CMDPARSER] Epoch conversion failed: status=%d\n", dt_status);
        return CMDPARSER_ERROR_INVALID_TIMESTAMP;
    }

    /* Store valid epoch time */
    current_epoch_seconds = epoch_result;
    epoch_valid = true;

    /* ✅ UPDATE MODBUS REGISTERS with the converted epoch time */
    Modbus_SetEpochTime(epoch_result);

    Serial.printf("[CMDPARSER] Epoch: %lu (%04d-%02d-%02d %02d:%02d:%02d)\n",
                  epoch_result,
                  datetime.year, datetime.month, datetime.day,
                  datetime.hour, datetime.minute, datetime.second);

    return CMDPARSER_ERROR_NONE;
}

static CmdParser_ErrorType_t ParseMessage(void)
{
    uint8_t field_count;
    CmdParser_ErrorType_t result = CMDPARSER_ERROR_NONE;
    uint32_t temp_u32;

    /* Ensure null termination */
    memcpy(s_message_str, rx_buffer, bytes_received);
    s_message_str[bytes_received] = '\0';

    /* Split message into fields */
    field_count = SplitFields(s_message_str, s_fields, SANSUI_EXPECTED_FIELD_COUNT);

    /* Validate field count */
    if (field_count < SANSUI_EXPECTED_FIELD_COUNT)
    {
        return CMDPARSER_ERROR_FIELD_COUNT_MISMATCH;
    }

    /* Parse timestamp */
    result = ParseTimestamp(s_fields[SANSUI_FIELD_DATE], s_fields[SANSUI_FIELD_TIME]);
    if (result != CMDPARSER_ERROR_NONE)
    {
        return result;
    }

    /* Parse weights */
    result = ParseWeight(s_fields[SANSUI_FIELD_GROSS_WEIGHT], &current_weights.gross_weight);
    if (result != CMDPARSER_ERROR_NONE)
    {
        return CMDPARSER_ERROR_INVALID_GROSS_WEIGHT;
    }

    result = ParseWeight(s_fields[SANSUI_FIELD_TARE_WEIGHT], &current_weights.tare_weight);
    if (result != CMDPARSER_ERROR_NONE)
    {
        return CMDPARSER_ERROR_INVALID_TARE_WEIGHT;
    }

    result = ParseWeight(s_fields[SANSUI_FIELD_NET_WEIGHT], &current_weights.net_weight);
    if (result != CMDPARSER_ERROR_NONE)
    {
        return CMDPARSER_ERROR_INVALID_NET_WEIGHT;
    }

/* Validate weight calculation if enabled */
#if (CMDPARSER_ENABLE_WEIGHT_VALIDATION == true)
    result = ValidateWeightCalculation();
    if (result != CMDPARSER_ERROR_NONE)
    {
        return result;
    }
#endif

    /* Parse status information */
    if (StringToUint32(s_fields[SANSUI_FIELD_STATUS_CODE], &temp_u32))
    {
        current_status.status_code = temp_u32;
    }
    else
    {
        current_status.status_code = 0U;
    }

    if (StringToUint32(s_fields[SANSUI_FIELD_MODE], &temp_u32))
    {
        current_status.mode = (uint8_t)temp_u32;
    }
    else
    {
        current_status.mode = 0U;
    }

    if (StringToUint32(s_fields[SANSUI_FIELD_FLAGS], &temp_u32))
    {
        current_status.flags = (uint8_t)temp_u32;
    }
    else
    {
        current_status.flags = 0U;
    }

    /* Parse range values */
    if (!StringToFloat(s_fields[SANSUI_FIELD_RANGE_MIN], &current_status.range_min))
    {
        current_status.range_min = 0.0f;
    }

    if (!StringToFloat(s_fields[SANSUI_FIELD_RANGE_MAX], &current_status.range_max))
    {
        current_status.range_max = 0.0f;
    }

    /* Copy sample information */
    size_t sample_len = strlen(s_fields[SANSUI_FIELD_SAMPLE_INFO]);
    if (sample_len < MAX_SAMPLE_INFO_LENGTH)
    {
        strcpy(current_status.sample_info, s_fields[SANSUI_FIELD_SAMPLE_INFO]);
    }
    else
    {
        strncpy(current_status.sample_info, s_fields[SANSUI_FIELD_SAMPLE_INFO], MAX_SAMPLE_INFO_LENGTH - 1U);
        current_status.sample_info[MAX_SAMPLE_INFO_LENGTH - 1U] = '\0';
    }

    /* Mark all data as valid */
    current_weights.valid = true;
    current_timestamp.valid = true;
    current_status.valid = true;
    current_timestamp.timestamp_ms = millis();

    return CMDPARSER_ERROR_NONE;
}