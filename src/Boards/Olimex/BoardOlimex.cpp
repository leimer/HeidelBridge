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
//   UEXT Pin 10 (#SS): GPIO 15
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
//   #SS/SDA = #SS:  UEXT Pin 10 (GPIO 15) -> /RE (Receiver Enable)
//
// NO JUMPER CHANGES REQUIRED!
//
// How it works:
//   - Code controls GPIO 14 (DE - Driver Enable)
//   - When HIGH: Transceiver in transmit mode, driver enabled
//   - When LOW: Transceiver in receive mode, driver disabled
//   - /RE (GPIO 15) can remain uncontrolled (pulled high by default)
//   - This works because RS485 is half-duplex (never transmit and receive simultaneously)
//
// Note: We don't explicitly control /RE (Receiver Enable) on GPIO 15.
//       The transceiver's /RE pin may be pulled high or low by board default,
//       but since we control DE (Driver Enable), the transceiver will work correctly
//       in half-duplex mode. When DE is LOW, the driver is off regardless of /RE state.
//
// DOCUMENTATION:
//   See docs/MOD-RS485-Configuration.md for complete jumper information.
//
// ============================================================================
constexpr uint8_t PinRX = GPIO_NUM_36;
constexpr uint8_t PinTX = GPIO_NUM_4;
constexpr uint8_t PinRTS = GPIO_NUM_14;  // Changed from GPIO 13 to work with default jumpers

// Constructor
BoardOlimex::BoardOlimex()
    : Board(PinRX, PinTX, PinRTS)
{
  // Nothing to do
}

// Initializes the board
void BoardOlimex::Init()
{
  // Configure RTS pin for RS485 DE/RE control
  pinMode(PinRTS, OUTPUT);
  digitalWrite(PinRTS, LOW);
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
