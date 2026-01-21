#include <Arduino.h>
#include "Board.h"

// Constructor
Board::Board(uint8_t pinRx, uint8_t pinTx, uint8_t pinRts, bool hasWiFi, bool hasEthernet)
    : mPinRx(pinRx), mPinTx(pinTx), mPinRts(pinRts), mHasWiFi(hasWiFi), mHasEthernet(hasEthernet)
{
}

// These functions return the pins used by this board
uint8_t Board::GetPinRx()
{
  return mPinRx;
}

// These functions return the pins used by this board
uint8_t Board::GetPinTx()
{
  return mPinTx;
}

// These functions return the pins used by this board
uint8_t Board::GetPinRts()
{
  return mPinRts;
}

// Network capability interfaces
bool Board::HasWiFi()
{
  return mHasWiFi;
}

bool Board::HasEthernet()
{
  return mHasEthernet;
}