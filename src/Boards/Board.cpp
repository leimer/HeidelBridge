#include <Arduino.h>
#include "Board.h"

// Constructor - Single RTS pin (backward compatibility)
Board::Board(uint8_t pinRx, uint8_t pinTx, uint8_t pinRts)
    : mPinRx(pinRx), mPinTx(pinTx), mPinRts(pinRts), mPinDE(pinRts), mPinRE(255), mDualPinRS485(false)
{
}

// Constructor - Dual pin RS485 control (DE and /RE)
Board::Board(uint8_t pinRx, uint8_t pinTx, uint8_t pinDE, uint8_t pinRE)
    : mPinRx(pinRx), mPinTx(pinTx), mPinRts(pinDE), mPinDE(pinDE), mPinRE(pinRE), mDualPinRS485(true)
{
}

// These functions return the pins used by this board
uint8_t Board::GetPinRx()
{
  return mPinRx;
}

uint8_t Board::GetPinTx()
{
  return mPinTx;
}

uint8_t Board::GetPinRts()
{
  return mPinRts;  // Returns DE pin for backward compatibility
}

uint8_t Board::GetPinDE()
{
  return mPinDE;
}

uint8_t Board::GetPinRE()
{
  return mPinRE;
}

bool Board::HasDualPinRS485()
{
  return mDualPinRS485;
}