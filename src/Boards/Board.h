#pragma once

class Board
{
protected:
  // Constructors for RS485 pin configuration
  Board(uint8_t pinRx, uint8_t pinTx, uint8_t pinDE);  // Single-pin RS485 (DE only)
  Board(uint8_t pinRx, uint8_t pinTx, uint8_t pinDE, uint8_t pinRE);  // Dual-pin RS485 (DE + RE)

public:
  // Initializes the board
  virtual void Init() = 0;

  // Logs board name/information
  virtual void Print() = 0;

  // RS485 pin access
  uint8_t GetPinRx();
  uint8_t GetPinTx();
  uint8_t GetPinDE();   // Driver Enable pin (all boards have this)
  uint8_t GetPinRE();   // Receiver Enable pin (only dual-pin boards)
  bool HasDualPinRS485();  // Returns true if both DE and RE pins are configured
  
  // Network capability interfaces - to be implemented by each board
  virtual bool HasWiFi() = 0;
  virtual bool HasEthernet() = 0;

private:
  uint8_t mPinRx, mPinTx;
  uint8_t mPinDE, mPinRE;
  bool mDualPinRS485;
};