#include <Arduino.h>
#include "Components/Logger/Logger.h"
#include "../Board.h"
#include "BoardOlimex.h"

// ============================================================================
// HARDWARE: Olimex ESP32-POE-ISO Board
// ============================================================================
//
// Processor: ESP32-WROOM-32E module (ESP32-D0WDQ6 chip, dual-core, 4MB flash)
// Ethernet PHY: LAN8720A (50MHz external oscillator on GPIO0)
// PoE Controller: Si3402-B (IEEE 802.3af/at compliant)
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
// CURRENT PIN CONFIGURATION: MOD-RS485 via UEXT
// ============================================================================
//
// Pin connections for MOD-RS485 module via UEXT connector:
//   UEXT Pin 3 (TXD) -> GPIO 4  -> MOD-RS485 DI (Driver Input)
//   UEXT Pin 4 (RXD) -> GPIO 36 -> MOD-RS485 RO (Receiver Output)
//   UEXT Pin 6 (SDA) -> GPIO 13 -> MOD-RS485 DE+RE (Direction Control)
//
// GPIO 36 Notes:
//   - Input-only pin on ESP32 (perfect for RS485 RX)
//   - Has 2.2k pull-up resistor on ESP32-POE-ISO board
//   - Pull-up is fine: RS485 RO can easily drive 1.5mA (~3.3V/2.2k)
//   - Provides beneficial defined idle state (high)
//
// Pin Safety Check:
//   ✓ GPIO 4: Available on UEXT, not used by SD card or special functions
//   ✓ GPIO 36: Available on UEXT, input-only (shared with EXT - don't use there!)
//   ✓ GPIO 13: Available on UEXT (shared with EXT - don't use there!)
//
// ============================================================================
constexpr uint8_t PinRX = GPIO_NUM_36;
constexpr uint8_t PinTX = GPIO_NUM_4;
constexpr uint8_t PinRTS = GPIO_NUM_13;

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
