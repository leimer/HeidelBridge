# OTA Firmware Updates over Ethernet

## Summary
✅ **OTA firmware updates work over the PoE Ethernet connection!**

No special configuration is required. The WebServer and OTA update functionality are network-agnostic and work identically over both WiFi and Ethernet connections.

## Quick Start

### Updating via Ethernet Web Interface

1. **Find your device's IP address:**
   - Check your router's DHCP leases, or
   - View serial monitor output for "ETH Got IP via DHCP", or
   - Use the static IP you configured

2. **Access the update page:**
   ```
   http://<ethernet-ip>/update
   ```

3. **Upload firmware:**
   - Select your `.bin` firmware file
   - Click "Update"
   - Wait for upload and automatic restart

## How It Works

### Network Initialization
When the ESP32-POE-ISO board with Ethernet support boots:

1. **Ethernet First**: System attempts Ethernet connection (DHCP or static IP)
2. **WebServer Starts**: Once Ethernet is connected, WebServer initializes on port 80
3. **All Endpoints Available**: All API endpoints, including `/update`, become accessible

```cpp
// From WifiManager.cpp - WebServer starts on Ethernet
if (EthernetConnection::IsConnected())
{
    Logger::Info("Ethernet connection established");
    WebServer::Instance()->Init();  // ✓ Starts on Ethernet
    return;
}
```

### Network Interface Details

**WebServer Configuration:**
- Binds to: `0.0.0.0:80` (all network interfaces)
- Accepts connections from: WiFi, Ethernet, or both simultaneously
- AsyncWebServer handles all network types identically

**OTA Update Endpoint:**
- Path: `/update` (web interface) or `/api/update` (API)
- Method: POST (multipart/form-data)
- Handler: Network-agnostic firmware upload
- Uses: ESP32 Update library (works on any network interface)

## Update Methods

### Method 1: Web Interface (Recommended)

The web interface provides a user-friendly way to update firmware:

1. Navigate to: `http://<ip>/update`
2. Click "Choose File" and select your firmware binary
3. Click "Update" to begin upload
4. Monitor progress bar
5. Device restarts automatically when complete

### Method 2: API Endpoint (Advanced)

For automation or scripting:

```bash
# Using curl
curl -X POST http://<ip>/api/update \
  -F "update=@.pio/build/olimex/firmware.bin"

# Using Python
import requests
with open('firmware.bin', 'rb') as f:
    files = {'update': f}
    response = requests.post('http://<ip>/api/update', files=files)
    print(response.json())
```

## Building Firmware for OTA

```bash
# Build firmware binary for Olimex ESP32-POE-ISO
platformio run -e olimex

# Binary location:
.pio/build/olimex/firmware.bin

# Upload via web interface or API
```

## Verification Steps

### 1. Check Ethernet Connection
```
Serial Monitor Output:
- "ETH Started"
- "ETH Connected" 
- "ETH Got IP via DHCP"
- "ETH IPv4: 192.168.1.xxx"
```

### 2. Test Web Interface Access
```bash
# Check if update page is accessible
curl http://<ip>/update

# Check version API
curl http://<ip>/api/version
```

### 3. Verify Update Works
- Upload a test firmware binary
- Monitor serial output during upload
- Confirm device restarts
- Verify new firmware is running

## Troubleshooting

### Can't Access Update Page

**Check Physical Connection:**
- Verify Ethernet cable is connected
- Look for link LED on RJ45 port (should be lit)
- Try different cable or port on switch/router

**Verify IP Address:**
- Check router DHCP leases for ESP32-POE-ISO
- View serial monitor for IP address
- Try static IP if DHCP fails

**Network Issues:**
- Ensure device and computer are on same network/VLAN
- Check firewall rules (port 80)
- Try pinging the device IP

### Update Fails

**Firmware Issues:**
- Verify using correct `.bin` file (built for `olimex` environment)
- Check file size (must fit in flash partition)
- Ensure firmware is not corrupted

**Network Issues:**
- Ensure stable Ethernet connection during upload
- Check for network congestion or packet loss
- Verify cable is not damaged

**Device Issues:**
- Check available flash space
- Monitor serial output for specific errors
- Verify partition table is correct

### After Update Issues

**Device Won't Boot:**
- Serial monitor will show error messages
- May need to flash via USB if OTA update corrupted firmware
- Check partition table and firmware size compatibility

**Network Not Working:**
- Ethernet settings persist through OTA updates
- Static IP configuration is preserved
- DHCP will reassign IP (may be different)

## Serial Monitor Output

During a successful OTA update over Ethernet, you should see:

```
[INFO] Ethernet connection established
[INFO] Initializing web server
[INFO] Updating firmware: firmware.bin
[INFO] Update progress: 25%
[INFO] Update progress: 50%
[INFO] Update progress: 75%
[INFO] Update progress: 100%
[INFO] Firmware update complete
[INFO] Restarting...
```

## Security Considerations

**Current Implementation:**
- ⚠️ No authentication on update endpoint
- ✅ Suitable for trusted local networks
- ✅ PoE Ethernet generally more secure than WiFi

**Recommendations:**
- Use on isolated network or VLAN
- Monitor physical access to Ethernet ports
- Consider adding authentication in future (pull requests welcome!)
- Review access logs via serial console

## Feature Comparison

| Feature | WiFi | Ethernet | Notes |
|---------|------|----------|-------|
| OTA Web Interface | ✅ | ✅ | Identical functionality |
| OTA API Endpoint | ✅ | ✅ | Same endpoint path |
| Update Speed | ~100KB/s | ~500KB/s | Ethernet typically faster |
| Reliability | Medium | High | Wired connection more stable |
| Physical Security | N/A | Better | Requires physical access |
| Range | Limited | Cable length | 100m max for Ethernet |

## Conclusion

OTA firmware updates work seamlessly over both WiFi and PoE Ethernet connections with **no special configuration required**. The implementation is network-agnostic, making updates equally accessible regardless of how the device is connected to the network.

For ESP32-POE-ISO boards, Ethernet OTA is often preferred due to:
- ✅ Higher reliability (wired connection)
- ✅ Faster transfer speeds
- ✅ No WiFi interference issues
- ✅ Better physical security

Simply connect via Ethernet, navigate to `http://<ip>/update`, and upload your firmware!
