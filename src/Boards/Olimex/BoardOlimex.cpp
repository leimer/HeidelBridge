#include <Arduino.h>
#include "Components/Logger/Logger.h"
#include "../Board.h"
#include "BoardOlimex.h"

// Pin connections for Olimex ESP32-POE with MOD-RS485:
// MOD-RS485 can be connected to UEXT connector
// UEXT Pin 3 (TXD) -> ESP32 GPIO1 (TX)
// UEXT Pin 4 (RXD) -> ESP32 GPIO3 (RX)
// For RS485 DE/RE control, we'll use GPIO4
constexpr uint8_t PinRX = GPIO_NUM_3;
constexpr uint8_t PinTX = GPIO_NUM_1;
constexpr uint8_t PinRTS = GPIO_NUM_4;

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
  Logger::Print("Olimex ESP32-POE with MOD-RS485");
}
