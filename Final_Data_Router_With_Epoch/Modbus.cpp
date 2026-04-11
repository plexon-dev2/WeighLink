/**
 * @file Modbus.cpp
 * @brief Modbus RTU Slave Implementation for Weight Scale - COMPLETE WITH YELLOW LED FIX
 * @details Implements Modbus RTU Slave with:
 *          - 3x IEEE 754 32-bit floats (Little Endian)
 *          - 2x 16-bit status registers
 *
 * Features:
 * - Automatic RS485 direction control
 * - CRC-16 validation
 * - Exception responses
 * - Statistics tracking
 * - Little Endian byte order (Low word first)
 * - Yellow LED: Blinks on every poll, solid 1s on valid data
 */

#include "Modbus.h"
#include "modbus_cfg.h"
#include "LED_HMI.h"
#include "Modbus_DateTime.h"
/* ============================================================================
 * PRIVATE VARIABLES
 * ============================================================================ */

static HardwareSerial ModbusSerial(MODBUS_UART_NUM);
static bool modbus_initialized = false;

// Holding registers storage - 8 registers
static uint16_t holding_registers[MODBUS_HOLDING_REG_COUNT] = {0};

// Receive buffer and state
static uint8_t rx_buffer[MODBUS_BUFFER_SIZE];
static uint16_t rx_index = 0;
static uint32_t last_rx_time = 0;

// Statistics
static Modbus_Statistics_t statistics = {0};

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ============================================================================ */

static uint16_t Modbus_CalculateCRC(uint8_t *data, uint16_t length);
static void Modbus_SetRS485Transmit(void);
static void Modbus_SetRS485Receive(void);
static void Modbus_ProcessFrame(uint8_t *frame, uint16_t length);
static void Modbus_SendResponse(uint8_t *response, uint16_t length);
static void Modbus_SendException(uint8_t function_code, uint8_t exception_code);
static void Modbus_HandleReadHoldingRegisters(uint8_t *frame, uint16_t length);
static void Modbus_FloatToRegisters_LE(float value, uint16_t *low_reg, uint16_t *high_reg);
static float Modbus_RegistersToFloat_LE(uint16_t low_reg, uint16_t high_reg);

/* ============================================================================
 * MODBUS EXCEPTION CODES
 * ============================================================================ */

#define MODBUS_EXCEPTION_ILLEGAL_FUNCTION 0x01
#define MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS 0x02
#define MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE 0x03
#define MODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE 0x04

