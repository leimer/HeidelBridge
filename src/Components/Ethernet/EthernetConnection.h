#pragma once

namespace EthernetConnection
{
    // Initializes ethernet with DHCP or static IP
    void Init();

    // Checks if the device is connected to ethernet
    bool IsConnected();

    // Gets the local IP address
    String GetLocalIP();
};
