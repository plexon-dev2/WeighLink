/**
 * @file Modbus.h
 * @brief Modbus RTU Slave Interface for ESP32-C3 with Weight Scale Data + Epoch Time
 * @details Implements Modbus RTU Slave with Function Code 3 (Read Holding Registers)
 *          Supports:
 *          - 1x Epoch Time (uint64_t, 64-bit, Little Endian) - 4 registers
 *          - 3x IEEE 754 32-bit floats (Little Endian) - 6 registers
 *          - 2x 16-bit status registers - 2 registers
 *
 * Configuration:
 * - Mode: Slave
 * - Protocol: Modbus RTU
 * - All configuration parameters are defined in modbus_cfg.h
 *
 * Register Mapping (Little Endian) - OPTION A: EPOCH FIRST:
 * - Reg 0-3:  Epoch Time (uint64_t, 64-bit) [Bits 0-15, 16-31, 32-47, 48-63]
 * - Reg 4-5:  Gross Weight (float, 32-bit) [Low, High]
 * - Reg 6-7:  Tare Weight (float, 32-bit) [Low, High]
 * - Reg 8-9:  Net Weight (float, 32-bit) [Low, High]
 * - Reg 10:   Data Valid Flag (uint16_t, 16-bit)
 * - Reg 11:   String Counter (uint16_t, 16-bit)
 *
 * PLC Poll Rate: Every 2 seconds
 *
 * @version 2.0.0 - Added Epoch Time support
 */

#ifndef MODBUS_H
#define MODBUS_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "modbus_cfg.h"      // Include configuration file
#include "Modbus_DateTime.h" // Include DateTime module

/* ============================================================================
 * DATA TYPES
 * ============================================================================ */

/**
 * @brief Modbus status enumeration
 */
typedef enum
{
    MODBUS_STATUS_OK = 0,                ///< Operation successful
    MODBUS_STATUS_INIT_FAILED,           ///< Initialization failed
    MODBUS_STATUS_INVALID_FRAME,         ///< Invalid frame format
    MODBUS_STATUS_CRC_ERROR,             ///< CRC check failed
    MODBUS_STATUS_INVALID_SLAVE_ID,      ///< Slave ID mismatch
    MODBUS_STATUS_INVALID_FUNCTION_CODE, ///< Unsupported function code
    MODBUS_STATUS_INVALID_ADDRESS,       ///< Register address out of range
    MODBUS_STATUS_TIMEOUT,               ///< Communication timeout
    MODBUS_STATUS_ERROR                  ///< General error
} Modbus_StatusType_t;

/**
 * @brief Weight data structure
 * @details Contains all weight measurements and status information
 */
typedef struct
{
    float gross_weight;      ///< Gross weight in kg
    float tare_weight;       ///< Tare weight in kg
    float net_weight;        ///< Net weight in kg (gross - tare)
    uint16_t data_valid;     ///< Data valid flag (0=Invalid, 1=Valid)
    uint16_t string_counter; ///< String counter (increments with each update)
} Modbus_WeightData_t;

/**
 * @brief Modbus communication statistics
 * @details Tracks performance and error metrics
 */
typedef struct
{
    uint32_t frames_received;    ///< Total frames received
    uint32_t frames_transmitted; ///< Total frames transmitted
    uint32_t crc_errors;         ///< CRC validation failures
    uint32_t timeout_errors;     ///< Timeout occurrences
    uint32_t invalid_requests;   ///< Invalid request count
    uint32_t successful_reads;   ///< Successful read operations
    uint32_t successful_writes;  ///< Successful write operations
} Modbus_Statistics_t;

/* ============================================================================
 * PUBLIC FUNCTION PROTOTYPES
 * ============================================================================ */

/**
 * @brief Initialize Modbus RTU Slave
 * @details Configures UART, RS485 control pin, and initializes registers to zero
 *
 * @note Must be called before any other Modbus functions
 * @note Clears UART buffer and resets all statistics
 */
void Modbus_Init(void);

/**
 * @brief Main Modbus handler - processes incoming requests
 * @details Handles frame reception, validation, and response generation
 *          Should be called periodically to ensure responsive communication
 *
 * @note Call this from Task_10ms or Task_50ms for optimal performance
 * @note Non-blocking - processes available data and returns
 */
void Modbus_Handler(void);

/* ============================================================================
 * REGISTER ACCESS FUNCTIONS (16-bit)
 * ============================================================================ */

