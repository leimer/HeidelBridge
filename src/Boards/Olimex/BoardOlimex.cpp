#include <Arduino.h>
#include "Components/Logger/Logger.h"
#include "../Board.h"
#include "BoardOlimex.h"

// ============================================================================
// HARDWARE: Olimex ESP32-POE-ISO Board
// ============================================================================
//
// Processor: ESP32-WROOM-32E module (ESP32-D0WDQ6 chip, dual-core, 4MB flash)
// Ethernet PHY: ETH8720 chip (LAN8720 compatible)
// PoE Controller: Si3402-B (IEEE 802.3af/at compliant)
//
// ETHERNET PHY CONFIGURATION (Official from ESP32-POE-ISO manual):
//   Chip: ETH8720 (LAN8720 compatible)
//   MDC: GPIO 23
//   MDIO: GPIO 18
//   PHY Reset: GPIO 12
//   PHY RMII Clock: GPIO 0 (50MHz input from external oscillator)
//   PHY Address: 0
//   RJ45 Connector: Supports both PoE Mode A and Mode B
//
// ============================================================================
// POWER SUPPLY SPECIFICATIONS
// ============================================================================
//
// +5V:   Max 0.2A @ 5V (1W max)
//        - Can be input or output (not both simultaneously)
//        - When using USB or PoE: Available as OUTPUT to power peripherals
//        - When using as INPUT: Disconnect USB and PoE first!
//
// +3.3V: Max 0.33A @ 3.3V (1W max)
//        - Output only
//
// IMPORTANT: Combined power from +5V and +3.3V must NOT exceed 1W total!
//
// ============================================================================
// GPIO PIN USAGE AND CONSTRAINTS
// ============================================================================
//
// SPECIAL FUNCTION PINS:
//   GPIO34: User button (has 10K pull-up resistor)
//   GPIO35: LiPo battery voltage measurement
//   GPIO39: External power supply voltage measurement
//
// PROGRAMMING PINS (free after programming):
//   GPIO0, GPIO1: Used only during programming, free to use afterwards
//
// SD CARD PINS (free if no SD card):
//   GPIO2, GPIO14, GPIO15: Used for SD-card, free if no SD card present
//
// SHARED PINS (WARNING - Do not use on both UEXT and EXT simultaneously):
//   GPIO2, GPIO4, GPIO5, GPIO13, GPIO14, GPIO15, GPIO16, GPIO36
//   These pins are shared between UEXT and EXT headers
//   Using a pin on one connector means it's unavailable on the other!
//
// ============================================================================
// ============================================================================
// CURRENT PIN CONFIGURATION: MOD-RS485 via UEXT
// ============================================================================
//
// Pin connections for MOD-RS485 module via UEXT connector:
//   UEXT Pin 3 (TXD) -> GPIO 4  -> MOD-RS485 DI (Driver Input)
//   UEXT Pin 4 (RXD) -> GPIO 36 -> MOD-RS485 RO (Receiver Output)
//   UEXT Pin 9 (SCK) -> GPIO 14 -> MOD-RS485 DE (Driver Enable)
//
// ⚠️ IMPORTANT: ESP32-POE vs ESP32-POE-ISO Pin Differences
//
// ESP32-POE-ISO (this board):
//   UEXT Pin 9 (SCK): GPIO 14
//   UEXT Pin 10 (#SS): GPIO 5
//
// ESP32-POE (non-ISO variant):
//   UEXT Pin 9 (SCK): GPIO 2  
//   UEXT Pin 10 (#SS): GPIO 5
//
// Always verify your board model! Using wrong pins = communication failure.
//
// GPIO 36 Notes:
//   - Input-only pin on ESP32 (perfect for RS485 RX)
//   - Has 2.2k pull-up resistor on ESP32-POE-ISO board
//   - Pull-up is fine: RS485 RO can easily drive 1.5mA (~3.3V/2.2k)
//   - Provides beneficial defined idle state (high)
//
// GPIO 14 Notes:
//   - Also used for SD card on ESP32-POE-ISO (conflict if SD present)
//   - HeidelBridge doesn't use SD card, so GPIO 14 is available
//   - Safe to use for RS485 direction control
//
// Pin Safety Check:
//   ✓ GPIO 4: Available on UEXT, not used by SD card or special functions
//   ✓ GPIO 36: Available on UEXT, input-only (shared with EXT - don't use there!)
//   ✓ GPIO 14: Available on UEXT (conflicts with SD if present, but we don't use SD)
//
// ============================================================================
// MOD-RS485 JUMPER CONFIGURATION - WORKS WITH DEFAULTS! ✓
// ============================================================================
//
// This configuration works with DEFAULT MOD-RS485 jumper positions:
//   SCL/SCK = SCK:  UEXT Pin 9 (GPIO 14) -> DE (Driver Enable) ✓
//   #SS/SDA = #SS:  UEXT Pin 10 (GPIO 5) -> /RE (Receiver Enable) ✓
//
// NO JUMPER CHANGES REQUIRED!
//
// How it works (DUAL-PIN FULL CONTROL):
//   - Code controls GPIO 14 (DE - Driver Enable) and GPIO 5 (/RE - Receiver Enable)
//   - Transmit mode: DE=HIGH (driver on), /RE=HIGH (receiver off)
//   - Receive mode: DE=LOW (driver off), /RE=LOW (receiver on)
//   - Automatic mode switching via RTScallback from eModbus library
//
// DOCUMENTATION:
//   See docs/MOD-RS485-Configuration.md for complete jumper information.
//
    // RS485 UEXT Pins (MOD-RS485 Module) - DUAL PIN FULL CONTROL
    // These pins connect to the MOD-RS485 RS485 transceiver module via UEXT connector
    //
    // Pin Assignments (ESP32-POE-ISO):
    //   TX:  GPIO 4  (UEXT Pin 3 - TXD) → MOD-RS485 DI (Driver Input)
    //   RX:  GPIO 36 (UEXT Pin 4 - RXD) → MOD-RS485 RO (Receiver Output)
    //   DE:  GPIO 14 (UEXT Pin 9 - SCK) → MOD-RS485 DE (Driver Enable via SCK jumper)
    //   /RE: GPIO 5  (UEXT Pin 10 - #SS) → MOD-RS485 /RE (Receiver Enable via #SS jumper)
    //
    // MOD-RS485 Jumper Configuration:
    //   ENABLE_RT: CLOSED (120Ω termination enabled)
    //   SCL/SCK:   SCK position (default) → GPIO 14 controls DE
    //   #SS/SDA:   #SS position (default) → GPIO 5 controls /RE
    //
    // Direction Control (DUAL-PIN FULL CONTROL):
    //   Using both DE and /RE for explicit transmit/receive mode control
    //   Transmit: DE=HIGH (driver on), /RE=LOW (receiver off)
    //   Receive:  DE=LOW (driver off), /RE=HIGH (receiver on)
    //
