# Troubleshooting ModbusRTU Error 224

## Problem Description

ModbusRTU write error 224 occurs during HeidelBridge initialization when trying to configure the Heidelberg Wallbox via RS485/Modbus.

**Typical Error Messages:**
```
[ERROR] ModbusRTU write error: 224
[ERROR] ERROR: Could not set fail safe current
[ERROR] ERROR: Could not configure standby
[ERROR] ERROR: Could not set watchdog timeout
```

## What is Error 224?

**Error Code:** 224 (0xE0)

**Meaning:** Communication timeout/failure
- The ESP32 sent a Modbus command but received no response
- This is NOT a Modbus exception code (those are < 128)
- Indicates the wallbox is not responding to commands

## Root Cause Analysis

Error 224 means the Modbus communication is failing. This happens during the `HeidelbergWallbox::Init()` function which tries to configure three registers:
1. Fail-safe current limit
2. Standby mode setting
3. Watchdog timeout

### Possible Causes

#### 1. **Physical Connection Issues** (Most Common)
- ❌ RS485 cable not connected
- ❌ Wrong cable (should be RJ12 or properly wired RS485)
- ❌ Loose connections in UEXT connector
- ❌ MOD-RS485 module not properly seated
- ❌ Cable polarity reversed (A/B swapped)

#### 2. **Wallbox Not Ready**
- ❌ Wallbox still booting when ESP32 tries to communicate
- ❌ Wallbox in firmware update mode
- ❌ Wallbox powered off or in error state

#### 3. **Configuration Mismatch**
- ❌ Wrong baud rate (default: 19200)
- ❌ Wrong parity (default: Even)
- ❌ Wrong server ID (default: 1)
- ❌ Wrong UART pins configured

#### 4. **Hardware Issues**
- ❌ Defective MOD-RS485 module
- ❌ Damaged RS485 cable
- ❌ Wallbox RS485 interface faulty
- ❌ ESP32 GPIO malfunction

#### 5. **Timing Issues**
- ❌ Commands sent too quickly after power-on
- ❌ Wallbox needs warm-up time
- ❌ Bus contention (multiple devices)

## Hardware Configuration

### ESP32-POE-ISO with MOD-RS485 via UEXT

**Pin Mapping:**
- **GPIO 4** (UEXT Pin 3 - TXD) → MOD-RS485 DI (Driver Input)
- **GPIO 36** (UEXT Pin 4 - RXD) → MOD-RS485 RO (Receiver Output)
- **GPIO 13** (UEXT Pin 6 - SDA) → MOD-RS485 DE+RE (Direction Control)

**Serial Configuration:**
- Baud Rate: 19200 bps
- Data Bits: 8
- Parity: Even
- Stop Bits: 1
- Mode: SERIAL_8E1

**Modbus Settings:**
- Server ID: 1 (Heidelberg Wallbox default)
- Timeout: Configured in Constants.h
- Function Codes: 0x03 (Read), 0x06 (Write Single Register)

### MOD-RS485 Module

**Connection to Wallbox:**
- Pin 1: A (non-inverting)
- Pin 2: B (inverting)
- GND: Common ground

**Important:** RS485 is differential signaling - polarity matters!

## Troubleshooting Steps

### Step 1: Check Physical Connections

1. **Verify UEXT Connection:**
   - MOD-RS485 properly inserted into UEXT connector
   - Module oriented correctly (check pin 1 alignment)
   - Connector fully seated

2. **Check RS485 Cable:**
   - Cable properly connected to wallbox
   - Use shielded twisted-pair cable for best results
   - Maximum cable length: 1200m (but keep reasonable for testing)
   - Check for physical damage

3. **Verify Wallbox Connection:**
   - Wallbox RS485 port correctly identified
   - A and B terminals connected (not swapped)
   - Common ground if required by wallbox model

### Step 2: Verify Wallbox Status

1. **Check Wallbox Power:**
   - Wallbox powered on and operational
   - Status LEDs indicate normal operation
   - Not in error or update mode

2. **Check Wallbox Settings:**
   - Modbus interface enabled (some models require activation)
   - Baud rate set to 19200 (check wallbox manual)
   - Server ID set to 1 (check wallbox manual)

3. **Wait for Boot:**
   - Power cycle wallbox
   - Wait 30-60 seconds for full boot
   - Then power cycle ESP32

### Step 3: Test with Serial Monitor

1. **Enable Debug Logging:**
   - Set logging level to DEBUG or TRACE
   - Watch serial output during initialization

2. **Look for:**
   - "Starting RS485 hardware serial"
   - "Creating Modbus RTU instance"
   - "ModbusRTU write attempt failed" messages

3. **Check Timing:**
   - Note when wallbox Init() is called
   - Wallbox might need delay after power-on

### Step 4: Verify Configuration

1. **Check Constants.h:**
   ```cpp
   ModbusBaudrate = 19200  // Should match wallbox
   ModbusServerId = 1      // Should match wallbox
   ModbusTimeoutMs = ?     // Check if adequate
   ```

2. **Check Board Pins:**
   ```cpp
   GetPinTx() = 4   // UEXT TXD
   GetPinRx() = 36  // UEXT RXD
   GetPinRts() = 13 // UEXT SDA (direction control)
   ```

### Step 5: Test Cable Polarity

**If A and B are swapped:**
- Communication will fail completely
- Error 224 will occur on all commands

**How to Test:**
1. Note which wire goes to A and which to B
2. Try swapping A and B at wallbox connection
3. Power cycle and test again

