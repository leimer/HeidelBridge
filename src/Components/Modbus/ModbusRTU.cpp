#include <Arduino.h>
#include "../Logger/Logger.h"
#include "../Statistics/Statistics.h"
#include "HardwareSerial.h"
#include "ModbusClientRTU.h"
#include "../../Configuration/Constants.h"
#include "../../Boards/BoardFactory.h"
#include "../../Boards/Board.h"
#include "ModbusRTU.h"

ModbusClientRTU gModbusRTU(BoardFactory::Instance()->GetBoard()->GetPinRts()); // Create a ModbusRTU client instance
HardwareSerial gRs485Serial(1);                                                // Define a Serial for UART1
SemaphoreHandle_t gMutex = nullptr;                                            // A mutex object for buss access

// Returns the singleton instance of ModbusRTU
ModbusRTU *ModbusRTU::Instance()
{
    static ModbusRTU instance;
    return &instance;
}

// Initializes the ModbusRTU instance
void ModbusRTU::Init()
{
    // Create the mutex
    gMutex = xSemaphoreCreateMutex();

    // Get pin configuration
    uint8_t pinTx = BoardFactory::Instance()->GetBoard()->GetPinTx();
    uint8_t pinRx = BoardFactory::Instance()->GetBoard()->GetPinRx();
    uint8_t pinRts = BoardFactory::Instance()->GetBoard()->GetPinRts();

    // Init serial conneted to the RTU Modbus
    Logger::Info("Starting RS485 hardware serial");
    Logger::Info("  TX Pin (DI): GPIO %d", pinTx);
    Logger::Info("  RX Pin (RO): GPIO %d", pinRx);
    Logger::Info("  RTS Pin (DE/RE): GPIO %d", pinRts);
    Logger::Info("  Baud Rate: %d", Constants::HeidelbergWallbox::ModbusBaudrate);
    Logger::Info("  Mode: SERIAL_8E1 (8 bits, Even parity, 1 stop bit)");
    Logger::Info("  Modbus Server ID: %d", Constants::HeidelbergWallbox::ModbusServerId);
    Logger::Info("  Timeout: %d ms", Constants::HeidelbergWallbox::ModbusTimeoutMs);
    
    RTUutils::prepareHardwareSerial(gRs485Serial);
    gRs485Serial.begin(
        Constants::HeidelbergWallbox::ModbusBaudrate,
        SERIAL_8E1,
        pinRx,
        pinTx);

    // Start Modbus RTU
    Logger::Debug("Creating Modbus RTU client with RTS pin %d", pinRts);
    gModbusRTU.setTimeout(Constants::HeidelbergWallbox::ModbusTimeoutMs);
    gModbusRTU.begin(gRs485Serial); // Start ModbusRTU background task
    Logger::Info("ModbusRTU client started successfully");
}

// Reads multiple registers starting from the specified address
bool ModbusRTU::ReadRegisters(uint16_t startAddress, uint8_t numValues, uint8_t fc, uint16_t *values)
{
    uint16_t numTries = 1 + Constants::ModbusRTU::NumReadRetries;
    uint8_t lastError = 0;
    uint8_t attemptNumber = 0;

    Logger::Trace("ModbusRTU read: Server=%d, Addr=%d, Count=%d, FC=0x%02X", 
                  Constants::HeidelbergWallbox::ModbusServerId, startAddress, numValues, fc);

    while (numTries > 0)
    {
        attemptNumber++;
        
        // Try to get the mutex
        if (xSemaphoreTake(gMutex, portMAX_DELAY))
        {
            ModbusMessage response = gModbusRTU.syncRequest(
                0,
                Constants::HeidelbergWallbox::ModbusServerId,
                (FunctionCode)fc,
                startAddress,
                numValues);

            // Free mutex
            xSemaphoreGive(gMutex);

            lastError = response.getError();
            
            if (lastError == SUCCESS)
            {
                constexpr uint16_t startIndex = 3;
                for (uint8_t wordIndex = 0; wordIndex < numValues; ++wordIndex)
                {
                    response.get(startIndex + wordIndex * Constants::ModbusRTU::RegisterSize, values[wordIndex]);
                }
                Logger::Trace("ModbusRTU read successful on attempt %d", attemptNumber);
                return true;
            }
            else
            {
                // Read failed
                if (lastError == 0xE0 || lastError == 224)
                {
                    Logger::Warning("ModbusRTU read attempt %d/%d: TIMEOUT (error 224/0xE0)", 
                                  attemptNumber, 1 + Constants::ModbusRTU::NumReadRetries);
                }
                else if (lastError >= 0x01 && lastError <= 0x08)
                {
                    Logger::Warning("ModbusRTU read attempt %d/%d: MODBUS EXCEPTION %d", 
                                  attemptNumber, 1 + Constants::ModbusRTU::NumReadRetries, lastError);
                }
                else
                {
                    Logger::Warning("ModbusRTU read attempt %d/%d: ERROR CODE %d (0x%02X)", 
                                  attemptNumber, 1 + Constants::ModbusRTU::NumReadRetries, lastError, lastError);
                }
                
                if (numTries > 1)
                {
                    delay(Constants::ModbusRTU::RetryDelayMs);
                }
            }
        }

        numTries--;
    }

    // All read attempts failed
    Logger::Error("ModbusRTU read FAILED after %d attempts: Error %d (0x%02X)", 
                  attemptNumber, lastError, lastError);
    
    if (lastError == 0xE0 || lastError == 224)
    {
        Logger::Error("TIMEOUT ERROR - Check RS485 wiring and wallbox configuration");
    }
    
    gStatistics.NumModbusReadErrors++;
    for (uint8_t wordIndex = 0; wordIndex < numValues; ++wordIndex)
    {
        values[wordIndex] = 0;
    }
    return false;
}

