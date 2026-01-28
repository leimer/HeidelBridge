# Build Process Documentation

This document explains the HeidelBridge build process and when you need to run `build-resources.sh`.

## Quick Answer

**To build the firmware, you only need:**

```bash
platformio run -e olimex
```

**You do NOT need to run `build-resources.sh` for normal builds.**

---

## Understanding the Build Process

### 1. C++ Source Files

All C++ source files in the `src/` directory are automatically compiled by PlatformIO:

- `src/Main.cpp` - Entry point
- `src/Components/` - All components (WiFi, Ethernet, WebServer, MQTT, etc.)
- `src/Boards/` - Board-specific implementations
- `src/Configuration/` - Settings and configuration

**PlatformIO handles all of this automatically.** No special steps needed.

### 2. Web Interface Files (Static Resources)

The web interface files (HTML, CSS, JavaScript, images) need special handling because they must be embedded into the firmware binary.

#### How It Works

```
data/index.html          →  data/headers/index.html.h
data/index.js            →  data/headers/index.js.h
data/heidelbridge.css    →  data/headers/heidelbridge.css.h
data/blueberry.svg       →  data/headers/blueberry.svg.h
... etc ...
```

The script `build-resources.sh` converts each file into a C++ header file containing:
- A byte array (`const unsigned char filename[] PROGMEM`)
- A length variable (`const unsigned int filename_len`)

These headers are then included in `src/Components/WebServer/WebServer.cpp`:

```cpp
#include "../../../data/headers/index.html.h"
#include "../../../data/headers/index.js.h"
// ... etc ...

StaticFile staticFiles[] = {
    {"/", "text/html", index_html, index_html_len},
    {"/index.js", "application/javascript", index_js, index_js_len},
    // ... etc ...
};
```

#### Important: Header Files Are Committed to Git

The generated header files in `data/headers/` **are tracked in the git repository**. This means:

✅ **For normal builds:** You don't need to run `build-resources.sh` because the headers already exist.

❌ **Only run it when:** You modify web interface files in the `data/` directory.

---

## When to Run `build-resources.sh`

### Scenarios Where You Need It

1. **Modifying Web Interface**
   ```bash
   # Edit any file in data/ directory
   vim data/index.html
   
   # Regenerate headers
   ./build-resources.sh
   
   # Commit both source and generated files
   git add data/index.html data/headers/index.html.h
   git commit -m "Update web interface"
   ```

2. **Adding New Web Files**
   ```bash
   # Add new file to data/
   cp my-new-file.js data/
   
   # Generate header
   ./build-resources.sh
   
   # Update WebServer.cpp to include and use the new file
   # Commit everything
   ```

3. **Updating Images or Styles**
   ```bash
   # Replace image
   cp new-logo.svg data/blueberry.svg
   
   # Regenerate headers
   ./build-resources.sh
   
   # Commit changes
   ```

### Scenarios Where You DON'T Need It

1. **Building firmware for the first time**
   - Headers already exist in the repository
   - Just run `platformio run -e olimex`

2. **Modifying C++ code**
   - Changes to `src/` directory don't require regenerating headers
   - Just build normally

3. **Modifying documentation**
   - No build process involved

4. **Changing board configuration**
   - Just modify `platformio.ini` and build

---

## Build Commands Reference

### Building Firmware

```bash
# Build for ESP32-POE-ISO (Olimex)
platformio run -e olimex

# Build for standard ESP32
platformio run -e esp32

# Build for LILYGO T-CAN485
platformio run -e lilygo

# Build all environments
platformio run
```

### Flashing Firmware

```bash
# Flash to ESP32-POE-ISO
platformio run -e olimex -t upload

# Flash to standard ESP32
platformio run -e esp32 -t upload
```

### Monitoring Serial Output

```bash
# Monitor serial output
platformio device monitor
```

### Clean Build

```bash
# Clean build artifacts
platformio run -e olimex -t clean

# Clean and rebuild
platformio run -e olimex -t clean && platformio run -e olimex
```

---

## Full Development Workflow

### For C++ Code Changes

```bash
# 1. Edit C++ files
vim src/Components/WebServer/WebServer.cpp

# 2. Build
platformio run -e olimex

# 3. Flash (if needed)
platformio run -e olimex -t upload

# 4. Monitor (if needed)
platformio device monitor
```

### For Web Interface Changes

