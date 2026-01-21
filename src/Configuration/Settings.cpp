#include <Arduino.h>
#include <Preferences.h>
#include "../Components/Logger/Logger.h"
#include "Constants.h"
#include "Settings.h"

Preferences gPreferences;

Settings *Settings::Instance()
{
    static Settings instance;
    return &instance;
}

void Settings::Init()
{
    Logger::Debug("Initializing persistent settings");
    gPreferences.begin("heidelbridge", false);
}

// Deinitializes the settings
void Settings::DeInit()
{
    Logger::Debug("Deinitializing persistent settings");
    gPreferences.end();
}

// Read all settings from SPIFstorageFS
void Settings::ReadFromPersistentMemory()
{
    Logger::Trace("Reading settings from persistent memory");

    DeviceName = gPreferences.getString("device_name", "HeidelBridge");
    WifiSsid = gPreferences.getString("wifi_ssid");
    WifiPassword = gPreferences.getString("wifi_password");
    IsEthernetDhcpEnabled = gPreferences.getBool("eth_dhcp", true);
    EthernetIpAddress = gPreferences.getString("eth_ip", "192.168.1.250");
    EthernetGateway = gPreferences.getString("eth_gateway", "192.168.1.1");
    EthernetSubnet = gPreferences.getString("eth_subnet", "255.255.255.0");
    EthernetDns = gPreferences.getString("eth_dns", "8.8.8.8");
    IsMqttEnabled = gPreferences.getBool("mqtt_enabled");
    MqttPort = gPreferences.getUShort("mqtt_port", 1883);
    MqttServer = gPreferences.getString("mqtt_server");
    MqttUser = gPreferences.getString("mqtt_user");
    MqttPassword = gPreferences.getString("mqtt_password");
}

// Write all settings to storage
void Settings::WriteToPersistentMemory()
{
    Logger::Trace("Writing settings to persistent memory");

    gPreferences.putString("device_name", DeviceName);
    gPreferences.putString("wifi_ssid", WifiSsid);
    gPreferences.putString("wifi_password", WifiPassword);
    gPreferences.putBool("eth_dhcp", IsEthernetDhcpEnabled);
    gPreferences.putString("eth_ip", EthernetIpAddress);
    gPreferences.putString("eth_gateway", EthernetGateway);
    gPreferences.putString("eth_subnet", EthernetSubnet);
    gPreferences.putString("eth_dns", EthernetDns);
    gPreferences.putBool("mqtt_enabled", IsMqttEnabled);
    gPreferences.putUShort("mqtt_port", MqttPort);
    gPreferences.putString("mqtt_server", MqttServer);
    gPreferences.putString("mqtt_user", MqttUser);
    gPreferences.putString("mqtt_password", MqttPassword);
}

// Prints all settings to the logger
void Settings::Print()
{
    Logger::Debug("Using the following device settings:");

    Logger::Debug(" > Device name: %s", DeviceName.c_str());
    Logger::Debug(" > WiFi SSID: %s", WifiSsid.c_str());
    Logger::Debug(" > Ethernet DHCP enabled: %s", IsEthernetDhcpEnabled ? "yes" : "no");
    Logger::Debug(" > Ethernet IP: %s", EthernetIpAddress.c_str());
    Logger::Debug(" > Ethernet Gateway: %s", EthernetGateway.c_str());
    Logger::Debug(" > Ethernet Subnet: %s", EthernetSubnet.c_str());
    Logger::Debug(" > Ethernet DNS: %s", EthernetDns.c_str());
    Logger::Debug(" > MQTT enabled: %s", IsMqttEnabled ? "yes" : "no");
    Logger::Debug(" > MQTT server: %s", MqttServer.c_str());
    Logger::Debug(" > MQTT port: %d", MqttPort);
    Logger::Debug(" > MQTT user: %s", MqttUser.c_str());
}