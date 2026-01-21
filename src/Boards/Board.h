#pragma once

class Board
{
protected:
  // Constructor
  Board(uint8_t pinRx, uint8_t pinTx, uint8_t pinRts, bool hasEthernet = false);

public:
  // Initializes the board
  virtual void Init() = 0;

  // Logs board name/information
  virtual void Print() = 0;

  // These functions return the pins used by this board
  uint8_t GetPinRx();
  uint8_t GetPinTx();
  uint8_t GetPinRts();
  
  // Returns true if this board has Ethernet support
  bool HasEthernet();

private:
  uint8_t mPinRx, mPinTx, mPinRts;
  bool mHasEthernet;
};