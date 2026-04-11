/**
 * @file SystemStates.cpp
 * @brief System State Machine Implementation - PRODUCTION VERSION
 * @details Manages overall system state and health monitoring
 *
 * @version 2.5.0 - PRODUCTION READY with proper init guard and self-recovery
 *
 * FIXES APPLIED:
 * - BUG 1: initStartTime uses bool flag
 * - BUG 2: UINT32_MAX replaced with dedicated bool
 * - BUG 3: ClearErrors() doesn't reset LastValidDataTime
 * - BUG 4: Init guard WITH SELF-RECOVERY (production safe)
 * - BUG 5: recoveryAttemptCount increments after check
 * - BUG 6: DiagnosticStatus() startup grace period
 * - BUG 8: Parameter naming unified
 * - BUG 9: Data check in health interval only
 */

/*==============================================================================
 *                              INCLUDES
 *============================================================================*/
#include "SystemStates.h"
#include "SystemStates_Cfg.h"
#include "Modbus.h"
#include "CmdParser.h"
#include "LED_HMI.h"
#include "App.h"
#include "Arduino.h"

/*==============================================================================
 *                          PRIVATE VARIABLES
 *============================================================================*/

/* Current system state */
static SystemStates_State_t SystemStates_CurrentState = SYSTEMSTATES_INIT;

/* Error tracking */
static SystemStates_ErrorCode_t SystemStates_LastError       = SYSTEMSTATES_ERROR_NONE;
static uint32_t SystemStates_ErrorCount                      = 0U;
static uint32_t SystemStates_ConsecutiveErrors               = 0U;

/* Timing */
static uint32_t SystemStates_LastValidDataTime               = 0U;
static uint32_t SystemStates_HealthCheckCounter              = 0U;

/* Module initialization status */
static bool SystemStates_ModuleInitialized                   = false;

/* FIX BUG 6: Track whether first valid data has been received */
static bool SystemStates_InitialDataReceived                 = false;

/*==============================================================================
 *                          PRIVATE FUNCTION PROTOTYPES
 *============================================================================*/
static void SystemStates_EnterInit(void);
static void SystemStates_EnterNormal(void);
static void SystemStates_EnterFailure(void);
static void SystemStates_HandleInit(void);
static void SystemStates_HandleNormal(void);
static void SystemStates_HandleFailure(void);
static bool SystemStates_CheckCommunication(void);
static bool SystemStates_CheckPowerSupply(void);
static void SystemStates_UpdateErrorStatus(SystemStates_ErrorCode_t error);
static uint32_t SystemStates_GetTimeDifference(uint32_t startTime);

/*==============================================================================
 *                          PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/**
 * @brief Initialize System States Module
 * @details Initializes state machine and enters INIT state
 */
void SystemStates__Init(void)
{
    Serial.println("[SYSTEMSTATES] Init starting...");
    
    /* Initialize variables */
    SystemStates_CurrentState       = SYSTEMSTATES_INIT;
    SystemStates_LastError          = SYSTEMSTATES_ERROR_NONE;
    SystemStates_ErrorCount         = 0U;
    SystemStates_ConsecutiveErrors  = 0U;
    SystemStates_LastValidDataTime  = millis();
    SystemStates_HealthCheckCounter = 0U;
    SystemStates_InitialDataReceived = false;
    SystemStates_ModuleInitialized  = true;

    Serial.println("[SYSTEMSTATES] Module initialized");
    Serial.println("[SYSTEMSTATES] State: INIT");
}

/**
 * @brief Main System States Handler
 * @details Called periodically by FreeRTOS scheduler task
 * 
 * PRODUCTION VERSION: Includes init guard WITH self-recovery mechanism
 */