/* ============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

void Modbus_Init(void)
{
    // Initialize UART
    ModbusSerial.begin(MODBUS_BAUD_RATE, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);

    // DEBUG: show Modbus UART config
    Serial.printf("[DBG] Modbus UART init: uart=%d baud=%lu rx=%d tx=%d de_re=%d\n",
                  (int)MODBUS_UART_NUM,
                  (unsigned long)MODBUS_BAUD_RATE,
                  (int)MODBUS_RX_PIN,
                  (int)MODBUS_TX_PIN,
                  (int)MODBUS_DE_RE_PIN);

    // Configure RS485 DE/RE pin
    pinMode(MODBUS_DE_RE_PIN, OUTPUT);
    Modbus_SetRS485Receive();

    // Initialize all registers to 0
    for (int i = 0; i < MODBUS_HOLDING_REG_COUNT; i++)
    {
        holding_registers[i] = 0;
    }

    // Reset statistics
    Modbus_ResetStatistics();

    // Clear UART buffer
    while (ModbusSerial.available())
    {
        ModbusSerial.read();
    }

    if (MODBUS_RX_PIN < 0 || MODBUS_TX_PIN < 0)
    {
        modbus_initialized = false;
        return;
    }

    rx_index = 0;
    last_rx_time = 0;

    modbus_initialized = true;
}

void Modbus_Handler(void)
{
    if (!modbus_initialized)
    {
        return;
    }

    uint32_t current_time = micros();

    // Read incoming bytes
    while (ModbusSerial.available())
    {
        if (rx_index < MODBUS_BUFFER_SIZE)
        {
            rx_buffer[rx_index++] = ModbusSerial.read();
            last_rx_time = current_time;
        }
        else
        {
            // Buffer overflow - reset
            rx_index = 0;
            while (ModbusSerial.available())
            {
                ModbusSerial.read();
            }
            break;
        }
    }

    // Check for frame completion (3.5 character delay)
    if (rx_index > 0)
    {
        if ((current_time - last_rx_time) >= MODBUS_FRAME_DELAY_US)
        {
            // Frame complete - process it
            Modbus_ProcessFrame(rx_buffer, rx_index);
            rx_index = 0;
        }
    }
}

Modbus_StatusType_t Modbus_SetHoldingRegister(uint16_t reg_addr, uint16_t value)
{
    if (!modbus_initialized)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    if (reg_addr >= MODBUS_HOLDING_REG_COUNT)
    {
        return MODBUS_STATUS_INVALID_ADDRESS;
    }

    holding_registers[reg_addr] = value;
    return MODBUS_STATUS_OK;
}

Modbus_StatusType_t Modbus_GetHoldingRegister(uint16_t reg_addr, uint16_t *value)
{
    if (!modbus_initialized)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    if (reg_addr >= MODBUS_HOLDING_REG_COUNT)
    {
        return MODBUS_STATUS_INVALID_ADDRESS;
    }

    if (value == NULL)
    {
        return MODBUS_STATUS_INVALID_ADDRESS;
    }

    *value = holding_registers[reg_addr];
    return MODBUS_STATUS_OK;
}

Modbus_StatusType_t Modbus_SetGrossWeight(float value)
{
    if (!modbus_initialized)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    uint16_t low_reg, high_reg;
    Modbus_FloatToRegisters_LE(value, &low_reg, &high_reg);

    holding_registers[REG_GROSS_WEIGHT_LOW] = low_reg;
    holding_registers[REG_GROSS_WEIGHT_HIGH] = high_reg;

    return MODBUS_STATUS_OK;
}

Modbus_StatusType_t Modbus_GetGrossWeight(float *value)
{
    if (!modbus_initialized || value == NULL)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    *value = Modbus_RegistersToFloat_LE(
        holding_registers[REG_GROSS_WEIGHT_LOW],
        holding_registers[REG_GROSS_WEIGHT_HIGH]);

    return MODBUS_STATUS_OK;
}

Modbus_StatusType_t Modbus_SetTareWeight(float value)
{
    if (!modbus_initialized)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    uint16_t low_reg, high_reg;
    Modbus_FloatToRegisters_LE(value, &low_reg, &high_reg);

    holding_registers[REG_TARE_WEIGHT_LOW] = low_reg;
    holding_registers[REG_TARE_WEIGHT_HIGH] = high_reg;

    return MODBUS_STATUS_OK;
}

Modbus_StatusType_t Modbus_GetTareWeight(float *value)
{
    if (!modbus_initialized || value == NULL)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    *value = Modbus_RegistersToFloat_LE(
        holding_registers[REG_TARE_WEIGHT_LOW],
        holding_registers[REG_TARE_WEIGHT_HIGH]);

    return MODBUS_STATUS_OK;
}

Modbus_StatusType_t Modbus_SetNetWeight(float value)
{
    if (!modbus_initialized)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    uint16_t low_reg, high_reg;
    Modbus_FloatToRegisters_LE(value, &low_reg, &high_reg);

    holding_registers[REG_NET_WEIGHT_LOW] = low_reg;
    holding_registers[REG_NET_WEIGHT_HIGH] = high_reg;

    return MODBUS_STATUS_OK;
}

Modbus_StatusType_t Modbus_GetNetWeight(float *value)
{
    if (!modbus_initialized || value == NULL)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    *value = Modbus_RegistersToFloat_LE(
        holding_registers[REG_NET_WEIGHT_LOW],
        holding_registers[REG_NET_WEIGHT_HIGH]);

    return MODBUS_STATUS_OK;
}

Modbus_StatusType_t Modbus_SetWeightData(Modbus_WeightData_t *data)
{
    if (!modbus_initialized || data == NULL)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    Modbus_SetGrossWeight(data->gross_weight);
    Modbus_SetTareWeight(data->tare_weight);
    Modbus_SetNetWeight(data->net_weight);
    Modbus_SetDataValid(data->data_valid);
    Modbus_SetStringCounter(data->string_counter);

    return MODBUS_STATUS_OK;
}

Modbus_StatusType_t Modbus_GetWeightData(Modbus_WeightData_t *data)
{
    if (!modbus_initialized || data == NULL)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    Modbus_GetGrossWeight(&data->gross_weight);
    Modbus_GetTareWeight(&data->tare_weight);
    Modbus_GetNetWeight(&data->net_weight);
    Modbus_GetDataValid(&data->data_valid);
    Modbus_GetStringCounter(&data->string_counter);

    return MODBUS_STATUS_OK;
}

Modbus_StatusType_t Modbus_SetDataValid(uint16_t valid)
{
    if (!modbus_initialized)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    holding_registers[REG_DATA_VALID] = valid;
    return MODBUS_STATUS_OK;
}

/**
 * @brief Get data valid flag - WITH FLAG CLEARING FOR LED BEHAVIOR
 * @details When PLC reads this register and flag = 1:
 *          1. Return 1 to PLC
 *          2. Clear flag to 0 for next poll
 *          This ensures LED blinks on next poll
 */
