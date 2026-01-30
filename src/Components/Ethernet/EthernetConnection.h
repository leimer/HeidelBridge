#pragma once

namespace EthernetConnection
{
    // Initializes ethernet with DHCP or static IP
    void Init();
    
    // Waits for ethernet connection with timeout
    bool WaitForConnection(int timeoutSeconds = 15);

    // Checks if the device is connected to ethernet
    bool IsConnected();

    // Gets the local IP address
    String GetLocalIP();
};