void SystemStates__Handler(void)
{
    /* PRODUCTION GUARD: Check initialization with self-recovery
     * 
     * In a properly configured system, Init() is always called before Handler()
     * via the scheduler's Init_Functions[] array. However, for production
     * robustness, we include this guard with automatic recovery.
     * 
     * If Handler is somehow called before Init (e.g., scheduler misconfiguration,
     * corruption, or race condition), we attempt self-recovery rather than
     * silently failing.
     */
    if (SystemStates_ModuleInitialized != true)
    {
        static uint8_t initRetryCount = 0;
        static uint32_t lastRetryTime = 0;
        uint32_t currentTime = millis();
        
        /* Attempt self-recovery with rate limiting (max 3 attempts, 1 per second) */
        if (initRetryCount < 3)
        {
            if ((currentTime - lastRetryTime) >= 1000U)
            {
                Serial.println("[SYSTEMSTATES] WARNING: Handler called before Init!");
                Serial.printf("[SYSTEMSTATES] Auto-recovery attempt %d/3\n", initRetryCount + 1);
                
                SystemStates__Init();
                
                initRetryCount++;
                lastRetryTime = currentTime;
            }
        }
        else
        {
            /* Recovery failed - log error and exit
             * This prevents infinite recovery attempts that could mask deeper issues */
            if ((currentTime - lastRetryTime) >= 5000U)
            {
                Serial.println("[SYSTEMSTATES] CRITICAL: Init recovery failed after 3 attempts");
                Serial.println("[SYSTEMSTATES] Check scheduler configuration!");
                lastRetryTime = currentTime;
            }
        }
        
        return;
    }

    /* Update health check counter */
    SystemStates_HealthCheckCounter++;

    /* Execute state-specific handler */
    switch (SystemStates_CurrentState)
    {
        case SYSTEMSTATES_INIT:
            SystemStates_HandleInit();
            break;

        case SYSTEMSTATES_NORMAL:
            SystemStates_HandleNormal();
            break;

        case SYSTEMSTATES_FAILURE:
            SystemStates_HandleFailure();
            break;

        default:
            Serial.println("[SYSTEMSTATES] ERROR: Invalid state!");
            SystemStates__SetState(SYSTEMSTATES_INIT);
            break;
    }
}

/**
 * @brief Get current system state
 * @return Current system state
 */
SystemStates_State_t SystemStates__GetState(void)
{
    return SystemStates_CurrentState;
}

/**
 * @brief Set system state
 * @details Forces a state transition
 * @param newState State to transition to
 */
void SystemStates__SetState(SystemStates_State_t newState)
{
    /* Validate input */
    if (newState >= SYSTEMSTATES_INVALID)
    {
        Serial.println("[SYSTEMSTATES] ERROR: Invalid state requested!");
        return;
    }

    if (newState == SystemStates_CurrentState)
    {
        return;
    }

    Serial.printf("[SYSTEMSTATES] State transition: %d -> %d\n",
                  (int)SystemStates_CurrentState, (int)newState);

    /* Update state */
    SystemStates_CurrentState = newState;

    /* Enter new state */
    switch (newState)
    {
        case SYSTEMSTATES_INIT:
            SystemStates_EnterInit();
            break;

        case SYSTEMSTATES_NORMAL:
            SystemStates_EnterNormal();
            break;

        case SYSTEMSTATES_FAILURE:
            SystemStates_EnterFailure();
            break;

        default:
            break;
    }
}

/**
 * @brief Get last error code
 * @return Last detected error code
 */
SystemStates_ErrorCode_t SystemStates__GetLastError(void)
{
    return SystemStates_LastError;
}

/**
 * @brief Get total error count
 * @return Total number of errors detected
 */
uint32_t SystemStates__GetErrorCount(void)
{
    return SystemStates_ErrorCount;
}

/**
 * @brief Get module initialization status
 * @return true if module is initialized, false otherwise
 * 
 * @note Added for production diagnostics and health monitoring
 */
bool SystemStates__IsInitialized(void)
{
    return SystemStates_ModuleInitialized;
}

/**
 * @brief Perform diagnostic status check
 * @details Checks all system components for errors
 * @return true if all diagnostics pass, false if error detected
 */
