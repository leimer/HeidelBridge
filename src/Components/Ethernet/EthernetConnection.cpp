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

    void EthernetEventHandler(WiFiEvent_t event)
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
        Logger::Info("Initializing Ethernet...");
        
        // Register Ethernet event handler
        // Note: WiFi.onEvent() is used for both WiFi and Ethernet events in ESP32
        WiFi.onEvent(EthernetEventHandler);

        // For Olimex ESP32-POE-ISO with ETH8720 chip (LAN8720 compatible)
        // ETH.begin(PHY_ADDR, PHY_POWER, PHY_MDC, PHY_MDIO, PHY_TYPE, CLK_MODE)
        // 
        // Official pin configuration from ESP32-POE-ISO manual:
        // PHY_ADDR = 0 (PHY address on MDIO bus)
        // PHY_POWER/RESET = 12 (GPIO 12 is PHY reset pin)
        // PHY_MDC = 23 (Management Data Clock)
        // PHY_MDIO = 18 (Management Data I/O)
        // PHY_TYPE = ETH_PHY_LAN8720 (ETH8720 is LAN8720 compatible)
        // CLK_MODE = ETH_CLOCK_GPIO0_IN (50MHz clock input from external oscillator on GPIO0)
        //
        // Note: PoE power is managed by Si3402-B chip. The PHY_POWER parameter (GPIO12)
        //       is used for PHY RESET control, not power control. Proper reset sequence
        //       during initialization ensures reliable PHY startup.
        Logger::Debug("Starting ETH PHY (ETH8720/LAN8720A)...");
        ETH.begin(0, 12, 23, 18, ETH_PHY_LAN8720, ETH_CLOCK_GPIO0_IN);

        // Wait for link to come up
        Logger::Debug("Waiting for Ethernet link...");
        uint32_t linkWaitStart = millis();
        const uint32_t linkTimeout = 5000; // 5 seconds for link
        while (!ETH.linkUp() && (millis() - linkWaitStart < linkTimeout))
        {
            delay(100);
        }

        if (!ETH.linkUp())
        {
            Logger::Warning("Ethernet link not established (check cable connection)");
            return;
        }
        
        Logger::Info("Ethernet link UP");

        if (!Settings::Instance()->IsEthernetDhcpEnabled)
        {
            // Use static IP configuration
            Logger::Info("Configuring Ethernet with static IP: %s", Settings::Instance()->EthernetStaticIp.c_str());
            Logger::Info("Gateway: %s, Subnet: %s, DNS: %s", 
                Settings::Instance()->EthernetGateway.c_str(),
                Settings::Instance()->EthernetSubnet.c_str(),
                Settings::Instance()->EthernetDns.c_str());
            
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
                Logger::Info("ETH Gateway: %s", ETH.gatewayIP().toString().c_str());
                Logger::Info("ETH Subnet: %s", ETH.subnetMask().toString().c_str());
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