Modbus_StatusType_t Modbus_GetDataValid(uint16_t *valid)
{
    if (!modbus_initialized || valid == NULL)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    // CRITICAL: Just return flag value, DON'T clear it
    // Flag will be cleared in Modbus_HandleReadHoldingRegisters()
    // when PLC actually reads the weight registers
    *valid = holding_registers[REG_DATA_VALID];

    return MODBUS_STATUS_OK;
}
Modbus_StatusType_t Modbus_SetStringCounter(uint16_t counter)
{
    if (!modbus_initialized)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    holding_registers[REG_STRING_COUNTER] = counter;
    return MODBUS_STATUS_OK;
}

Modbus_StatusType_t Modbus_GetStringCounter(uint16_t *counter)
{
    if (!modbus_initialized || counter == NULL)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    *counter = holding_registers[REG_STRING_COUNTER];
    return MODBUS_STATUS_OK;
}

bool Modbus_IsInitialized(void)
{
    return modbus_initialized;
}

void Modbus_GetStatistics(Modbus_Statistics_t *stats)
{
    if (stats != NULL)
    {
        memcpy(stats, &statistics, sizeof(Modbus_Statistics_t));
    }
}

void Modbus_ResetStatistics(void)
{
    memset(&statistics, 0, sizeof(Modbus_Statistics_t));
}

void Modbus_PrintStatistics(void)
{
    Serial.println("\nâ•”â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•—");
    Serial.println("â•‘      MODBUS RTU STATISTICS           â•‘");
    Serial.println("â• â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•£");
    Serial.printf("â•‘ Frames Received:    %8lu        â•‘\n", statistics.frames_received);
    Serial.printf("â•‘ Frames Transmitted: %8lu        â•‘\n", statistics.frames_transmitted);
    Serial.printf("â•‘ Successful Reads:   %8lu        â•‘\n", statistics.successful_reads);
    Serial.printf("â•‘ CRC Errors:         %8lu        â•‘\n", statistics.crc_errors);
    Serial.printf("â•‘ Invalid Requests:   %8lu        â•‘\n", statistics.invalid_requests);
    Serial.println("â•šâ•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•\n");
}