```bash
# 1. Edit web files
vim data/index.html
vim data/index.js

# 2. Regenerate headers
./build-resources.sh

# 3. Build firmware
platformio run -e olimex

# 4. Flash (if needed)
platformio run -e olimex -t upload

# 5. Test in browser
# Navigate to http://<device-ip>/
```

---

## How `build-resources.sh` Works

The script performs these steps:

1. **Finds all files** in `data/` directory (excluding existing `.h` files)

2. **For each file:**
   - Reads the binary content
   - Converts to hexadecimal byte array using `xxd -i`
   - Wraps in C++ header format with `PROGMEM` attribute
   - Saves to `data/headers/<filename>.h`

3. **Example transformation:**

   **Input:** `data/index.html`
   ```html
   <!doctype html>
   <html>
   <body>Hello</body>
   </html>
   ```

   **Output:** `data/headers/index.html.h`
   ```cpp
   #pragma once
   
   const unsigned char index_html[] PROGMEM = {
     0x3c, 0x21, 0x64, 0x6f, 0x63, 0x74, 0x79, 0x70, 0x65, 0x20, ...
   };
   
   const unsigned int index_html_len = sizeof(index_html);
   ```

4. **Why PROGMEM?**
   - Stores data in flash memory instead of RAM
   - ESP32 has limited RAM but plenty of flash
   - Web files can be large (especially CSS frameworks)

---

## Build System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   HeidelBridge Build                     │
└─────────────────────────────────────────────────────────┘

┌──────────────────┐         ┌──────────────────────────┐
│   C++ Sources    │         │   Web Resources          │
│   src/**/*.cpp   │         │   data/*.html, *.js      │
│   src/**/*.h     │         │   data/*.css, *.svg      │
└────────┬─────────┘         └──────────┬───────────────┘
         │                              │
         │                              │ (manual)
         │                              │ ./build-resources.sh
         │                              ▼
         │                   ┌──────────────────────────┐
         │                   │  Generated Headers       │
         │                   │  data/headers/*.h        │
         │                   │  (committed to git)      │
         │                   └──────────┬───────────────┘
         │                              │
         │◄─────────────────────────────┘
         │            #include
         │
         ▼
┌──────────────────────────────────────────────────────────┐
│               PlatformIO Build System                     │
│                                                           │
│  1. Compiles all C++ files                               │
│  2. Links libraries (AsyncWebServer, ArduinoJson, etc.)  │
│  3. Creates firmware.bin                                 │
└────────────────────────────┬─────────────────────────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │  firmware.bin   │
                    │  (ready to flash)│
                    └─────────────────┘
```

---

## Troubleshooting

### "File not found" error for data/headers/*.h

**Problem:** Build fails with missing header file error.

**Solution:** Run `./build-resources.sh` to generate header files.

### Headers out of date with web files

**Problem:** Web interface shows old content after updating files.

**Solution:**
```bash
./build-resources.sh
platformio run -e olimex -t clean
platformio run -e olimex
```

### build-resources.sh fails

**Problem:** Script fails to run.

**Possible causes:**
- Missing `xxd` utility (install via `sudo apt-get install xxd` on Linux)
- No execute permission (`chmod +x build-resources.sh`)
- Files in `data/` directory have special characters in names

---

## Best Practices

### For Contributors

1. **When modifying web files:**
   - Always run `build-resources.sh` after changes
   - Commit both source files and generated headers
   - Test the web interface before committing

2. **When reviewing pull requests:**
   - Check that header files match source files
   - Verify both are updated together

3. **Don't manually edit header files:**
   - Always edit source files in `data/`
   - Let the script generate headers

### For Users

1. **Normal firmware updates:**
   - Just pull latest code and run `platformio run -e olimex`
   - Header files are already up-to-date in the repository

2. **Customizing web interface:**
   - Fork the repository
   - Modify files in `data/`
   - Run `build-resources.sh`
   - Build and flash your custom firmware

---

## Summary

| Task | Need build-resources.sh? | Command |
|------|--------------------------|---------|
| Build firmware | ❌ No | `platformio run -e olimex` |
| Flash firmware | ❌ No | `platformio run -e olimex -t upload` |
| Modify C++ code | ❌ No | Edit, then build normally |
| Modify web interface | ✅ **YES** | Edit `data/`, run script, then build |
| Add new web files | ✅ **YES** | Add to `data/`, run script, update WebServer.cpp |
| First time build | ❌ No | Headers already in git |

**Bottom line:** `platformio run -e olimex` is sufficient for building the firmware. You only need `build-resources.sh` when modifying the web interface.
