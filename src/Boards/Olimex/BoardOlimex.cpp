#include <Arduino.h>
#include "Components/Logger/Logger.h"
#include "../Board.h"
#include "BoardOlimex.h"

// Pin connections for Olimex ESP32-POE-ISO with MOD-RS485 via UEXT connector:
// UEXT Pin 3 (TXD) -> GPIO 4 -> MOD-RS485 DI (Driver Input)
// UEXT Pin 4 (RXD) -> GPIO 36 -> MOD-RS485 RO (Receiver Output)
// UEXT Pin 6 (SDA) -> GPIO 13 -> MOD-RS485 DE+RE (Direction Control)
//
// Note: GPIO 36 is input-only on ESP32, which is perfect for RX
//       GPIO 36 has a 2.2k pull-up on ESP32-POE-ISO board
//       This is not an issue: RS485 RO can easily drive 1.5mA (~3.3V/2.2k)
//       The pull-up provides a defined idle state (high) which is beneficial
// GPIO 4 is used for TX (output)
// GPIO 13 is used for DE/RE control (output)
constexpr uint8_t PinRX = GPIO_NUM_36;
constexpr uint8_t PinTX = GPIO_NUM_4;
constexpr uint8_t PinRTS = GPIO_NUM_13;

// Constructor
BoardOlimex::BoardOlimex()
    : Board(PinRX, PinTX, PinRTS, true)  // Has Ethernet support
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
