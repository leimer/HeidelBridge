#include <Arduino.h>
#include "Board.h"

// Constructor - Single-pin RS485 control (DE only)
Board::Board(uint8_t pinRx, uint8_t pinTx, uint8_t pinDE)
    : mPinRx(pinRx), mPinTx(pinTx), mPinDE(pinDE), mPinRE(255), mDualPinRS485(false)
{
}

// Constructor - Dual-pin RS485 control (DE and /RE)
Board::Board(uint8_t pinRx, uint8_t pinTx, uint8_t pinDE, uint8_t pinRE)
    : mPinRx(pinRx), mPinTx(pinTx), mPinDE(pinDE), mPinRE(pinRE), mDualPinRS485(true)
{
}

// Pin accessor methods
uint8_t Board::GetPinRx()
{
  return mPinRx;
}

uint8_t Board::GetPinTx()
{
  return mPinTx;
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