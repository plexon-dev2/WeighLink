/**
 * @file modbus_cfg.h
 * @brief Modbus RTU Configuration File - WITH EPOCH TIME SUPPORT
 * @details Centralized configuration for Modbus RTU Slave implementation
 *          All user-configurable parameters are defined here
 *
 * @note Modify this file to customize Modbus settings for your application
 * @version 2.0.0 - Added Epoch Time support (4 registers)
 */

#ifndef MODBUS_CFG_H
#define MODBUS_CFG_H

/* ============================================================================
 * HARDWARE CONFIGURATION
 * ============================================================================ */

/**
 * @brief UART peripheral selection
 * @note ESP32-C3 supports UART0 and UART1
 *       UART0 is typically used for USB serial (programming/debug)
 *       UART1 is available for general use
 */
#define MODBUS_UART_NUM 0

/**
 * @brief UART baud rate
 * @note Common values: 9600, 19200, 38400, 57600, 115200
 *       Standard Modbus RTU rates: 9600, 19200
 */
#define MODBUS_BAUD_RATE 9600

/**
 * @brief GPIO pin assignments for RS485 communication
 */
#define MODBUS_RX_PIN       	20    	// UART RX pin (receive data)
#define MODBUS_TX_PIN 			  21    	// UART TX pin (transmit data)
#define MODBUS_DE_RE_PIN 		  10 		// RS485 direction control (DE/RE)

/* ============================================================================
 * PROTOCOL CONFIGURATION
 * ============================================================================ */

/**
 * @brief Modbus slave address
 * @note Valid range: 1-247
 *       0 = Broadcast (not used in this implementation)
 *       248-255 = Reserved
 */
#define MODBUS_SLAVE_ID 			1

/**
 * @brief Maximum Modbus frame size in bytes
 * @note Standard Modbus maximum is 256 bytes
 */
#define MODBUS_BUFFER_SIZE 			256
	
/**
 * @brief Response timeout in milliseconds
 * @note Time to wait for complete frame reception
 */
#define MODBUS_TIMEOUT_MS 			100

/**
 * @brief Inter-frame delay in microseconds (3.5 character times)
 * @note Calculated as: (1 / baud_rate) * 11 bits * 3.5 characters * 1,000,000
 *       For 9600 baud: (1/9600) * 11 * 3.5 * 1000000 = 4010 Âµs
 *       Using 1750 Âµs for faster response (adjust if needed)
 */
#define MODBUS_FRAME_DELAY_US 		4010

/* ============================================================================
 * REGISTER CONFIGURATION - OPTION A: EPOCH TIME FIRST
 * ============================================================================ */

/**
 * @brief Total number of holding registers
 * @note 12 registers total:
 *       - 4 registers for Epoch Time (uint64_t, 64-bit)
 *       - 6 registers for 3x 32-bit floats (2 registers each)
 *       - 2 registers for status values (16-bit each)
 */
#define MODBUS_HOLDING_REG_COUNT 	13

// Epoch Time Registers (4 registers for uint64_t, Little Endian)
#define REG_EPOCH_TIME_0 			0 		// 40001 - Epoch Bits 0-15 (LSW)
#define REG_EPOCH_TIME_1 			1 		// 40002 - Epoch Bits 16-31
#define REG_EPOCH_TIME_2 			0 		// 40003 - Epoch Bits 32-47
#define REG_EPOCH_TIME_3 			0 		// 40004 - Epoch Bits 48-63 (MSW)

// Weight Registers (float values, Little Endian: Low word first, High word second)
#define REG_GROSS_WEIGHT_LOW 		4  		// 40005 - Gross Weight Low Word
#define REG_GROSS_WEIGHT_HIGH 	5 		// 40006 - Gross Weight High Word
#define REG_TARE_WEIGHT_LOW 		6   	// 40007 - Tare Weight Low Word
#define REG_TARE_WEIGHT_HIGH 		7  		// 40008 - Tare Weight High Word
#define REG_NET_WEIGHT_LOW 			8    	// 40009 - Net Weight Low Word
#define REG_NET_WEIGHT_HIGH 		9   	// 40010 - Net Weight High Word

// Status Registers
#define REG_DATA_VALID 				  10      // 40011 - Data Valid Flag (0=Invalid, 1=Valid)
#define REG_STRING_COUNTER 			11 		// 40012 - String Counter

// Control Registers (writable by PLC via FC06)
#define REG_COUNTER_RESET           12      // 40013 - PLC writes 0x0001 to reset string counter