/**
 * @brief Set holding register value (16-bit)
 * @param reg_addr Register address (0-11)
 * @param value Value to set (16-bit unsigned)
 * @return MODBUS_STATUS_OK on success, error code otherwise
 */
Modbus_StatusType_t Modbus_SetHoldingRegister(uint16_t reg_addr, uint16_t value);

/**
 * @brief Get holding register value (16-bit)
 * @param reg_addr Register address (0-11)
 * @param value Pointer to store the retrieved value
 * @return MODBUS_STATUS_OK on success, error code otherwise
 */
Modbus_StatusType_t Modbus_GetHoldingRegister(uint16_t reg_addr, uint16_t *value);

/* ============================================================================
 * EPOCH TIME ACCESS FUNCTIONS (64-bit)
 * ============================================================================ */

/**
 * @brief Set Epoch Time in Modbus Registers
 * @details Stores 64-bit epoch time across 4 registers (Little Endian)
 *
 * @param epoch__seconds Epoch time in seconds (uint32_t)
 * @return MODBUS_STATUS_OK on success, error code otherwise
 *
 * @note Stored in registers 0-3 (bits 0-15, 16-31, 32-47, 48-63)
 * @note Little Endian: Low word first
 */
Modbus_StatusType_t Modbus_SetEpochTime(uint32_t epoch__seconds);

/**
 * @brief Get Epoch Time from Modbus Registers
 * @details Reconstructs 64-bit epoch time from 4 registers (Little Endian)
 *
 * @param epoch__seconds Pointer to store epoch time
 * @return MODBUS_STATUS_OK on success, error code otherwise
 */
Modbus_StatusType_t Modbus_GetEpochTime(uint32_t *epoch__seconds);

/**
 * @brief Set Epoch Time from DateTime Structure
 * @details Converts DateTime to epoch and stores in registers
 *
 * @param datetime Pointer to DateTime structure
 * @return MODBUS_STATUS_OK on success, error code otherwise
 *
 * @note Uses Modbus_DateTime module for conversion
 */
Modbus_StatusType_t Modbus_SetEpochTimeFromDateTime(const Modbus_DateTime_t *datetime);

/**
 * @brief Get Epoch Time as DateTime Structure
 * @details Reads epoch from registers and converts to DateTime
 *
 * @param datetime Pointer to store DateTime
 * @return MODBUS_STATUS_OK on success, error code otherwise
 *
 * @note Uses Modbus_DateTime module for conversion
 */
Modbus_StatusType_t Modbus_GetEpochTimeAsDateTime(Modbus_DateTime_t *datetime);

/* ============================================================================
 * WEIGHT DATA ACCESS FUNCTIONS (32-bit Float)
 * ============================================================================ */

/**
 * @brief Set gross weight value (IEEE 754 32-bit float, Little Endian)
 * @param value Gross weight in kg
 * @return MODBUS_STATUS_OK on success, error code otherwise
 *
 * @note Stored in registers 4-5 (low word, high word)
 */
Modbus_StatusType_t Modbus_SetGrossWeight(float value);

/**
 * @brief Get gross weight value
 * @param value Pointer to store the gross weight in kg
 * @return MODBUS_STATUS_OK on success, error code otherwise
 */
Modbus_StatusType_t Modbus_GetGrossWeight(float *value);

/**
 * @brief Set tare weight value (IEEE 754 32-bit float, Little Endian)
 * @param value Tare weight in kg
 * @return MODBUS_STATUS_OK on success, error code otherwise
 *
 * @note Stored in registers 6-7 (low word, high word)
 */
Modbus_StatusType_t Modbus_SetTareWeight(float value);

/**
 * @brief Get tare weight value
 * @param value Pointer to store the tare weight in kg
 * @return MODBUS_STATUS_OK on success, error code otherwise
 */
Modbus_StatusType_t Modbus_GetTareWeight(float *value);

/**
 * @brief Set net weight value (IEEE 754 32-bit float, Little Endian)
 * @param value Net weight in kg
 * @return MODBUS_STATUS_OK on success, error code otherwise
 *
 * @note Stored in registers 8-9 (low word, high word)
 */
Modbus_StatusType_t Modbus_SetNetWeight(float value);

/**
 * @brief Get net weight value
 * @param value Pointer to store the net weight in kg
 * @return MODBUS_STATUS_OK on success, error code otherwise
 */
Modbus_StatusType_t Modbus_GetNetWeight(float *value);