bool SystemStates__DiagnosticStatus(void)
{
    uint32_t timeSinceData = 0U;

    /* FIX BUG 6: Only check communication timeout after first data received */
    if (SystemStates_InitialDataReceived == true)
    {
        timeSinceData = SystemStates_GetTimeDifference(SystemStates_LastValidDataTime);
        if (timeSinceData > SYSTEMSTATES_COMM_TIMEOUT_MS)
        {
            Serial.printf("[SYSTEMSTATES] Diagnostic FAIL: Communication timeout (%lu ms)\n",
                          (unsigned long)timeSinceData);
            return false;
        }
    }

    /* Check: Modbus initialized */
    if (Modbus_IsInitialized() != true)
    {
        Serial.println("[SYSTEMSTATES] Diagnostic FAIL: Modbus not initialized");
        return false;
    }

    /* Check: CmdParser initialized */
    if (CmdParser__IsInitialized() != true)
    {
        Serial.println("[SYSTEMSTATES] Diagnostic FAIL: CmdParser not initialized");
        return false;
    }

    /* Check: Communication errors */
    if (SystemStates_CheckCommunication() != true)
    {
        Serial.println("[SYSTEMSTATES] Diagnostic FAIL: Communication error");
        return false;
    }

    /* Check: Power supply errors */
    if (SystemStates_CheckPowerSupply() != true)
    {
        Serial.println("[SYSTEMSTATES] Diagnostic FAIL: Power supply error");
        return false;
    }

    return true;
}

/**
 * @brief Clear all errors and reset counters
 * @details Resets error counters and clears error flags in all modules
 */
void SystemStates__ClearErrors(void)
{
    SystemStates_LastError         = SYSTEMSTATES_ERROR_NONE;
    SystemStates_ErrorCount        = 0U;
    SystemStates_ConsecutiveErrors = 0U;

    /* FIX BUG 3: Do NOT reset LastValidDataTime here */

    App__ClearErrors();
    CmdParser__ClearErrorStatus();

    Serial.println("[SYSTEMSTATES] All errors cleared");
}

/**
 * @brief Force system reset from failure state (manual intervention)
 * @details Call this after fixing hardware issues to exit lockout mode
 */
void SystemStates__ForceReset(void)
{
    Serial.println("[SYSTEMSTATES] ===================================");
    Serial.println("[SYSTEMSTATES] MANUAL RESET TRIGGERED");
    Serial.println("[SYSTEMSTATES] ===================================");

    /* Reset all state variables */
    SystemStates_InitialDataReceived = false;
    SystemStates__ClearErrors();
    SystemStates_LastValidDataTime = millis();

    /* Force transition to INIT state */
    SystemStates__SetState(SYSTEMSTATES_INIT);

    Serial.println("[SYSTEMSTATES] System reset complete");
}

/*==============================================================================
 *                          PRIVATE HELPER FUNCTIONS
 *============================================================================*/

static uint32_t SystemStates_GetTimeDifference(uint32_t startTime)
{
    uint32_t currentTime = millis();

    if (currentTime >= startTime)
    {
        return currentTime - startTime;
    }
    else
    {
        /* Overflow occurred (after ~49 days) */
        return (UINT32_MAX - startTime) + currentTime + 1U;
    }
}

/*==============================================================================
 *                          STATE ENTRY FUNCTIONS
 *============================================================================*/

static void SystemStates_EnterInit(void)
{
    Serial.println("[SYSTEMSTATES] Entering INIT state");
    SystemStates_ConsecutiveErrors = 0U;
    SystemStates_LastValidDataTime = millis();
}

static void SystemStates_EnterNormal(void)
{
    Serial.println("[SYSTEMSTATES] Entering NORMAL state");
    SystemStates_LastError         = SYSTEMSTATES_ERROR_NONE;
    SystemStates_ConsecutiveErrors = 0U;
    SystemStates_LastValidDataTime = millis();
}

