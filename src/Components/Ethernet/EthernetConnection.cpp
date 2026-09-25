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
    bool gEventHandlerRegistered = false;

    static bool HasValidLocalIP()
    {
        return ETH.localIP() != IPAddress(0, 0, 0, 0);
    }

    static bool WaitForLocalIP(uint32_t timeoutMs)
    {
        uint32_t startTimeMs = millis();
        while (!HasValidLocalIP() && (millis() - startTimeMs < timeoutMs))
        {
            delay(100);
        }

        gEthernetConnected = HasValidLocalIP();
        return gEthernetConnected;
    }

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
            Logger::Info(Settings::Instance()->IsEthernetDhcpEnabled ? "ETH Got IP via DHCP" : "ETH Got IP (static)");
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

    // Initialize Ethernet PHY using board-defined configuration
    static void InitializePHY()
    {
        Logger::Debug("Starting ETH PHY with board-defined configuration...");
        ETH.begin();
    }

    // Wait for Ethernet link to establish
    static bool WaitForLink()
    {
        Logger::Debug("Waiting for Ethernet link...");
        uint32_t linkWaitStart = millis();
        const uint32_t linkTimeout = 5000;
        
        while (!ETH.linkUp() && (millis() - linkWaitStart < linkTimeout))
        {
            delay(100);
        }

        if (!ETH.linkUp())
        {
            Logger::Warning("Ethernet link not established (check cable connection)");
            return false;
        }
        
        Logger::Info("Ethernet link UP");
        return true;
    }

    // Configure static IP address
    static bool ConfigureStaticIP()
    {
        Logger::Info("Configuring Ethernet with static IP: %s", Settings::Instance()->EthernetStaticIp.c_str());
        Logger::Info("Gateway: %s, Subnet: %s, DNS: %s", 
            Settings::Instance()->EthernetGateway.c_str(),
            Settings::Instance()->EthernetSubnet.c_str(),
            Settings::Instance()->EthernetDns.c_str());
        
        IPAddress localIP, gateway, subnet, dns;
        if (!localIP.fromString(Settings::Instance()->EthernetStaticIp) ||
            !gateway.fromString(Settings::Instance()->EthernetGateway) ||
            !subnet.fromString(Settings::Instance()->EthernetSubnet) ||
            !dns.fromString(Settings::Instance()->EthernetDns))
        {
            Logger::Error("Invalid static Ethernet IP configuration");
            return false;
        }

        if (ETH.config(localIP, gateway, subnet, dns))
        {
            if (WaitForLocalIP(2000))
            {
                Logger::Info("Static IP configuration successful");
                return true;
            }

            Logger::Error("Static IP configuration did not produce a usable address");
            return false;
        }
        else
        {
            Logger::Error("Failed to configure static IP");
            return false;
        }
    }

    // Configure APIPA (Auto Private IP Addressing) fallback
    static bool ConfigureAPIPAFallback()
    {
        Logger::Warning("DHCP failed, using APIPA address");
        gDhcpFailed = true;
        
        // Generate APIPA address in range 169.254.1.0 - 169.254.254.255
        uint8_t mac[6];
        ETH.macAddress(mac);
        IPAddress apipaIP(169, 254, mac[4], mac[5]);
        IPAddress gateway(169, 254, 0, 1);
        IPAddress subnet(255, 255, 0, 0);
        
        Logger::Info("Configuring APIPA IP: %s", apipaIP.toString().c_str());

        if (ETH.config(apipaIP, gateway, subnet))
        {
            if (WaitForLocalIP(2000))
            {
                Logger::Info("APIPA configuration successful");
                return true;
            }

            Logger::Error("APIPA configuration did not produce a usable address");
            return false;
        }
        else
        {
            Logger::Error("Failed to configure APIPA address");
            return false;
        }
    }

    // Try to obtain IP via DHCP
    static bool TryDHCP()
    {
        Logger::Info("Initializing Ethernet with DHCP");
        
        uint32_t startTime = millis();
        const uint32_t dhcpTimeout = 10000;
        
        while (!gEthernetConnected && !HasValidLocalIP() && (millis() - startTime < dhcpTimeout))
        {
            delay(100);
        }

        gEthernetConnected = gEthernetConnected || HasValidLocalIP();
        return gEthernetConnected;
    }

    // Initializes ethernet with DHCP or static IP
    void Init()
    {
        Logger::Info("Initializing Ethernet...");
        gEthernetConnected = false;
        gDhcpFailed = false;
        
        // Register event handler once
        if (!gEventHandlerRegistered)
        {
            WiFi.onEvent(EthernetEventHandler);
            gEventHandlerRegistered = true;
        }

        // Initialize PHY
        InitializePHY();

        // Wait for link
        if (!WaitForLink())
        {
            return;
        }

        // Configure IP address
        if (!Settings::Instance()->IsEthernetDhcpEnabled)
        {
            // Static IP
            ConfigureStaticIP();
        }
        else
        {
            // DHCP with APIPA fallback
            if (!TryDHCP())
            {
                ConfigureAPIPAFallback();
            }
        }
    }
    
    // Waits for ethernet connection with timeout
    bool WaitForConnection(int timeoutSeconds)
    {
        Logger::Debug("Waiting for Ethernet connection (timeout: %d seconds)...", timeoutSeconds);
        
        uint32_t startTimeMs = millis();
        while (!gEthernetConnected && (millis() - startTimeMs < timeoutSeconds * 1000))
        {
            delay(100);
        }
        
        return gEthernetConnected;
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