**Correct Polarity:**
- Refer to wallbox manual for correct A/B assignment
- Some wallboxes label as RS485+ and RS485-
- MOD-RS485: Pin 1 = A (non-inverting), Pin 2 = B (inverting)

### Step 6: Add Startup Delay

If wallbox needs time to boot, add delay before Init():

```cpp
// In Main.cpp or wherever wallbox is initialized
delay(5000);  // Wait 5 seconds for wallbox to boot
HeidelbergWallbox::Instance()->Init();
```

### Step 7: Test with Minimal Configuration

Comment out the three failing writes and test:
```cpp
void HeidelbergWallbox::Init()
{
    // Temporarily commented for testing
    /*
    if (!ModbusRTU::Instance()->WriteHoldRegister16(...))
    {
        Logger::Error("ERROR: Could not set fail safe current");
    }
    */
}
```

Then try to read a register to test basic communication:
- Read charging state register
- If read works but write fails → check wallbox write permissions
- If read also fails → connection/wiring issue

## Advanced Diagnostics

### Use Oscilloscope or Logic Analyzer

If available, capture RS485 signals:
1. **Check TX (GPIO 4):**
   - Verify data being sent
   - Check baud rate accuracy
   - Verify 8E1 format

2. **Check RX (GPIO 36):**
   - No response = wallbox not connected/powered
   - Garbled response = wrong baud/parity
   - Valid response = check DE/RE timing

3. **Check DE/RE (GPIO 13):**
   - Should go HIGH before TX
   - Should stay HIGH during TX
   - Should go LOW after TX completes

### Check with USB-RS485 Adapter

Use a USB-RS485 adapter and PC software:
1. Connect adapter to wallbox (same A/B connections)
2. Use Modbus testing software (e.g., QModMaster, ModbusPoll)
3. Try reading/writing same registers
4. Verify wallbox responds correctly
5. Compare successful PC communication with ESP32

## Common Solutions

### Solution 1: Wait for Wallbox Boot
Add delay before HeidelbergWallbox::Init():
```cpp
delay(10000);  // Wait 10 seconds
```

### Solution 2: Swap A and B
Reverse RS485 polarity at wallbox connection.

### Solution 3: Check Cable
Use known-good RS485 cable, preferably shielded twisted-pair.

### Solution 4: Add Termination Resistor
RS485 might need 120Ω termination resistor:
- At each end of long cables
- Check if wallbox has built-in termination
- MOD-RS485 has onboard termination (can be enabled)

### Solution 5: Increase Timeout
In Constants.h, increase:
```cpp
ModbusTimeoutMs = 2000;  // Try 2 seconds
```

### Solution 6: Power Cycle Sequence
1. Power off wallbox
2. Power off ESP32
3. Wait 10 seconds
4. Power on wallbox
5. Wait 30 seconds for wallbox boot
6. Power on ESP32

## Expected Behavior When Working

**Successful Initialization:**
```
[INFO] Starting RS485 hardware serial
[TRACE] Creating Modbus RTU instance
[DEBUG] Heidelberg wallbox: Initializing fail safe current with 160 (raw)
[DEBUG] Heidelberg wallbox: Initializing standby mode with 4
[DEBUG] Heidelberg wallbox: Setting watch dog timeout to 60 s
```

No error messages = successful configuration.

## Technical Details

### Modbus RTU Frame Format

**Write Single Register (Function Code 0x06):**
```
[Slave ID][Function][Address Hi][Address Lo][Value Hi][Value Lo][CRC Lo][CRC Hi]
```

**Example (Set Fail-Safe Current):**
- Slave ID: 0x01
- Function: 0x06 (Write Single Register)
- Address: 0x0105 (Fail-safe current register)
- Value: 0x00A0 (160 = 16.0A)
- CRC: Calculated

**If wallbox receives this and doesn't respond:**
- ESP32 waits for timeout period
- Returns error 224 (0xE0)

### Error Code Reference

- **224 (0xE0):** Timeout - No response from slave
- **1-4:** Modbus exception codes from slave
- Other codes: Library-specific errors

### GPIO 36 Limitation

GPIO 36 is **input-only** with 2.2k pull-up:
- Perfect for RX (receive only)
- Cannot be used for output
- Pull-up provides defined idle state
- Compatible with RS485 receiver output

## Still Not Working?

If error persists after all troubleshooting:

1. **Hardware Test:**
   - Test MOD-RS485 with known-good device
   - Test wallbox RS485 with USB adapter
   - Verify both work independently

2. **Verify Wallbox Model:**
   - Confirm it's a Heidelberg wallbox
   - Check model number supports Modbus
   - Verify firmware version is recent

3. **Check Wallbox Configuration:**
   - Some models require enabling Modbus in settings
   - Check user manual for Modbus activation procedure
   - Verify no other device is connected to same RS485 bus

4. **Contact Support:**
   - Heidelberg wallbox technical support
   - Olimex for ESP32-POE-ISO/MOD-RS485 issues
   - Provide serial output logs
   - Include hardware configuration details

## Related Documentation

- [ESP32-POE-ISO User Manual](https://www.olimex.com/Products/IoT/ESP32/ESP32-POE-ISO/)
- [MOD-RS485 Documentation](https://www.olimex.com/Products/Modules/Interface/MOD-RS485/)
- [Heidelberg Wallbox Modbus Documentation](https://www.heidelberg-wallbox.com/) (check manufacturer site)
- [eModbus Library](https://github.com/eModbus/eModbus)