void Modbus_PrintWeightData(void)
{
    Modbus_WeightData_t data;
    Modbus_GetWeightData(&data);

    uint32_t epoch__seconds;
    if (Modbus_GetEpochTime(&epoch__seconds) == MODBUS_STATUS_OK)
    {
        Modbus_DateTime_t datetime;
        if (Modbus_GetEpochTimeAsDateTime(&datetime) == MODBUS_STATUS_OK)
        {
            Serial.printf("â•‘ Epoch Time:   %12lu sec    â•‘\n", epoch__seconds);
            Serial.printf("â•‘ Date/Time:    %04d-%02d-%02d %02d:%02d:%02d â•‘\n",
                          datetime.year, datetime.month, datetime.day,
                          datetime.hour, datetime.minute, datetime.second);
        }
    }

    Serial.println("\nâ•”â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•—");
    Serial.println("â•‘      MODBUS WEIGHT DATA              â•‘");
    Serial.println("â• â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•£");
    Serial.printf("â•‘ Gross Weight: %12.3f kg     â•‘\n", data.gross_weight);
    Serial.printf("â•‘ Tare Weight:  %12.3f kg     â•‘\n", data.tare_weight);
    Serial.printf("â•‘ Net Weight:   %12.3f kg     â•‘\n", data.net_weight);
    Serial.printf("â•‘ Data Valid:   %12d        â•‘\n", data.data_valid);
    Serial.printf("â•‘ Counter:      %12d        â•‘\n", data.string_counter);
    Serial.println("â•šâ•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•\n");
}

/**
 * @brief Set Epoch Time in Modbus Registers
 */
Modbus_StatusType_t Modbus_SetEpochTime(uint32_t epoch__seconds)
{
    if (!modbus_initialized)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    // Direct Little Endian conversion (no helper function)
    holding_registers[REG_EPOCH_TIME_0] = (uint16_t)(epoch__seconds & 0xFFFF);
    holding_registers[REG_EPOCH_TIME_1] = (uint16_t)((epoch__seconds >> 16) & 0xFFFF);

    Serial.printf("[MODBUS] Epoch time set: %lu seconds\n", epoch__seconds);
    Serial.printf("[MODBUS] Registers (LE): [%d]=0x%04X [%d]=0x%04X\n",
                  REG_EPOCH_TIME_0, holding_registers[REG_EPOCH_TIME_0],
                  REG_EPOCH_TIME_1, holding_registers[REG_EPOCH_TIME_1]);

    return MODBUS_STATUS_OK;
}

/**
 * @brief Get Epoch Time from Modbus Registers
 */
Modbus_StatusType_t Modbus_GetEpochTime(uint32_t *epoch__seconds)
{
    if (!modbus_initialized || epoch__seconds == NULL)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    // âœ… Direct Little Endian reconstruction (no helper function)
    *epoch__seconds = ((uint32_t)holding_registers[REG_EPOCH_TIME_1] << 16) |
                      ((uint32_t)holding_registers[REG_EPOCH_TIME_0]);

    return MODBUS_STATUS_OK;
}

/**
 * @brief Set Epoch Time from DateTime Structure
 */
Modbus_StatusType_t Modbus_SetEpochTimeFromDateTime(const Modbus_DateTime_t *datetime)
{
    if (!modbus_initialized || datetime == NULL)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    uint32_t epoch__seconds;
    Modbus_DateTime_Status_t dt_status;

    // Convert DateTime to epoch
    dt_status = Modbus_DateTime__ConvertToEpoch(datetime, &epoch__seconds);

    if (dt_status != MODBUS_DATETIME_STATUS_OK)
    {
        Serial.println("[MODBUS] ERROR: DateTime to Epoch conversion failed");
        return MODBUS_STATUS_ERROR;
    }

    // Store in registers
    return Modbus_SetEpochTime(epoch__seconds);
}

/**
 * @brief Get Epoch Time as DateTime Structure
 */
