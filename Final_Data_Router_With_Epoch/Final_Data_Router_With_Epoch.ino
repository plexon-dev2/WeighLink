/**
 * @file Main.ino
 * @brief Arduino Entry Point for FreeRTOS Scheduler
 * @details This file contains setup() and loop() functions
 *          Integrates weight scale with Modbus and LED HMI system
 *
 * ESP32-C3 FreeRTOS Preemptive Scheduler
 * Version: 4.1.0 - With LED_HMI Handler Support + System Commands
 */

#include "Scheduler.h"
#include "Scheduler_Cfg.h"
#include "Modbus.h"
#include "LED_HMI.h"
#include "SystemStates.h"

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================ */
void HandleSerialCommands(void);

/* ============================================================================
 * ARDUINO SETUP (RUNS ONCE)
 * ============================================================================ */

void setup()
{
    // CRITICAL: Initialize Serial FIRST, before anything else!
    Serial.begin(115200);
    delay(3000); // Wait for Serial to be ready

    Serial.println("BOOT OK");
    Serial.println();
    Serial.println("===============================================");
    Serial.println("ESP32-C3 Weight Scale + Modbus System");
    Serial.println("Version 4.1 - LED HMI with Handler + Commands");
    Serial.println("===============================================");
    Serial.print("Free heap at start: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println();

    // Initialize modules
    Serial.println("[SETUP] Initializing modules...");

    // Initialize other modules
    Scheduler_ExecuteInit();

    Serial.println("[SETUP] Checking module status...");
    if (Modbus_IsInitialized())
    {
        Serial.println("[SETUP] ✓ Modbus initialized");

        Serial.printf("[MODBUS] Slave ID: %d\n", MODBUS_SLAVE_ID);
        Serial.printf("[MODBUS] Baud Rate: %d\n", MODBUS_BAUD_RATE);
        Serial.printf("[MODBUS] RX Pin: GPIO %d\n", MODBUS_RX_PIN);
        Serial.printf("[MODBUS] TX Pin: GPIO %d\n", MODBUS_TX_PIN);
        Serial.printf("[MODBUS] DE/RE Pin: GPIO %d\n", MODBUS_DE_RE_PIN);
        Serial.printf("[MODBUS] Total Registers: %d\n", MODBUS_HOLDING_REG_COUNT);

        // Test YELLOW LED - solid ON
        LED_Yellow_RS485_DataReceived();
        delay(1500);
    }
    else
    {
        Serial.println("[SETUP] ✗ Modbus init failed!");
        // RED LED will blink rapidly via error indication
        for (int i = 0; i < 5; i++)
        {
            LED_Red_RS232_Received();
            delay(300);
        }
    }

    // Create FreeRTOS tasks
    Serial.println("[SETUP] Creating FreeRTOS tasks...");

    if (!Scheduler_Init())
    {
        Serial.println("[ERROR] Task creation failed!");
        // Continuous RED blink on error
        while (1)
        {
            LED_Red_RS232_Received();
            delay(200);
        }
    }

    Serial.println("[SETUP] ✓ All tasks created");

    // Start scheduler
    Scheduler_Start();

    Serial.print("[SETUP] Free heap after tasks: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("[SETUP] *** SYSTEM READY ***");
    Serial.println("===============================================");
    Serial.println();
    Serial.println("[INFO] Type HELP for available commands");
    Serial.println();
}

/* ============================================================================
 * ARDUINO LOOP (RUNS CONTINUOUSLY)
 * ============================================================================ */

void loop(void)
{
    // Handle user commands from Serial
    HandleSerialCommands();

    // Print status every 30 seconds
    static uint32_t last_print = 0;
    uint32_t current_time = millis();

    if ((current_time - last_print) >= 30000)
    {
#if SCHEDULER_CFG_SERIAL_DEBUG
        Serial.println("[LOOP] Arduino loop() is still running");
        Serial.print("[LOOP] Free heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");
#endif
        last_print = current_time;
    }

    // Small delay to prevent watchdog issues
    delay(10);
}

/* ============================================================================
 * SYSTEM COMMAND HANDLER
 * ============================================================================ */

/**
 * @brief Handle USB Serial commands for system control
 * @details Processes user commands like RESET, STATUS, HELP, etc.
 */
void HandleSerialCommands(void)
{
    if (Serial.available())
    {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toUpperCase(); // Convert to uppercase

        if (command == "RESET")
        {
            Serial.println();
            Serial.println("═══════════════════════════════════════════");
            Serial.println("       MANUAL SYSTEM RESET REQUESTED       ");
            Serial.println("═══════════════════════════════════════════");
            SystemStates__ForceReset();
            Serial.println("═══════════════════════════════════════════");
            Serial.println();
        }
        else if (command == "STATUS")
        {
            Serial.println();
            Serial.println("═══════════════════════════════════════════");
            Serial.println("           SYSTEM STATUS REPORT            ");
            Serial.println("═══════════════════════════════════════════");
            Serial.printf("  Current State    : %d\n", SystemStates__GetState());
            Serial.printf("  Total Errors     : %lu\n", SystemStates__GetErrorCount());
            Serial.printf("  Last Error Code  : %d\n", SystemStates__GetLastError());
            Serial.printf("  Free Heap        : %lu bytes\n", (unsigned long)ESP.getFreeHeap());
            Serial.printf("  Uptime           : %lu ms\n", millis());
            Serial.println("═══════════════════════════════════════════");
            Serial.println();
        }
        else if (command == "CLEAR")
        {
            Serial.println();
            Serial.println("[SYSTEM] Clearing all errors...");
            SystemStates__ClearErrors();
            Serial.println("[SYSTEM] ✓ Errors cleared successfully");
            Serial.println();
        }
        else if (command == "HELP")
        {
            Serial.println();
            Serial.println("═══════════════════════════════════════════");
            Serial.println("         AVAILABLE SYSTEM COMMANDS         ");
            Serial.println("═══════════════════════════════════════════");
            Serial.println("  RESET   - Force system reset from failure");
            Serial.println("            Use when in LOCKOUT mode");
            Serial.println();
            Serial.println("  STATUS  - Display current system status");
            Serial.println("            Shows state, errors, memory");
            Serial.println();
            Serial.println("  CLEAR   - Clear error counters");
            Serial.println("            Resets error statistics");
            Serial.println();
            Serial.println("  HELP    - Display this help message");
            Serial.println("            Shows all available commands");
            Serial.println("═══════════════════════════════════════════");
            Serial.println();
        }
        else if (command.length() > 0)
        {
            Serial.println();
            Serial.println("[SYSTEM] ✗ Unknown command: " + command);
            Serial.println("[SYSTEM] Type HELP for available commands");
            Serial.println();
        }
    }
}