/* ============================================================================
 * BULK DATA ACCESS FUNCTIONS
 * ============================================================================ */

/**
 * @brief Set all weight data at once
 * @param data Pointer to weight data structure containing all values
 * @return MODBUS_STATUS_OK on success, error code otherwise
 *
 * @note Updates gross weight, tare weight, net weight, data valid flag, and counter
 * @note More efficient than setting each value individually
 */
Modbus_StatusType_t Modbus_SetWeightData(Modbus_WeightData_t *data);

/**
 * @brief Get all weight data at once
 * @param data Pointer to structure where weight data will be stored
 * @return MODBUS_STATUS_OK on success, error code otherwise
 *
 * @note Retrieves gross weight, tare weight, net weight, data valid flag, and counter
 * @note More efficient than getting each value individually
 */
Modbus_StatusType_t Modbus_GetWeightData(Modbus_WeightData_t *data);

/* ============================================================================
 * STATUS REGISTER ACCESS FUNCTIONS
 * ============================================================================ */

/**
 * @brief Set data valid flag
 * @param valid 1 = Data is valid, 0 = Data is invalid
 * @return MODBUS_STATUS_OK on success, error code otherwise
 *
 * @note Stored in register 10
 */
Modbus_StatusType_t Modbus_SetDataValid(uint16_t valid);

/**
 * @brief Get data valid flag
 * @param valid Pointer to store the valid flag
 * @return MODBUS_STATUS_OK on success, error code otherwise
 */
Modbus_StatusType_t Modbus_GetDataValid(uint16_t *valid);

/**
 * @brief Set string counter
 * @param counter Counter value (increments with each data update)
 * @return MODBUS_STATUS_OK on success, error code otherwise
 *
 * @note Stored in register 11
 */
Modbus_StatusType_t Modbus_SetStringCounter(uint16_t counter);

/**
 * @brief Get string counter
 * @param counter Pointer to store the counter value
 * @return MODBUS_STATUS_OK on success, error code otherwise
 */
Modbus_StatusType_t Modbus_GetStringCounter(uint16_t *counter);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Check if Modbus is initialized and ready
 * @return true if initialized, false otherwise
 */
bool Modbus_IsInitialized(void);

/**
 * @brief Check if PLC has requested a counter reset
 * @details PLC writes 0x0001 to REG_COUNTER_RESET (register 12 / address 40013).
 *          Returns true as long as bit 0 is HIGH.
 *          ESP32 never clears this register — PLC owns it fully.
 *          When PLC writes 0x0000, this returns false and counting resumes from 0.
 *
 * @return true  Bit 0 is HIGH — reset counter to 0
 * @return false Bit 0 is LOW  — normal counting
 */
bool Modbus_IsCounterResetRequested(void);

/**
 * @brief Get Modbus communication statistics
 * @param stats Pointer to statistics structure to be filled
 *
 * @note Provides insight into communication health and performance
 */
void Modbus_GetStatistics(Modbus_Statistics_t *stats);

/**
 * @brief Reset all Modbus statistics counters to zero
 *
 * @note Useful for starting fresh monitoring sessions
 */
void Modbus_ResetStatistics(void);

/**
 * @brief Print Modbus statistics to Serial (for debugging)
 * @details Displays formatted table with frame counts and error statistics
 *
 * @note Requires Serial to be initialized
 */
void Modbus_PrintStatistics(void);

/**
 * @brief Print current weight data to Serial (for debugging)
 * @details Displays formatted table with all weight values, epoch time, and status
 *
 * @note Requires Serial to be initialized
 */
void Modbus_PrintWeightData(void);

/**
 * @brief Convert 32-bit epoch time to two 16-bit Modbus registers (Little Endian)
 * @details Splits 32-bit value into low word (bits 0-15) and high word (bits 16-31)
 * @param epoch Epoch time in seconds (32-bit unsigned)
 * @param registers Array to store 2 x 16-bit registers [low, high]
 */
void Modbus_EpochToRegisters_LE(uint32_t epoch, uint16_t registers[2]);

/**
 * @brief Reconstruct 32-bit epoch time from two 16-bit Modbus registers (Little Endian)
 * @details Combines low word and high word into 32-bit value
 * @param registers Array of 2 x 16-bit registers [low, high]
 * @return Reconstructed epoch time in seconds (32-bit unsigned)
 */
uint32_t Modbus_RegistersToEpoch_LE(const uint16_t registers[2]);

#endif /* MODBUS_H */