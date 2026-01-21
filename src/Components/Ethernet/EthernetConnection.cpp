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
            return true;
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
            gEthernetConnected = true;
            Logger::Info("APIPA configuration successful");
            Logger::Info("ETH IPv4: %s", ETH.localIP().toString().c_str());
            return true;
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
        
        while (!gEthernetConnected && (millis() - startTime < dhcpTimeout))
        {
            delay(100);
        }

        return gEthernetConnected;
    }

    // Initializes ethernet with DHCP or static IP
    void Init()
    {
        Logger::Info("Initializing Ethernet...");
        
        // Register event handler
        WiFi.onEvent(EthernetEventHandler);

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