// Writes a single 16 bit holding register at the specified address
bool ModbusRTU::WriteHoldRegister16(uint16_t address, uint16_t value)
{
    uint16_t numTries = 1 + Constants::ModbusRTU::NumWriteRetries;
    uint8_t lastError = 0;
    uint8_t attemptNumber = 0;

    Logger::Debug("ModbusRTU write: Server=%d, Addr=%d, Value=%d (0x%04X)", 
                  Constants::HeidelbergWallbox::ModbusServerId, address, value, value);

    while (numTries > 0)
    {
        attemptNumber++;
        
        // Try to get the mutex
        if (xSemaphoreTake(gMutex, portMAX_DELAY))
        {
            Logger::Trace("ModbusRTU write attempt %d/%d: Sending request...", 
                         attemptNumber, 1 + Constants::ModbusRTU::NumWriteRetries);
                         
            ModbusMessage response = gModbusRTU.syncRequest(
                0,
                Constants::HeidelbergWallbox::ModbusServerId,
                WRITE_HOLD_REGISTER,
                address,
                value);

            // Free mutex
            xSemaphoreGive(gMutex);

            lastError = response.getError();
            
            if (lastError == SUCCESS)
            {
                Logger::Debug("ModbusRTU write successful on attempt %d", attemptNumber);
                return true;
            }
            else
            {
                // Write failed - log detailed error info
                if (lastError == 0xE0 || lastError == 224)
                {
                    Logger::Warning("ModbusRTU write attempt %d/%d: TIMEOUT (error 224/0xE0)", 
                                  attemptNumber, 1 + Constants::ModbusRTU::NumWriteRetries);
                    Logger::Warning("  → No response from wallbox (check wiring, baud rate, slave ID)");
                }
                else if (lastError >= 0x01 && lastError <= 0x08)
                {
                    Logger::Warning("ModbusRTU write attempt %d/%d: MODBUS EXCEPTION %d", 
                                  attemptNumber, 1 + Constants::ModbusRTU::NumWriteRetries, lastError);
                }
                else
                {
                    Logger::Warning("ModbusRTU write attempt %d/%d: ERROR CODE %d (0x%02X)", 
                                  attemptNumber, 1 + Constants::ModbusRTU::NumWriteRetries, lastError, lastError);
                }
                
                if (numTries > 1)
                {
                    Logger::Trace("  Retrying in %d ms...", Constants::ModbusRTU::RetryDelayMs);
                    delay(Constants::ModbusRTU::RetryDelayMs);
                }
            }
        }

        numTries--;
    }

    // All write attempts failed - provide diagnostic information
    Logger::Error("ModbusRTU write FAILED after %d attempts: Error %d (0x%02X)", 
                  attemptNumber, lastError, lastError);
    
    if (lastError == 0xE0 || lastError == 224)
    {
        Logger::Error("TIMEOUT ERROR - Wallbox not responding. Check:");
        Logger::Error("  1. RS485 wiring: A→A, B→B, GND→GND");
        Logger::Error("  2. Wallbox is powered on and ready");
        Logger::Error("  3. Baud rate matches wallbox (%d)", Constants::HeidelbergWallbox::ModbusBaudrate);
        Logger::Error("  4. Slave ID matches wallbox (%d)", Constants::HeidelbergWallbox::ModbusServerId);
        Logger::Error("  5. Try swapping A/B wires if polarity unclear");
        Logger::Error("  6. Check MOD-RS485 is firmly seated in UEXT connector");
    }
    
    gStatistics.NumModbusWriteErrors++;
    return false;
}