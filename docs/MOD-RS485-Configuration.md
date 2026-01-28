# MOD-RS485 Configuration Guide

Complete configuration guide for the Olimex MOD-RS485 module when used with ESP32-POE-ISO via the UEXT connector.

---

## Quick Fix for Error 224

**If you're getting ModbusRTU error 224 (timeout), the most likely cause is jumper misconfiguration!**

**Solution:** Move the `#SS/SDA` jumper on your MOD-RS485 from `#SS` position to `SDA` position.

This takes 30 seconds and should fix the issue immediately. See [Step-by-Step Instructions](#step-by-step-jumper-configuration) below.

---

## Table of Contents

1. [Pin Mapping Reference](#pin-mapping-reference)
2. [MOD-RS485 Jumper Configuration](#mod-rs485-jumper-configuration)
3. [Configuration Options](#configuration-options)
4. [Step-by-Step Instructions](#step-by-step-jumper-configuration)
5. [Troubleshooting](#troubleshooting)
6. [Technical Reference](#technical-reference)

---

## Pin Mapping Reference

### UEXT Connector Pinout

The UEXT connector is a 10-pin universal extension interface standard.

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | VCC | 3.3V Power Supply |
| 2 | GND | Ground |
| 3 | TXD | UART Transmit Data |
| 4 | RXD | UART Receive Data |
| 5 | SCL | I2C Clock |
| 6 | SDA | I2C Data |
| 7 | - | Not Connected |
| 8 | - | Not Connected |
| 9 | SCK | SPI Clock |
| 10 | #SS | SPI Chip Select |

### ESP32-POE-ISO UEXT Pin Mapping

| UEXT Pin | Signal | ESP32 GPIO | HeidelBridge Usage |
|----------|--------|------------|---------------------|
| 1 | VCC | - | Powers MOD-RS485 (3.3V) |
| 2 | GND | - | Common Ground |
| 3 | TXD | **GPIO 4** | **Serial TX → MOD-RS485 DI** |
| 4 | RXD | **GPIO 36** | **Serial RX ← MOD-RS485 RO** |
| 5 | SCL | GPIO 16 | Available (I2C) |
| 6 | SDA | **GPIO 13** | **Used for RTS (Direction Control)** |
| 7 | - | - | Not Connected |
| 8 | - | - | Not Connected |
| 9 | SCK | GPIO 14 | Available (SPI, also SD card) |
| 10 | #SS | GPIO 15 | Available (SPI, also SD card) |

### MOD-RS485 Internal Connections

The MOD-RS485 uses an **ADM3483ARZ** RS485 transceiver chip with the following pins:

| ADM3483 Pin | Function | Description |
|-------------|----------|-------------|
| Pin 1 (RO) | Receiver Output | Connected to UEXT Pin 4 (RXD) |
| Pin 2 (/RE) | Receiver Enable | Controlled by jumper (#SS or SDA) |
| Pin 3 (DE) | Driver Enable | Controlled by jumper (SCK or SCL) |
| Pin 4 (DI) | Driver Input | Connected to UEXT Pin 3 (TXD) |
| Pins 6,7 (A,B) | RS485 Bus | Connected to screw terminals |

---

## MOD-RS485 Jumper Configuration

The MOD-RS485 has three jumpers that control its operation:

### 1. ENABLE_RT Jumper

**Function:** Enables 120Ω termination resistor on RS485 bus

**Default Position:** **CLOSED** (termination enabled)

**Recommendation:** Keep CLOSED for proper RS485 operation

**When to Change:**
- If multiple devices on bus, only endpoints should have termination
- If you have termination resistors elsewhere, you may open this

### 2. SCL/SCK Jumper

**Function:** Selects which UEXT pin controls DE (Driver Enable)

**Positions:**
- **SCL:** UEXT Pin 5 (GPIO 16) → DE
- **SCK:** UEXT Pin 9 (GPIO 14) → DE **(DEFAULT)**

**Default Position:** **SCK**

**Impact:** Determines which GPIO controls transmit mode

### 3. #SS/SDA Jumper (CRITICAL!)

**Function:** Selects which UEXT pin controls /RE (Receiver Enable)

**Positions:**
- **#SS:** UEXT Pin 10 (GPIO 15) → /RE **(DEFAULT)**
- **SDA:** UEXT Pin 6 (GPIO 13) → /RE **(REQUIRED FOR HEIDELBRIDGE)**

**Default Position:** **#SS**

**IMPORTANT:** For HeidelBridge, this jumper **MUST** be in **SDA** position!

---

## Configuration Options

### Option 1: Change Jumper (RECOMMENDED) ⭐

**Action:** Move #SS/SDA jumper to SDA position

**Configuration:**
```
ENABLE_RT: CLOSED (default)
SCL/SCK:   SCK (default)
#SS/SDA:   SDA (CHANGED from default)
```

**Result:**
- GPIO 13 (RTS) controls /RE ✓
- Works with current HeidelBridge code
- No code changes needed
- No GPIO conflicts

**Pros:**
- ✅ Simple 30-second hardware change
- ✅ No software modifications needed
- ✅ No GPIO conflicts
- ✅ Recommended solution

**Cons:**
- ❌ Requires physical access to MOD-RS485
- ❌ Non-standard jumper configuration

---

### Option 2: Change Code to GPIO 14

**Action:** Modify code to use GPIO 14 for RTS instead of GPIO 13

**Configuration:**
```
ENABLE_RT: CLOSED (default)
SCL/SCK:   SCK (default)  
#SS/SDA:   #SS (default)
```

**Code Change:**
```cpp
// File: src/Boards/Olimex/BoardOlimex.cpp
// Before:
return new ModbusClientRTU(4, 36, 13);

// After:
return new ModbusClientRTU(4, 36, 14);
```

**Pros:**
- ✅ Uses default jumper positions
- ✅ No hardware modifications

**Cons:**
- ❌ Requires code modification and rebuild
- ❌ GPIO 14 also used for SD card (potential conflict)
- ❌ If SD card present, may not work

---

### Option 3: Dual Pin Control (Advanced)

**Action:** Control DE and /RE separately with two GPIOs

**Configuration:**
```
ENABLE_RT: CLOSED (default)
SCL/SCK:   SCK (default)
#SS/SDA:   #SS (default)
```

**Code Changes:**
- Modify ModbusClientRTU to support dual-pin control
- Use GPIO 14 for DE (Driver Enable)
- Use GPIO 15 for /RE (Receiver Enable)
- Synchronize both pins for transmit/receive modes

**Pros:**
- ✅ Uses default jumper positions
- ✅ Full independent control of DE and /RE
- ✅ Most flexibility

**Cons:**
- ❌ Requires significant code modifications
- ❌ Uses two GPIOs instead of one
- ❌ More complex logic
- ❌ GPIO 14/15 also used for SD card (conflicts)

---

### Comparison Table

| Option | Jumper Change | Code Change | GPIO Used | SD Card Conflict | Difficulty |
|--------|---------------|-------------|-----------|------------------|------------|
| **1: Change Jumper** | Yes (#SS→SDA) | No | GPIO 13 | No | ⭐ Easy |
| **2: Use GPIO 14** | No | Yes | GPIO 14 | Yes | ⭐⭐ Medium |
| **3: Dual Control** | No | Major | GPIO 14+15 | Yes | ⭐⭐⭐ Hard |

**Recommendation:** **Option 1** - Change the jumper. It's quick, simple, and has no downsides.

---

## Step-by-Step Jumper Configuration

### Required Tools
- None (jumper can be moved by hand)

### Procedure

**1. Power Off Everything**
```
- Disconnect USB cable from ESP32-POE-ISO
- Disconnect Ethernet cable (or ensure PoE is off)
- Disconnect RS485 cable from MOD-RS485
- Wait 10 seconds for capacitors to discharge
```

**2. Locate MOD-RS485 Module**
```
- Find the MOD-RS485 board
- It should be connected to ESP32-POE-ISO via UEXT connector
- May need to gently remove it from connector for easier access
```

**3. Identify #SS/SDA Jumper**
```
- Look for a 3-pin header labeled "#SS/SDA"
- Should have a small black jumper cap on it
- Default position: covering pins labeled "#SS" and center pin
```

**4. Move Jumper**
```
- Gently pull jumper cap straight up to remove
- Move it to cover pins labeled "SDA" and center pin
- Ensure jumper is firmly seated (press down gently)
```

**5. Verify Other Jumpers**
```
- ENABLE_RT: Should be CLOSED (jumper installed)
- SCL/SCK: Should be in SCK position
- If not, adjust to match default configuration
```

**6. Reconnect and Test**
```
- Reinsert MOD-RS485 into UEXT connector if removed
- Ensure firm connection
- Connect RS485 cable (A→A, B→B, GND→GND)
- Connect power (PoE or USB)
- Monitor serial output for success
```

---

## Troubleshooting

### How to Verify Jumper Positions

**Visual Inspection:**
1. Look at each jumper
2. Note which two pins are covered
3. Compare to labels on board

**Electrical Testing (Advanced):**
1. Power off everything
2. Use multimeter in continuity mode
3. Check connections:
   - With #SS/SDA in SDA: UEXT Pin 6 should have continuity to ADM3483 Pin 2
   - With #SS/SDA in #SS: UEXT Pin 10 should have continuity to ADM3483 Pin 2

### Common Issues

**Issue:** Still getting error 224 after moving jumper

**Checks:**
1. Verify jumper is in SDA position (not #SS)
2. Ensure jumper is firmly seated
3. Check UEXT connector is fully inserted
4. Verify RS485 cable connections (A, B, GND)
5. Check wallbox is powered on
6. Try swapping A and B wires (polarity)

**Issue:** Uncertain which position jumper is in

**Solution:**
- Take photo of jumper before moving
- Look for silk screen labels on board
- SDA position is usually away from edge of board
- When in doubt, try both positions (can't damage anything)

**Issue:** Jumper won't stay in place

**Solution:**
- Replace jumper cap (may be loose)
- Use needle-nose pliers to gently squeeze jumper sides
- Ensure pins are straight and not bent

---

## Technical Reference

### RS485 Transceiver Operation

**ADM3483ARZ Pin Functions:**

**/RE (Receiver Enable) - Pin 2:**
- Active LOW: Receiver enabled (receive mode)
- Logic HIGH: Receiver disabled

**DE (Driver Enable) - Pin 3:**
- Logic HIGH: Driver enabled (transmit mode)
- Active LOW: Driver disabled

**Typical Control:**
- Both pins controlled together by single GPIO (RTS)
- LOW = Receive mode (/RE=0, DE=0)
- HIGH = Transmit mode (/RE=1, DE=1)

### Why Separate Jumpers?

The MOD-RS485 provides separate jumpers for DE and /RE to support:

1. **Different microcontrollers** with varying pin availability
2. **Independent control** if needed for advanced applications
3. **Flexibility** in UEXT connector usage

For most applications (including HeidelBridge), they work together.

### RS485 Bus Termination

**120Ω Termination:**
- Required at both ends of RS485 bus
- Prevents signal reflections
- Improves signal quality

**HeidelBridge Configuration:**
- MOD-RS485: ENABLE_RT should be CLOSED (120Ω enabled)
- Wallbox: Usually has internal termination
- This provides proper termination at both ends

### Signal Timing

**Half-Duplex Communication:**
- RS485 is half-duplex (one direction at a time)
- RTS pin switches between transmit and receive
- Small delay needed between mode switches
- Hardware handles timing automatically

**Timing Sequence:**
1. Set RTS HIGH (transmit mode)
2. Wait ~1ms for transceiver to switch
3. Send data via TXD
4. Wait for transmission complete
5. Set RTS LOW (receive mode)
6. Wait ~1ms for transceiver to switch  
7. Listen for response on RXD

---

## References

- **MOD-RS485 Product Page:** https://www.olimex.com/Products/Modules/Interface/MOD-RS485/
- **ADM3483 Datasheet:** Analog Devices 3.3V RS485 Transceiver
- **UEXT Specification:** https://www.olimex.com/Products/Modules/UEXT/
- **RS485 Standard:** TIA/EIA-485-A

---

## Revision History

| Date | Version | Changes |
|------|---------|---------|
| 2026-01-28 | 1.0 | Initial documentation |

---

**Need Help?** See [docs/Troubleshooting-ModbusRTU.md](Troubleshooting-ModbusRTU.md) for additional assistance.
