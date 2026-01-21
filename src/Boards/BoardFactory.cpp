#include <Arduino.h>
#include "Board.h"
#include "BoardFactory.h"
#include "../Configuration/Settings.h"

// All supported boards
#include "ESP32/BoardESP32.h"
#include "Lilygo/BoardLilygo.h"
#include "Olimex/BoardOlimex.h"

// Factory instance
BoardFactory *BoardFactory::Instance()
{
  static BoardFactory factoryInstance;
  return &factoryInstance;
}

// Gets the currently used board
Board *BoardFactory::GetBoard()
{
<<<<<<< HEAD
  static BoardESP32 genericBoard;
  static BoardLilygo lilygoBoard;
=======
#ifdef BOARD_ESP32
  static BoardESP32 boardInstance;
#elif BOARD_LILYGO
  static BoardLilygo boardInstance;
#elif BOARD_OLIMEX
  static BoardOlimex boardInstance;
#endif
>>>>>>> 0f959c5 (Add ESP32-POE board support with ethernet and DHCP settings)

  if (Settings::Instance()->BoardType == "lilygo")
  {
    return &lilygoBoard;
  }
  return &genericBoard;
}