/* ============================================================================
 * FUNCTION CODE SUPPORT
 * ============================================================================ */

/**
 * @brief Supported Modbus function codes
 * @note Currently only Function Code 3 (Read Holding Registers) is implemented
 *       Add more function codes here as needed
 */
#define MODBUS_FC_READ_HOLDING_REGS 				0x03 	// Read Holding Registers
#define MODBUS_FC_WRITE_SINGLE_REG  				0x06    // Write Single Register (PLC counter reset)
#define MODBUS_FC_WRITE_MULTIPLE_REGS               0x10    // Write Multiple Registers (ModbusView)

/* ============================================================================
 * EXCEPTION CODES
 * ============================================================================ */

/**
 * @brief Modbus exception codes (error responses)
 */
#define MODBUS_EXCEPTION_ILLEGAL_FUNCTION 			  0x01     // Function code not supported
#define MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS 		0x02 	 // Register address out of range
#define MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE 		  0x03   	 // Invalid data value
#define MODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE 		0x04 	 // Slave device failure

/* ============================================================================
 * TIMING CONFIGURATION
 * ============================================================================ */

/**
 * @brief RS485 direction control timing
 * @note Delay in microseconds before/after transmission
 *       Adjust based on your RS485 transceiver specifications
 */
#define MODBUS_RS485_SWITCH_DELAY_US 	10

/* ============================================================================
 * FEATURE ENABLE/DISABLE
 * ============================================================================ */

/**
 * @brief Enable debug output via Serial
 * @note Set to 1 to enable debug messages, 0 to disable
 */
#define MODBUS_DEBUG_ENABLE				 0

/**
 * @brief Enable statistics tracking
 * @note Set to 1 to track frame counts, errors, etc.
 */
#define MODBUS_STATISTICS_ENABLE 		1

/**
 * @brief Enable CRC validation
 * @note Should always be enabled for proper Modbus operation
 */
#define MODBUS_CRC_VALIDATION_ENABLE 	1

/* ============================================================================
 * DATA FORMAT CONFIGURATION
 * ============================================================================ */

/**
 * @brief Float byte order (endianness)
 * @note 0 = Big Endian (ABCD) - High word first
 *       1 = Little Endian (DCBA) - Low word first (default)
 *       2 = Big Endian Byte Swap (BADC)
 *       3 = Little Endian Byte Swap (CDAB)
 */
#define MODBUS_FLOAT_BYTE_ORDER 	   1 		// Little Endian (Low word first)

/**
 * @brief Epoch time byte order (endianness)
 * @note Must match float byte order for consistency
 *       1 = Little Endian (Low word first) - DCBA format
 */
#define MODBUS_EPOCH_BYTE_ORDER 	   1        // Little Endian (Low word first)

/* ============================================================================
 * VALIDATION MACROS
 * ============================================================================ */

// Compile-time checks for valid configuration
#if (MODBUS_SLAVE_ID < 1) || (MODBUS_SLAVE_ID > 247)
#error "MODBUS_SLAVE_ID must be between 1 and 247"
#endif

#if (MODBUS_BAUD_RATE < 1200) || (MODBUS_BAUD_RATE > 115200)
#warning "MODBUS_BAUD_RATE outside typical range (1200-115200)"
#endif

#if (MODBUS_HOLDING_REG_COUNT < 1) || (MODBUS_HOLDING_REG_COUNT > 125)
#error "MODBUS_HOLDING_REG_COUNT must be between 1 and 125"
#endif

#if (MODBUS_BUFFER_SIZE < 8) || (MODBUS_BUFFER_SIZE > 512)
#error "MODBUS_BUFFER_SIZE must be between 8 and 512 bytes"
#endif

/* ============================================================================
 * HELPER MACROS
 * ============================================================================ */

/**
 * @brief Calculate 3.5 character delay for given baud rate
 * @note Use this macro to calculate MODBUS_FRAME_DELAY_US for different baud rates
 */
#define MODBUS_CALC_FRAME_DELAY(baud) ((1000000UL * 11 * 35) / ((baud) * 10))

/**
 * @brief Debug print macro (only active if debug is enabled)
 */
#if MODBUS_DEBUG_ENABLE
#define MODBUS_DEBUG_PRINT(...) Serial.printf(__VA_ARGS__)
#else
#define MODBUS_DEBUG_PRINT(...) ((void)0)
#endif

#endif /* MODBUS_CFG_H */