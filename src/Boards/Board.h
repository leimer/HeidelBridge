#pragma once

class Board
{
protected:
  // Constructor
  Board(uint8_t pinRx, uint8_t pinTx, uint8_t pinRts);
  Board(uint8_t pinRx, uint8_t pinTx, uint8_t pinDE, uint8_t pinRE);  // Dual-pin RS485 control

public:
  // Initializes the board
  virtual void Init() = 0;

  // Logs board name/information
  virtual void Print() = 0;

  // These functions return the pins used by this board
  uint8_t GetPinRx();
  uint8_t GetPinTx();
  uint8_t GetPinRts();  // Returns DE pin (for backward compatibility)
  uint8_t GetPinDE();   // Driver Enable pin
  uint8_t GetPinRE();   // Receiver Enable pin (active LOW)
  bool HasDualPinRS485();  // Returns true if both DE and RE pins are configured
  
  // Network capability interfaces - to be implemented by each board
  virtual bool HasWiFi() = 0;
  virtual bool HasEthernet() = 0;

private:
  uint8_t mPinRx, mPinTx, mPinRts;
  uint8_t mPinDE, mPinRE;  // For dual-pin RS485 control
  bool mDualPinRS485;  // Flag to indicate if using dual-pin control
};