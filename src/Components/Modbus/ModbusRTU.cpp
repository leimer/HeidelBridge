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

// Helper functions for dual-pin RS485 control
void SetRS485TransmitMode()
{
    if (BoardFactory::Instance()->GetBoard()->HasDualPinRS485())
    {
        uint8_t pinRE = BoardFactory::Instance()->GetBoard()->GetPinRE();
        // Transmit mode: /RE = HIGH (receiver DISABLED, because /RE is active LOW)
        digitalWrite(pinRE, HIGH);
    }
}

void SetRS485ReceiveMode()
{
    if (BoardFactory::Instance()->GetBoard()->HasDualPinRS485())
    {
        uint8_t pinRE = BoardFactory::Instance()->GetBoard()->GetPinRE();
        // Receive mode: /RE = LOW (receiver ENABLED, because /RE is active LOW)
        digitalWrite(pinRE, LOW);
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
    
    // Init serial conneted to the RTU Modbus
    Logger::Info("========================================");
    Logger::Info("RS485 ModbusRTU Initialization");
    Logger::Info("========================================");
    Logger::Info("Hardware Configuration:");
    Logger::Info("  TX Pin (DI):  GPIO %d → MOD-RS485 Driver Input", pinTx);
    Logger::Info("  RX Pin (RO):  GPIO %d → MOD-RS485 Receiver Output", pinRx);
    
    if (dualPin)
    {
        uint8_t pinDE = BoardFactory::Instance()->GetBoard()->GetPinDE();
        uint8_t pinRE = BoardFactory::Instance()->GetBoard()->GetPinRE();
        Logger::Info("  DE Pin:       GPIO %d → MOD-RS485 Driver Enable (active HIGH)", pinDE);
        Logger::Info("  /RE Pin:      GPIO %d → MOD-RS485 Receiver Enable (active LOW)", pinRE);
        Logger::Info("  Mode:         DUAL-PIN FULL CONTROL");
    }
    else
    {
        uint8_t pinRts = BoardFactory::Instance()->GetBoard()->GetPinRts();
        Logger::Info("  RTS Pin (DE): GPIO %d → MOD-RS485 Direction Control", pinRts);
        Logger::Info("  Mode:         SINGLE-PIN CONTROL");
    }
    
    Logger::Info("");
    Logger::Info("Serial Configuration:");
    Logger::Info("  Baud Rate: %d bps", Constants::HeidelbergWallbox::ModbusBaudrate);
    Logger::Info("  Data Bits: 8");
    Logger::Info("  Parity:    Even");
    Logger::Info("  Stop Bits: 1");
    Logger::Info("  Mode:      SERIAL_8E1");
    Logger::Info("");
    Logger::Info("Modbus Configuration:");
    Logger::Info("  Server ID: %d (wallbox slave address)", Constants::HeidelbergWallbox::ModbusServerId);
    Logger::Info("  Timeout:   %d ms", Constants::HeidelbergWallbox::ModbusTimeoutMs);
    Logger::Info("  Retries:   %d (write), %d (read)", 
                 Constants::ModbusRTU::NumWriteRetries, Constants::ModbusRTU::NumReadRetries);
    Logger::Info("========================================");
    
    RTUutils::prepareHardwareSerial(gRs485Serial);
    gRs485Serial.begin(
        Constants::HeidelbergWallbox::ModbusBaudrate,
        SERIAL_8E1,
        pinRx,
        pinTx);

    // Create Modbus RTU client
    // For dual-pin control, we'll use the DE pin for the library's RTS
    // and manually control /RE pin
    if (dualPin)
    {
        uint8_t pinDE = BoardFactory::Instance()->GetBoard()->GetPinDE();
        Logger::Debug("Creating ModbusClientRTU with dual-pin control: DE=GPIO%d", pinDE);
        gModbusRTU = new ModbusClientRTU(pinDE);
    }
    else
    {
        uint8_t pinRts = BoardFactory::Instance()->GetBoard()->GetPinRts();
        Logger::Debug("Creating ModbusClientRTU with RTS pin GPIO %d", pinRts);
        gModbusRTU = new ModbusClientRTU(pinRts);
    }
    
    // Start Modbus RTU
    gModbusRTU->setTimeout(Constants::HeidelbergWallbox::ModbusTimeoutMs);
    gModbusRTU->begin(gRs485Serial); // Start ModbusRTU background task
    Logger::Info("ModbusRTU client started successfully");
    Logger::Info("========================================");
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
            // For dual-pin RS485: Set transmit mode before sending
            SetRS485TransmitMode();
            
            ModbusMessage response = gModbusRTU->syncRequest(
                0,
                Constants::HeidelbergWallbox::ModbusServerId,
                (FunctionCode)fc,
                startAddress,
                numValues);

            // For dual-pin RS485: Set receive mode after transaction
            SetRS485ReceiveMode();
            
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

    Logger::Info("┌─────────────────────────────────────────────");
    Logger::Info("│ ModbusRTU WRITE Request");
    Logger::Info("├─────────────────────────────────────────────");
    Logger::Info("│ Server ID:   %d", Constants::HeidelbergWallbox::ModbusServerId);
    Logger::Info("│ Function:    0x06 (Write Single Register)");
    Logger::Info("│ Address:     %d (0x%04X)", address, address);
    Logger::Info("│ Value:       %d (0x%04X)", value, value);
    Logger::Info("│ GPIO Usage:  TX=%d RX=%d RTS=%d", 
                 BoardFactory::Instance()->GetBoard()->GetPinTx(),
                 BoardFactory::Instance()->GetBoard()->GetPinRx(),
                 BoardFactory::Instance()->GetBoard()->GetPinRts());
    Logger::Info("└─────────────────────────────────────────────");

    while (numTries > 0)
    {
        attemptNumber++;
        
        Logger::Info(">>> Attempt %d/%d: Sending request...", 
                     attemptNumber, 1 + Constants::ModbusRTU::NumWriteRetries);
        
        // Try to get the mutex
        if (xSemaphoreTake(gMutex, portMAX_DELAY))
        {
            // For dual-pin RS485: Set transmit mode before sending
            // The library will control DE (Driver Enable) via its RTS pin
            // We manually control /RE (Receiver Enable)
            SetRS485TransmitMode();
            
            // Build and send the request
            ModbusMessage response = gModbusRTU->syncRequest(
                0,
                Constants::HeidelbergWallbox::ModbusServerId,
                WRITE_HOLD_REGISTER,
                address,
                value);

            // For dual-pin RS485: Set receive mode after transaction
            SetRS485ReceiveMode();
            
            // Free mutex
            xSemaphoreGive(gMutex);

            // Log the request frame (reconstructed for display)
            Logger::Info(">>> TX BYTES: 01 06 %02X %02X %02X %02X [+CRC]", 
                        (address >> 8) & 0xFF, address & 0xFF,
                        (value >> 8) & 0xFF, value & 0xFF);
            Logger::Info("    Frame: [ServerID=0x01][FC=0x06][Addr][Value][CRC]");

            lastError = response.getError();
            
            // Log the response
            String responseHex = FormatModbusMessageHex(response);
            Logger::Info("<<< RX BYTES: %s", responseHex.c_str());
            
            if (lastError == SUCCESS)
            {
                Logger::Info("<<< Response: SUCCESS");
                Logger::Info("└─────────────────────────────────────────────");
                Logger::Info("✓ Write completed successfully on attempt %d", attemptNumber);
                Logger::Info("");
                return true;
            }
            else
            {
                // Write failed - log detailed error info
                Logger::Error("<<< Response: ERROR %d (0x%02X)", lastError, lastError);
                Logger::Error("<<< Meaning: %s", GetModbusErrorDescription(lastError));
                Logger::Error("└─────────────────────────────────────────────");
                
                if (lastError == 0xE0 || lastError == 224)
                {
                    Logger::Error("✗ TIMEOUT - No response received from wallbox");
                    Logger::Error("  Possible causes:");
                    Logger::Error("  • RS485 wiring incorrect (check A/B/GND)");
                    Logger::Error("  • Wallbox not powered or not ready");
                    Logger::Error("  • Wrong baud rate (wallbox ≠ %d)", Constants::HeidelbergWallbox::ModbusBaudrate);
                    Logger::Error("  • Wrong server ID (wallbox ≠ %d)", Constants::HeidelbergWallbox::ModbusServerId);
                    Logger::Error("  • A/B polarity reversed");
                    Logger::Error("  • MOD-RS485 not seated properly in UEXT");
                    Logger::Error("  • Cable too long or poor quality");
                }
                else if (lastError >= 0x01 && lastError <= 0x08)
                {
                    Logger::Error("✗ MODBUS EXCEPTION - Wallbox rejected the request");
                    Logger::Error("  The wallbox received the request but cannot fulfill it");
                }
                else if (lastError == 0xE2)
                {
                    Logger::Error("✗ CRC ERROR - Communication corruption detected");
                    Logger::Error("  • Check for electrical noise on RS485 line");
                    Logger::Error("  • Verify proper cable shielding");
                    Logger::Error("  • Check termination resistors");
                }
                
                if (numTries > 1)
                {
                    Logger::Warning("  ⟳ Retrying in %d ms...", Constants::ModbusRTU::RetryDelayMs);
                    Logger::Info("");
                    delay(Constants::ModbusRTU::RetryDelayMs);
                }
            }
        }

        numTries--;
    }

    // All write attempts failed
    Logger::Error("═══════════════════════════════════════════════");
    Logger::Error("✗ ModbusRTU WRITE FAILED");
    Logger::Error("═══════════════════════════════════════════════");
    Logger::Error("After %d attempts, all failed with error %d (0x%02X)", 
                  attemptNumber, lastError, lastError);
    Logger::Error("Error: %s", GetModbusErrorDescription(lastError));
    Logger::Error("═══════════════════════════════════════════════");
    Logger::Error("");
    
    gStatistics.NumModbusWriteErrors++;
    return false;
}