Modbus_StatusType_t Modbus_GetEpochTimeAsDateTime(Modbus_DateTime_t *datetime)
{
    if (!modbus_initialized || datetime == NULL)
    {
        return MODBUS_STATUS_INIT_FAILED;
    }

    uint32_t epoch__seconds;
    Modbus_StatusType_t mb_status;
    Modbus_DateTime_Status_t dt_status;

    // Get epoch from registers
    mb_status = Modbus_GetEpochTime(&epoch__seconds);

    if (mb_status != MODBUS_STATUS_OK)
    {
        return mb_status;
    }

    // Convert epoch to DateTime
    dt_status = Modbus_DateTime__ConvertFromEpoch(epoch__seconds, datetime);

    if (dt_status != MODBUS_DATETIME_STATUS_OK)
    {
        Serial.println("[MODBUS] ERROR: Epoch to DateTime conversion failed");
        return MODBUS_STATUS_ERROR;
    }

    return MODBUS_STATUS_OK;
}

/* ============================================================================
 * PRIVATE FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

static void Modbus_SetRS485Transmit(void)
{
    digitalWrite(MODBUS_DE_RE_PIN, HIGH);
    delayMicroseconds(10);
}

static void Modbus_SetRS485Receive(void)
{
    delayMicroseconds(10);
    digitalWrite(MODBUS_DE_RE_PIN, LOW);
}

static uint16_t Modbus_CalculateCRC(uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

static void Modbus_FloatToRegisters_LE(float value, uint16_t *low_reg, uint16_t *high_reg)
{
    // Convert float to IEEE 754 32-bit representation
    union
    {
        float f;
        uint32_t u32;
    } converter;

    converter.f = value;

    // Little Endian: Low word first, then high word
    *low_reg = converter.u32 & 0xFFFF;          // Low 16 bits
    *high_reg = (converter.u32 >> 16) & 0xFFFF; // High 16 bits
}

static float Modbus_RegistersToFloat_LE(uint16_t low_reg, uint16_t high_reg)
{
    // Combine two 16-bit registers into 32-bit IEEE 754 float (Little Endian)
    union
    {
        float f;
        uint32_t u32;
    } converter;

    converter.u32 = ((uint32_t)high_reg << 16) | low_reg;

    return converter.f;
}

/**
 * @brief Process received Modbus frame - WITH YELLOW LED LOGIC
 * @details Yellow LED behavior:
 *          - Blinks on EVERY valid frame (after CRC/ID check)
 *          - Goes solid 1s when data_valid flag = 1
 */
