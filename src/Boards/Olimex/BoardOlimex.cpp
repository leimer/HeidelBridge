#include <Arduino.h>
#include "Components/Logger/Logger.h"
#include "../Board.h"
#include "BoardOlimex.h"

// Pin connections for Olimex ESP32-POE-ISO with MOD-RS485 via UEXT:
// ESP32 GPIO4  -> MOD-RS485 DI (Driver Input)
// ESP32 GPIO36 -> MOD-RS485 RO (Receiver Output)
// ESP32 GPIO14 -> MOD-RS485 DE (Driver Enable)
// ESP32 GPIO5  -> MOD-RS485 /RE (Receiver Enable, active LOW)
//
// Note: Uses dual-pin RS485 control for explicit transmit/receive mode switching
// For detailed hardware information, see docs/ESP32-POE-ISO-Hardware.md
constexpr uint8_t PinRX = GPIO_NUM_36;
constexpr uint8_t PinTX = GPIO_NUM_4;
constexpr uint8_t PinDE = GPIO_NUM_14;
constexpr uint8_t PinRE = GPIO_NUM_5;

// Constructor - Use dual-pin RS485 control
BoardOlimex::BoardOlimex()
    : Board(PinRX, PinTX, PinDE, PinRE)
{
  // Nothing to do
}

// Initializes the board
void BoardOlimex::Init()
{
  // Configure DE pin (Driver Enable - active HIGH)
  pinMode(PinDE, OUTPUT);
  digitalWrite(PinDE, LOW);  // Start in receive mode (driver off)
  
  // Configure /RE pin (Receiver Enable - active LOW)
  pinMode(PinRE, OUTPUT);
  digitalWrite(PinRE, LOW);  // Start in receive mode (receiver on)
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
