#pragma once

class BoardOlimex : public Board
{
public:
  // Constructor
  BoardOlimex();

  // Initializes the board
  virtual void Init();

  // Logs board name/information
  virtual void Print();
  
  // Network capability interfaces
  virtual bool HasWiFi();
  virtual bool HasEthernet();
};