static void Modbus_ProcessFrame(uint8_t *frame, uint16_t length)
{
    static uint32_t poll_count = 0;
    poll_count++;

    statistics.frames_received++;

    // Debug every 10th poll to reduce spam
    if (poll_count % 10 == 1)
    {
        Serial.printf("[MODBUS] Frame #%lu received, length=%d\n", poll_count, length);
    }

    if (length < 5)
    {
        statistics.invalid_requests++;
        return;
    }

    // Check CRC
    uint16_t received_crc = (frame[length - 1] << 8) | frame[length - 2];
    uint16_t calculated_crc = Modbus_CalculateCRC(frame, length - 2);

    if (received_crc != calculated_crc)
    {
        statistics.crc_errors++;
        return;
    }

    // Check slave ID
    if (frame[0] != MODBUS_SLAVE_ID)
    {
        statistics.invalid_requests++;
        return;
    }

    // Get function code
    uint8_t function_code = frame[1];

    // ============================================
    // YELLOW LED LOGIC - EXECUTES ON EVERY VALID FRAME
    // ============================================
    uint16_t data_valid_flag = holding_registers[REG_DATA_VALID];

    if (data_valid_flag == 1)
    {
        // Valid data exists â†’ solid LED for 1 second
        LED_Yellow_RS485_DataReceived();
        Serial.println("[MODBUS] Valid data - Yellow LED solid 1s");
    }
    else
    {
        // No valid data â†’ single blink for poll
        LED_Yellow_PLC_Poll();
    }

    // ============================================
    // Process function code (LED already handled above)
    // ============================================
    switch (function_code)
    {
    case MODBUS_FC_READ_HOLDING_REGS:
        Modbus_HandleReadHoldingRegisters(frame, length);
        break;

    default:
        Modbus_SendException(function_code, MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
        statistics.invalid_requests++;
        break;
    }
}

/**
 * @brief Handle Read Holding Registers request (Function Code 3)
 * @details NO LED LOGIC HERE - LED handled in ProcessFrame()
 */
static void Modbus_HandleReadHoldingRegisters(uint8_t *frame, uint16_t length)
{
    if (length != 8)
    {
        Modbus_SendException(MODBUS_FC_READ_HOLDING_REGS, MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE);
        return;
    }

    uint16_t start_addr = (frame[2] << 8) | frame[3];
    uint16_t reg_count = (frame[4] << 8) | frame[5];

    if (start_addr >= MODBUS_HOLDING_REG_COUNT ||
        (start_addr + reg_count) > MODBUS_HOLDING_REG_COUNT ||
        reg_count == 0 || reg_count > 125)
    {
        Modbus_SendException(MODBUS_FC_READ_HOLDING_REGS, MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS);
        return;
    }

    // Check data validity
    uint16_t data_valid = holding_registers[REG_DATA_VALID];

    // Build response
    uint8_t response[MODBUS_BUFFER_SIZE];
    uint8_t idx = 0;

    response[idx++] = MODBUS_SLAVE_ID;
    response[idx++] = MODBUS_FC_READ_HOLDING_REGS;
    response[idx++] = reg_count * 2;

    for (uint16_t i = 0; i < reg_count; i++)
    {
        uint16_t reg_value = holding_registers[start_addr + i];
        response[idx++] = (reg_value >> 8) & 0xFF;
        response[idx++] = reg_value & 0xFF;
    }

    uint16_t crc = Modbus_CalculateCRC(response, idx);
    response[idx++] = crc & 0xFF;
    response[idx++] = (crc >> 8) & 0xFF;

    Modbus_SendResponse(response, idx);
    statistics.successful_reads++;

    // ============================================
    // CRITICAL: Clear flag AFTER sending response to PLC
    // ============================================
    if (data_valid == 1)
    {
        // Check if PLC read the data_valid register or weight registers
        uint16_t req_end = start_addr + reg_count - 1;
        bool read_data_valid_reg = (start_addr <= REG_DATA_VALID && req_end >= REG_DATA_VALID);
        bool read_weight_regs = (start_addr <= REG_NET_WEIGHT_HIGH && req_end >= REG_GROSS_WEIGHT_LOW);

        if (read_data_valid_reg || read_weight_regs)
        {
            holding_registers[REG_DATA_VALID] = 0;
            Serial.println("[MODBUS] PLC read data â†’ flag cleared");
        }
    }
}

static void Modbus_SendResponse(uint8_t *response, uint16_t length)
{
    if (!modbus_initialized || response == NULL || length == 0)
    {
        return;
    }

    // Toggle RS485 to transmit mode
    Modbus_SetRS485Transmit();

    // Write response bytes (CRC already included in response buffer)
    ModbusSerial.write(response, length);
    ModbusSerial.flush();

    // Small delay to ensure transmission completes
    delayMicroseconds(100);

    // Switch back to receive mode
    Modbus_SetRS485Receive();

    // Update statistics
    statistics.frames_transmitted++;
}

static void Modbus_SendException(uint8_t function_code, uint8_t exception_code)
{
    if (!modbus_initialized)
    {
        return;
    }

    uint8_t response[5];
    uint8_t idx = 0;

    response[idx++] = MODBUS_SLAVE_ID;
    response[idx++] = function_code | 0x80; // Set MSB to indicate exception
    response[idx++] = exception_code;

    // Calculate CRC for exception response
    uint16_t crc = Modbus_CalculateCRC(response, idx);
    response[idx++] = crc & 0xFF;        // CRC low byte
    response[idx++] = (crc >> 8) & 0xFF; // CRC high byte

    // RS485 transmit
    Modbus_SetRS485Transmit();
    ModbusSerial.write(response, 5);
    ModbusSerial.flush();
    delayMicroseconds(100);
    Modbus_SetRS485Receive();

    // Update statistics
    statistics.frames_transmitted++;
    statistics.invalid_requests++;
}