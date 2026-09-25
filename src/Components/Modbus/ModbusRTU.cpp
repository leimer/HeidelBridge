#include <Arduino.h>
#include "../Logger/Logger.h"
#include "../Statistics/Statistics.h"
#include "HardwareSerial.h"
#include "../../Configuration/Constants.h"
#include "../../Boards/BoardFactory.h"
#include "../../Boards/Board.h"
#include "ModbusRTU.h"

HardwareSerial gRs485Serial(1);                           // Define a Serial for UART1
SemaphoreHandle_t gMutex = nullptr;                       // A mutex object for buss access

// Global pin variables for RTScallback (used only by dual-pin boards)
// These must be global because the callback cannot capture local state.
static uint8_t gPinDE = 0;
static uint8_t gPinRE = 0;

// Helper function to explain Modbus error codes
const char *GetModbusErrorDescription(uint8_t errorCode)
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
    Logger::Info("Starting RS485 hardware serial");
    RTUutils::prepareHardwareSerial(gRs485Serial);
    gRs485Serial.begin(
        Constants::HeidelbergWallbox::ModbusBaudrate,
        SERIAL_8E1,
        pinRx,
        pinTx);

    // Create ModbusClientRTU with appropriate constructor
    if (dualPin)
    {
        gPinDE = BoardFactory::Instance()->GetBoard()->GetPinDE();
        gPinRE = BoardFactory::Instance()->GetBoard()->GetPinRE();

        auto rtsCallback = [](bool level)
        {
            if (level)
            {
                digitalWrite(gPinDE, HIGH);
                digitalWrite(gPinRE, HIGH);
            }
            else
            {
                digitalWrite(gPinDE, LOW);
                digitalWrite(gPinRE, LOW);
            }
        };

        modbusClient = new ModbusClientRTU(rtsCallback);
    }
    else
    {
        modbusClient = new ModbusClientRTU(BoardFactory::Instance()->GetBoard()->GetPinDE());
    }

    // Start Modbus RTU
    Logger::Trace("Creating Modbus RTU instance");
    modbusClient->setTimeout(Constants::HeidelbergWallbox::ModbusTimeoutMs);
    modbusClient->begin(gRs485Serial);
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
            ModbusMessage response = modbusClient->syncRequest(
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

            Logger::Warning("ModbusRTU read attempt %d/%d: Error %d (0x%02X) - %s",
                            attemptNumber, 1 + Constants::ModbusRTU::NumReadRetries,
                            lastError, lastError, GetModbusErrorDescription(lastError));
            delay(Constants::ModbusRTU::RetryDelayMs);
        }

        numTries--;
    }

    // All read attempts failed
    Logger::Error("ModbusRTU read FAILED after %d attempts: Error %d (0x%02X) - %s",
                  attemptNumber, lastError, lastError, GetModbusErrorDescription(lastError));

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
            ModbusMessage response = modbusClient->syncRequest(
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

            Logger::Warning("ModbusRTU write attempt %d/%d: Error %d (0x%02X) - %s",
                            attemptNumber, 1 + Constants::ModbusRTU::NumWriteRetries,
                            lastError, lastError, GetModbusErrorDescription(lastError));
            delay(Constants::ModbusRTU::RetryDelayMs);
        }

        numTries--;
    }

    // All write attempts failed
    Logger::Error("ModbusRTU write FAILED after %d attempts: Error %d (0x%02X) - %s",
                  attemptNumber, lastError, lastError, GetModbusErrorDescription(lastError));

    gStatistics.NumModbusWriteErrors++;
    return false;
}