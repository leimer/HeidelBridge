#include <Arduino.h>
#include <ArduinoJson.h>
#include "../../Configuration/Constants.h"
#include "../../Configuration/Settings.h"
#include "../../Boards/BoardFactory.h"
#include "../../Boards/Board.h"
#include "../Logger/Logger.h"
#include "WifiConnection.h"
#include "../Ethernet/EthernetConnection.h"
#include "CaptivePortal.h"
#include "../WebServer/WebServer.h"
#include "WifiManager.h"

WifiManager *WifiManager::Instance()
{
    static WifiManager instance;
    return &instance;
}

void WifiManager::Start()
{
    Logger::Info("Starting network manager");

    // Try Ethernet first if the board supports it
    if (BoardFactory::Instance()->GetBoard()->HasEthernet())
    {
        EthernetConnection::Init();
        
        // Wait a bit for ethernet to connect
        uint32_t startTimeMs = millis();
        while (!EthernetConnection::IsConnected() && (millis() - startTimeMs < 5000))
        {
            delay(100);
        }
        
        if (EthernetConnection::IsConnected())
        {
            Logger::Info("Ethernet connection established");
            // Start the web server without captive portal
            WebServer::Instance()->Init();
            return;
        }
        else
        {
            Logger::Warning("Ethernet connection failed, falling back to WiFi");
        }
    }

    // Check if WiFi credentials are available and try to connect
    bool isConnectedToWifi = false;
    if (Settings::Instance()->WifiSsid.length() > 0)
    {
        isConnectedToWifi = ConnectToWifiNetwork();
    }

    // Fallback in case no WiFi connection could not be established
    if(!isConnectedToWifi)
    {
        // Start captive portal
        StartCaptivePortal();
    }

    // Start the web server
    WebServer::Instance()->Init();
}

// Starts the captive portal
void WifiManager::StartCaptivePortal()
{
    Logger::Info("Starting captive portal");

    CaptivePortal::Start();
}

// Connects to a known WiFi network
bool WifiManager::ConnectToWifiNetwork()
{
    Logger::Info("Connecting to configured WiFi network");
    
    WifiConnection::Connect(Settings::Instance()->WifiSsid, Settings::Instance()->WifiPassword);

    // Wait for connection
    uint32_t startTimeMs = millis();
    while (!WifiConnection::IsConnected())
    {
        delay(100);
        if (millis() - startTimeMs > Constants::WiFi::ConnectionTimeoutMs)
        {
            WifiConnection::Disconnect();
            Logger::Error(" -> Failed to connect to WiFi network");
            return false;
        }
    }

    Logger::Trace(" -> WiFi connection established");
    return true;
}

// Cyclic processing
void WifiManager::Update()
{
    // If using ethernet, no need for captive portal timeout
    if (BoardFactory::Instance()->GetBoard()->HasEthernet() && 
        EthernetConnection::IsConnected())
    {
        return;
    }

    CaptivePortal::Update();

    // If the captive portal has been running for a while without any activity, restart the ESP32
    if (CaptivePortal::GetUptime() > Constants::WiFi::CaptivePortalTimeoutS * 1000)
    {
        if(!WebServer::Instance()->HadActivity())
        {
            Logger::Error("Captive portal timeout reached, restarting ESP32");
            ESP.restart();
        } 
    }
}