static void SystemStates_EnterFailure(void)
{
    Serial.println("[SYSTEMSTATES] Entering FAILURE state");
    Serial.printf("[SYSTEMSTATES] Error code: %d\n", (int)SystemStates_LastError);
    Serial.printf("[SYSTEMSTATES] Total errors: %lu\n", (unsigned long)SystemStates_ErrorCount);
}

/*==============================================================================
 *                          STATE HANDLER FUNCTIONS
 *============================================================================*/

static void SystemStates_HandleInit(void)
{
    /* FIX BUG 1: Use dedicated bool flag */
    static uint32_t initStartTime    = 0U;
    static bool     initTimerStarted = false;

    if (initTimerStarted == false)
    {
        initStartTime    = millis();
        initTimerStarted = true;
        Serial.printf("[SYSTEMSTATES] Init timer started (%lu ms delay)\n",
                      (unsigned long)SYSTEMSTATES_INIT_DELAY_MS);
    }

    if (SystemStates_GetTimeDifference(initStartTime) > SYSTEMSTATES_INIT_DELAY_MS)
    {
        initTimerStarted = false;
        SystemStates__SetState(SYSTEMSTATES_NORMAL);
    }
}

static void SystemStates_HandleNormal(void)
{
    uint32_t timeSinceData = 0U;
    uint16_t dataValid     = 0U;

    /* Periodic health check */
    if (SystemStates_HealthCheckCounter >= SYSTEMSTATES_HEALTH_CHECK_INTERVAL)
    {
        SystemStates_HealthCheckCounter = 0U;

        /* Check data validity - only during health interval */
        if (Modbus_GetDataValid(&dataValid) == MODBUS_STATUS_OK)
        {
            if (dataValid == 1U)
            {
                SystemStates_LastValidDataTime = millis();

                /* FIX BUG 6: Mark first valid data received */
                if (SystemStates_InitialDataReceived == false)
                {
                    SystemStates_InitialDataReceived = true;
                    Serial.println("[SYSTEMSTATES] First valid data received - startup grace ended");
                }
            }
        }

        timeSinceData = SystemStates_GetTimeDifference(SystemStates_LastValidDataTime);
        Serial.printf("[SYSTEMSTATES] Normal | Errors: %lu | Consecutive: %lu | Data age: %lu ms\n",
                      (unsigned long)SystemStates_ErrorCount,
                      (unsigned long)SystemStates_ConsecutiveErrors,
                      (unsigned long)timeSinceData);
    }

    /* Check power supply */
    if (SystemStates_CheckPowerSupply() != true)
    {
        SystemStates_UpdateErrorStatus(SYSTEMSTATES_ERROR_POWER_SUPPLY);
        SystemStates__SetState(SYSTEMSTATES_FAILURE);
        return;
    }

    /* Check communication */
    if (SystemStates_CheckCommunication() != true)
    {
        SystemStates_UpdateErrorStatus(SYSTEMSTATES_ERROR_COMMUNICATION);
        SystemStates_ConsecutiveErrors++;

        if (SystemStates_ConsecutiveErrors >= SYSTEMSTATES_MAX_CONSECUTIVE_ERRORS)
        {
            SystemStates__SetState(SYSTEMSTATES_FAILURE);
        }
        return;
    }

    SystemStates_ConsecutiveErrors = 0U;
}