constexpr uint8_t PinRX = GPIO_NUM_36;
constexpr uint8_t PinTX = GPIO_NUM_4;
constexpr uint8_t PinDE = GPIO_NUM_14;   // Driver Enable (active HIGH)
constexpr uint8_t PinRE = GPIO_NUM_5;    // Receiver Enable (active LOW)

// Constructor - Use dual-pin RS485 control
BoardOlimex::BoardOlimex()
    : Board(PinRX, PinTX, PinDE, PinRE)
{
  // Nothing to do
}

// Initializes the board
void BoardOlimex::Init()
{
  Logger::Info("========================================");
  Logger::Info("BoardOlimex Initialization");
  Logger::Info("========================================");
  
  // Log pin configuration
  Logger::Info("RS485 Pin Configuration:");
  Logger::Info("  TX  (UEXT Pin 3):  GPIO %d", PinTX);
  Logger::Info("  RX  (UEXT Pin 4):  GPIO %d (input-only, 2.2k pull-up)", PinRX);
  Logger::Info("  DE  (UEXT Pin 9):  GPIO %d (Driver Enable, active HIGH)", PinDE);
  Logger::Info("  /RE (UEXT Pin 10): GPIO %d (Receiver Enable, active LOW)", PinRE);
  
  // Configure DE pin (Driver Enable - active HIGH)
  pinMode(PinDE, OUTPUT);
  digitalWrite(PinDE, LOW);  // Start in receive mode (driver off)
  Logger::Debug("GPIO %d (DE) configured as OUTPUT, initial state: LOW (driver OFF)", PinDE);
  
  // Configure /RE pin (Receiver Enable - active LOW)
  pinMode(PinRE, OUTPUT);
  digitalWrite(PinRE, LOW);  // Start in receive mode (receiver enabled, /RE=LOW means ON)
  Logger::Debug("GPIO %d (/RE) configured as OUTPUT, initial state: LOW (receiver ON)", PinRE);
  
  // Verify pin states
  int deState = digitalRead(PinDE);
  int reState = digitalRead(PinRE);
  Logger::Info("Pin state verification:");
  Logger::Info("  DE  (GPIO %d): %s (expected: LOW)", PinDE, deState == LOW ? "LOW ✓" : "HIGH ✗");
  Logger::Info("  /RE (GPIO %d): %s (expected: LOW)", PinRE, reState == LOW ? "LOW ✓" : "HIGH ✗");
  Logger::Info("  Mode: RECEIVE (driver off, receiver on)");
  
  Logger::Info("RS485 dual-pin control initialized successfully");
  Logger::Info("========================================");
}

// Logs board name/information
void BoardOlimex::Print()
{
  Logger::Print("Olimex ESP32-POE-ISO with MOD-RS485 via UEXT");
}

// Network capability interfaces
bool BoardOlimex::HasWiFi()
{
  return true;
}

bool BoardOlimex::HasEthernet()
{
  return true;
}
