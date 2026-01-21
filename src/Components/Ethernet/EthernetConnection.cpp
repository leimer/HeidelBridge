#include <Arduino.h>
#include <ETH.h>
#include "../../Configuration/Constants.h"
#include "../../Configuration/Settings.h"
#include "EthernetConnection.h"
#include "../Logger/Logger.h"

namespace EthernetConnection
{
    bool gEthernetConnected = false;
    bool gDhcpFailed = false;

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
            Logger::Info("ETH Got IP via DHCP");
            Logger::Info("ETH MAC: %s", ETH.macAddress().c_str());
            Logger::Info("ETH IPv4: %s", ETH.localIP().toString().c_str());
            if (ETH.fullDuplex())
            {
                Logger::Info("ETH FULL_DUPLEX");
            }
            Logger::Info("ETH Link Speed: %d Mbps", ETH.linkSpeed());
            gEthernetConnected = true;
            gDhcpFailed = false;
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
        WiFi.onEvent(WiFiEvent);

        // For Olimex ESP32-POE-ISO, PHY is LAN8720A
        // ETH.begin(PHY_ADDR, PHY_POWER, PHY_MDC, PHY_MDIO, PHY_TYPE, CLK_MODE)
        // Olimex ESP32-POE-ISO uses:
        // PHY_ADDR = 0
        // PHY_POWER = -1 (no GPIO control - PHY powered from PoE circuit)
        // PHY_MDC = 23
        // PHY_MDIO = 18
        // PHY_TYPE = ETH_PHY_LAN8720
        // CLK_MODE = ETH_CLOCK_GPIO17_OUT
        //
        // Note: PoE power is managed by Si3402-B chip on the board.
        //       The PHY is always powered when PoE is connected - no GPIO control needed.
        ETH.begin(0, -1, 23, 18, ETH_PHY_LAN8720, ETH_CLOCK_GPIO17_OUT);

        if (!Settings::Instance()->IsEthernetDhcpEnabled)
        {
            // Use static IP configuration
            Logger::Info("Configuring Ethernet with static IP: %s", Settings::Instance()->EthernetStaticIp.c_str());
            
            IPAddress localIP, gateway, subnet, dns;
            localIP.fromString(Settings::Instance()->EthernetStaticIp);
            gateway.fromString(Settings::Instance()->EthernetGateway);
            subnet.fromString(Settings::Instance()->EthernetSubnet);
            dns.fromString(Settings::Instance()->EthernetDns);

            if (ETH.config(localIP, gateway, subnet, dns))
            {
                gEthernetConnected = true;
                Logger::Info("Static IP configuration successful");
                Logger::Info("ETH IPv4: %s", ETH.localIP().toString().c_str());
            }
            else
            {
                Logger::Error("Failed to configure static IP");
            }
        }
        else
        {
            // Use DHCP
            Logger::Info("Initializing Ethernet with DHCP");
            
            // Wait for DHCP to assign an IP (with timeout)
            uint32_t startTime = millis();
            const uint32_t dhcpTimeout = 10000; // 10 seconds timeout for DHCP
            
            while (!gEthernetConnected && (millis() - startTime < dhcpTimeout))
            {
                delay(100);
            }

            // If DHCP failed, configure APIPA address (169.254.x.x)
            if (!gEthernetConnected)
            {
                Logger::Warning("DHCP failed, using APIPA address");
                gDhcpFailed = true;
                
                // Generate APIPA address in range 169.254.1.0 - 169.254.254.255
                // Use last two octets of MAC address for uniqueness
                uint8_t mac[6];
                ETH.macAddress(mac);
                IPAddress apipaIP(169, 254, mac[4], mac[5]);
                IPAddress gateway(169, 254, 0, 1);
                IPAddress subnet(255, 255, 0, 0);
                
                Logger::Info("Configuring APIPA IP: %s", apipaIP.toString().c_str());
                
                if (ETH.config(apipaIP, gateway, subnet))
                {
                    gEthernetConnected = true;
                    Logger::Info("APIPA configuration successful");
                    Logger::Info("ETH IPv4: %s", ETH.localIP().toString().c_str());
                }
                else
                {
                    Logger::Error("Failed to configure APIPA address");
                }
            }
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
