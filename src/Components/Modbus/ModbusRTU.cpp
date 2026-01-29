#include <Arduino.h>
#include "../Logger/Logger.h"
#include "../Statistics/Statistics.h"
#include "HardwareSerial.h"
#include "ModbusClientRTU.h"
#include "../../Configuration/Constants.h"
#include "../../Boards/BoardFactory.h"
#include "../../Boards/Board.h"
#include "ModbusRTU.h"

// NOTE: ModbusClientRTU cannot be initialized as global variable because
// BoardFactory::Instance()->GetBoard()->GetPinRts() requires BoardFactory to be
// initialized first. Global initialization order is undefined in C++.
// We use a pointer and initialize it in Init() instead.
ModbusClientRTU *gModbusRTU = nullptr;                                         // ModbusRTU client instance (initialized in Init())
HardwareSerial gRs485Serial(1);                                                // Define a Serial for UART1
SemaphoreHandle_t gMutex = nullptr;                                            // A mutex object for buss access

// Global pin variables for RTScallback
static uint8_t gPinDE = 0;
static uint8_t gPinRE = 0;
static bool gUseDualPin = false;

// RTScallback function for automatic RS485 direction control
// Called by ModbusClientRTU library when RTS state changes
// level: true = HIGH (transmit mode), false = LOW (receive mode)
void RS485RTSCallback(bool level)
{
    if (level)  // Transmit mode
    {
        digitalWrite(gPinDE, HIGH);  // DE = HIGH (driver ON)
        if (gUseDualPin)
        {
            digitalWrite(gPinRE, HIGH);  // /RE = HIGH (receiver OFF, active LOW)
        }
    }
    else  // Receive mode
    {
        digitalWrite(gPinDE, LOW);   // DE = LOW (driver OFF)
        if (gUseDualPin)
        {
            digitalWrite(gPinRE, LOW);   // /RE = LOW (receiver ON, active LOW)
        }
    }
}

// Helper function to format Modbus message as hex string
String FormatModbusMessageHex(ModbusMessage &msg)
{
    String hexStr = "";
    uint16_t len = msg.size();
    
    if (len == 0)
    {
        return "[EMPTY - NO RESPONSE/TIMEOUT]";
    }
    
    const uint8_t *data = msg.data();
    for (uint16_t i = 0; i < len; i++)
    {
        if (i > 0) hexStr += " ";
        char buf[3];
        sprintf(buf, "%02X", data[i]);
        hexStr += buf;
    }
    return hexStr;
}

// Helper function to explain Modbus error codes
const char* GetModbusErrorDescription(uint8_t errorCode)
{
    switch (errorCode)
    {
        case 0x00: return "SUCCESS - No error";
        case 0x01: return "ILLEGAL FUNCTION - Function code not supported by wallbox";
        case 0x02: return "ILLEGAL DATA ADDRESS - Register address invalid";
        case 0x03: return "ILLEGAL DATA VALUE - Value out of range";
        case 0x04: return "SLAVE DEVICE FAILURE - Wallbox internal error";
        case 0x05: return "ACKNOWLEDGE - Wallbox needs more time (rare)";
        case 0x06: return "SLAVE DEVICE BUSY - Wallbox busy, retry later";
        case 0x07: return "NEGATIVE ACKNOWLEDGE - Wallbox cannot process";
        case 0x08: return "MEMORY PARITY ERROR - Wallbox memory fault";
        case 0xE0: return "TIMEOUT - No response from wallbox (check wiring/config)";
        case 0xE1: return "INVALID SERVER - Wrong server ID configured";
        case 0xE2: return "CRC ERROR - Communication error (noise/wiring issue)";
        case 0xE3: return "FC MISMATCH - Response doesn't match request";
        case 0xE4: return "SERVER ID MISMATCH - Wrong server responded";
        case 0xE5: return "PACKET LENGTH ERROR - Truncated response";
        default: return "UNKNOWN ERROR - Check eModbus documentation";
    }
}

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
    bool dualPin = BoardFactory::Instance()->GetBoard()->HasDualPinRS485();
    
    // Init serial connected to the RTU Modbus
    RTUutils::prepareHardwareSerial(gRs485Serial);
    gRs485Serial.begin(
        Constants::HeidelbergWallbox::ModbusBaudrate,
        SERIAL_8E1,
        pinRx,
        pinTx);

    // Create ModbusClientRTU with appropriate constructor
    if (dualPin)
    {
        // Dual-pin boards (e.g., ESP32-POE-ISO): Use RTScallback for explicit DE+RE control
        gPinDE = BoardFactory::Instance()->GetBoard()->GetPinDE();
        gPinRE = BoardFactory::Instance()->GetBoard()->GetPinRE();
        gUseDualPin = true;
        gModbusRTU = new ModbusClientRTU(RS485RTSCallback);
    }
    else
    {
        // Single-pin boards (e.g., ESP32, Lilygo): Let library handle RTS pin directly
        uint8_t pinRts = BoardFactory::Instance()->GetBoard()->GetPinRts();
        gModbusRTU = new ModbusClientRTU(pinRts);
    }
    
    // Start Modbus RTU
    gModbusRTU->setTimeout(Constants::HeidelbergWallbox::ModbusTimeoutMs);
    gModbusRTU->begin(gRs485Serial);
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
            // RTScallback automatically handles mode switching
            ModbusMessage response = gModbusRTU->syncRequest(
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
            // RTScallback automatically handles mode switching
            ModbusMessage response = gModbusRTU->syncRequest(
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
                Logger::Trace("ModbusRTU write successful on attempt %d", attemptNumber);
                return true;
            }
            else
            {
                // Write failed
                if (lastError == 0xE0 || lastError == 224)
                {
                    Logger::Warning("ModbusRTU write attempt %d/%d: TIMEOUT (error 224/0xE0)", 
                                  attemptNumber, 1 + Constants::ModbusRTU::NumWriteRetries);
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
                    delay(Constants::ModbusRTU::RetryDelayMs);
                }
            }
        }

        numTries--;
    }

    // All write attempts failed
    Logger::Error("ModbusRTU write FAILED after %d attempts: Error %d (0x%02X) - %s", 
                  attemptNumber, lastError, lastError, GetModbusErrorDescription(lastError));
    
    gStatistics.NumModbusWriteErrors++;
    return false;
}