static void SystemStates_HandleFailure(void)
{
    /* FIX BUG 2: Dedicated lockout flag */
    static uint32_t failureStartTime     = 0U;
    static uint32_t recoveryAttemptCount = 0U;
    static bool     failureTimerStarted  = false;
    static bool     inLockoutMode        = false;

    bool     communicationRestored = false;
    uint16_t dataValid             = 0U;

    /* In lockout mode */
    if (inLockoutMode == true)
    {
        return;
    }

    /* Start failure timer */
    if (failureTimerStarted == false)
    {
        failureStartTime     = millis();
        failureTimerStarted  = true;
        recoveryAttemptCount = 0U;
        Serial.printf("[SYSTEMSTATES] Failure timer started (%lu ms recovery delay)\n",
                      (unsigned long)SYSTEMSTATES_FAILURE_RECOVERY_DELAY_MS);
    }

    /* Check if recovery delay elapsed */
    if (SystemStates_GetTimeDifference(failureStartTime) > SYSTEMSTATES_FAILURE_RECOVERY_DELAY_MS)
    {
        /* FIX BUG 5: Check communication BEFORE incrementing */
        communicationRestored = false;

        if (Modbus_GetDataValid(&dataValid) == MODBUS_STATUS_OK && dataValid == 1U)
        {
            communicationRestored = true;
            Serial.println("[SYSTEMSTATES] Valid data received - communication restored!");
        }
        else if (App__IsRS232CommError() != true)
        {
            communicationRestored = true;
            Serial.println("[SYSTEMSTATES] RS232 communication restored!");
        }

        if (communicationRestored == true)
        {
            /* Recovery succeeded */
            Serial.printf("[SYSTEMSTATES] Recovery successful after %lu attempt(s)\n",
                          (unsigned long)recoveryAttemptCount);

            SystemStates__ClearErrors();
            SystemStates_LastValidDataTime   = millis();
            SystemStates_InitialDataReceived = true;

            if (SystemStates__DiagnosticStatus() == true)
            {
                Serial.println("[SYSTEMSTATES] All diagnostics passed - returning to NORMAL");
                failureTimerStarted  = false;
                recoveryAttemptCount = 0U;
                SystemStates__SetState(SYSTEMSTATES_NORMAL);
            }
            else
            {
                Serial.println("[SYSTEMSTATES] Communication restored but other errors persist");
                failureStartTime = millis();
            }
        }
        else
        {
            /* FIX BUG 5: Recovery failed - NOW increment */
            recoveryAttemptCount++;
            Serial.printf("[SYSTEMSTATES] Recovery attempt #%lu FAILED\n",
                          (unsigned long)recoveryAttemptCount);

            SystemStates_UpdateErrorStatus(SYSTEMSTATES_ERROR_COMMUNICATION);

            if (SystemStates_ErrorCount >= SYSTEMSTATES_MAX_ERROR_COUNT)
            {
                Serial.println("[SYSTEMSTATES] ===================================");
                Serial.println("[SYSTEMSTATES] CRITICAL FAILURE - LOCKOUT MODE");
                Serial.println("[SYSTEMSTATES] Manual intervention required:");
                Serial.println("[SYSTEMSTATES]   1. Check RS232/RS485 connections");
                Serial.println("[SYSTEMSTATES]   2. Check 120 ohm termination");
                Serial.println("[SYSTEMSTATES]   3. Send 'RESET' command via Serial");
                Serial.println("[SYSTEMSTATES] ===================================");

                /* FIX BUG 2: Use dedicated flag */
                inLockoutMode       = true;
                failureTimerStarted = false;
                return;
            }

            failureStartTime = millis();
        }
    }
}

/*==============================================================================
 *                          CHECK FUNCTIONS
 *============================================================================*/

static bool SystemStates_CheckCommunication(void)
{
    if (App__IsRS232CommError() == true)
    {
        Serial.println("[SYSTEMSTATES] RS232 communication error detected");
        return false;
    }

    if (App__IsRS485CommError() == true)
    {
        Serial.println("[SYSTEMSTATES] RS485 communication error detected");
        return false;
    }

    return true;
}

static bool SystemStates_CheckPowerSupply(void)
{
    if (App__IsRS232PowerSupplyError() == true)
    {
        return false;
    }

    if (App__IsRS485PowerSupplyError() == true)
    {
        return false;
    }

    return true;
}

static void SystemStates_UpdateErrorStatus(SystemStates_ErrorCode_t error)
{
    SystemStates_LastError = error;
    SystemStates_ErrorCount++;

    Serial.printf("[SYSTEMSTATES] Error recorded: %d (Total: %lu)\n",
                  (int)error, (unsigned long)SystemStates_ErrorCount);
}
