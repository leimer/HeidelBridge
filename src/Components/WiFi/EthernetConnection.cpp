#include <Arduino.h>
#include <ETH.h>
#include "../../Configuration/Constants.h"
#include "../../Configuration/Settings.h"
#include "EthernetConnection.h"
#include "../Logger/Logger.h"

namespace EthernetConnection
{
    bool gEthernetConnected = false;

    void WiFiEvent(WiFiEvent_t event)
    {
        switch (event)
        {
        case ARDUINO_EVENT_ETH_START:
            Logger::Info("ETH Started");
            ETH.setHostname(Settings::Instance()->DeviceName.c_str());
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Logger::Info("ETH Connected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Logger::Info("ETH Got IP");
            Logger::Info("ETH MAC: %s", ETH.macAddress().c_str());
            Logger::Info("ETH IPv4: %s", ETH.localIP().toString().c_str());
            if (ETH.fullDuplex())
            {
                Logger::Info("ETH FULL_DUPLEX");
            }
            Logger::Info("ETH Link Speed: %d Mbps", ETH.linkSpeed());
            gEthernetConnected = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Logger::Warning("ETH Disconnected");
            gEthernetConnected = false;
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Logger::Warning("ETH Stopped");
            gEthernetConnected = false;
            break;
        default:
            break;
        }
    }

    // Initializes ethernet with DHCP or static IP
    void Init()
    {
        Logger::Info("Initializing Ethernet");

        WiFi.onEvent(WiFiEvent);

        // For Olimex ESP32-POE, PHY is LAN8720
        // ETH.begin(PHY_ADDR, PHY_POWER, PHY_MDC, PHY_MDIO, PHY_TYPE, CLK_MODE)
        // Olimex ESP32-POE uses:
        // PHY_ADDR = 0
        // PHY_POWER = -1 (not used, powered externally)
        // PHY_MDC = 23
        // PHY_MDIO = 18
        // PHY_TYPE = ETH_PHY_LAN8720
        // CLK_MODE = ETH_CLOCK_GPIO17_OUT
        ETH.begin(0, -1, 23, 18, ETH_PHY_LAN8720, ETH_CLOCK_GPIO17_OUT);

        if (!Settings::Instance()->IsEthernetDhcpEnabled)
        {
            Logger::Info("Configuring static IP: %s", Settings::Instance()->EthernetIpAddress.c_str());
            
            IPAddress localIP, gateway, subnet, dns;
            localIP.fromString(Settings::Instance()->EthernetIpAddress);
            gateway.fromString(Settings::Instance()->EthernetGateway);
            subnet.fromString(Settings::Instance()->EthernetSubnet);
            dns.fromString(Settings::Instance()->EthernetDns);

            if (!ETH.config(localIP, gateway, subnet, dns))
            {
                Logger::Error("Failed to configure static IP");
            }
        }
        else
        {
            Logger::Info("Using DHCP for IP configuration");
        }
    }

    // Checks if the device is connected to ethernet
    bool IsConnected()
    {
        return gEthernetConnected;
    }

    // Gets the local IP address
    String GetLocalIP()
    {
        return ETH.localIP().toString();
    }
};
