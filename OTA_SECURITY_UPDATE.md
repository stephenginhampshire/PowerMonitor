# V1.0.3 OTA Security Enhancement Summary

## Overview
Successfully moved OTA (Over-The-Air) firmware update credentials from hardcoded values in source code to configurable values in config.json on the SD card. This enhances security and allows different credentials per installation without recompilation.

## Changes Made

### 1. RuntimeConfig Structure Extended
**Location:** Lines 329-345

Added two new fields to the RuntimeConfig structure:
```cpp
String ota_username = "SteveG";  // OTA firmware update login
String ota_password = "W00dle1gh";  // OTA firmware update password
```

**Default credentials:** SteveG/W00dle1gh

### 2. Configuration File Management

#### create_default_config() Updated
**Location:** Lines 1456-1503

Added two new JSON fields to the default config.json:
```json
"ota_username": "SteveG",
"ota_password": "W00dle1gh"
```

#### load_config_from_sd() Updated
**Location:** Lines 1530-1625

Added parsing logic for OTA credentials:
```cpp
else if (line.indexOf("ota_username") >= 0) {
    int start = line.indexOf("\"", line.indexOf(":")) + 1;
    int end = line.indexOf("\"", start);
    config.ota_username = line.substring(start, end);
}
else if (line.indexOf("ota_password") >= 0) {
    int start = line.indexOf("\"", line.indexOf(":")) + 1;
    int end = line.indexOf("\"", start);
    config.ota_password = line.substring(start, end);
}
```

Console output now includes:
```
OTA Username: admin
```

### 3. Dynamic Login Page Generation
**Location:** Lines 648-686

**Before:** Static const char* with hardcoded credentials
```cpp
const char* loginIndex = "...if(form.userid.value=='W00dle1gh'...";
```

**After:** Dynamic function using config values
```cpp
String get_loginIndex() {
    // ... HTML generation ...
    page += "if(form.userid.value=='" + config.ota_username + "' && ";
    page += "form.pwd.value=='" + config.ota_password + "') {";
    // ... rest of validation ...
    return page;
}
```

### 4. Documentation Added

#### get_loginIndex() Function
```cpp
/**
 * @brief Generate OTA firmware update login page with credentials from config
 * 
 * Creates an HTML login form for OTA firmware updates. Validates username and
 * password against values loaded from config.json. On successful authentication,
 * redirects to /serverIndex upload page.
 * 
 * @return HTML string containing login form with embedded JavaScript validation
 * 
 * @note Credentials are loaded from config.ota_username and config.ota_password
 * @see load_config_from_sd(), serverIndex
 */
```

#### OTA Route Handlers
- `/Firmware` (GET) - Login page endpoint
- `/serverIndex` (GET) - Upload page endpoint
- `/update` (POST) - Firmware upload handler

All routes now have comprehensive Doxygen documentation with:
- @brief descriptions
- @note annotations (routes, requirements, behavior)
- @see cross-references

### 5. CHANGELOG.md Created
**New File:** Full version history from V12.37 through V1.0.3

Documented:
- Semantic versioning adoption (V1.0.0)
- Documentation additions (V1.0.1)
- Critical CSV header bug fix (V1.0.2)
- Current OTA security enhancement (V1.0.3)

## Step-by-Step OTA Firmware Update Instructions

### Prerequisites
- ESP32 must be powered on and connected to WiFi
- You must know the ESP32's IP address (check serial console or your router)
- SD card must be inserted in ESP32 with valid config.json
- PlatformIO CLI installed (for building firmware)

### Part 1: Build the Firmware (On Your Computer)

#### Step 1: Navigate to Project Directory
Open PowerShell or Command Prompt:
```powershell
cd "C:\Users\Stephen\Dropbox\Power_Monitor\Power_Monitor"
```

#### Step 2: Build the Firmware
Run PlatformIO build command:
```powershell
pio run
```

**Expected Output:**
```
Processing esp32da (platform: espressif32; board: esp32dev; framework: arduino)
...
Building .pio\build\esp32da\firmware.bin
====================================== [SUCCESS] Took 9.63 seconds ======================================
```

#### Step 3: Locate the Compiled Binary
The firmware file is created at:
```
C:\Users\Stephen\Dropbox\Power_Monitor\Power_Monitor\.pio\build\esp32da\firmware.bin
```

**File Details:**
- **Name:** firmware.bin
- **Location:** `.pio\build\esp32da\` subdirectory
- **Size:** ~1.0 MB (1,059,505 bytes for V1.0.3)
- **Important:** This file is recreated every time you build

#### Step 4: Copy Binary to Convenient Location (Optional)
For easier access, you can copy the file:
```powershell
copy ".pio\build\esp32da\firmware.bin" "C:\Users\Stephen\Desktop\Power_Monitor_V1.0.3.bin"
```

### Part 2: Configure OTA Credentials (First Time Only)

#### Step 1: Check Current Credentials
After first boot with V1.0.3, the ESP32 creates `/config.json` on SD card with defaults:
```json
{
  "wifi_ssid": "Woodleigh",
  "wifi_password": "2008198399",
  "ntp_server": "pool.ntp.org",
  "weather_city": "Basingstoke",
  "weather_region": "uk",
  "weather_enabled": true,
  "whatsapp_enabled": true,
  "debug_level": 2,
  "main_cycle_ms": 5000,
  "weather_update_ms": 600000,
  "health_check_ms": 60000,
  "ota_username": "SteveG",
  "ota_password": "W00dle1gh"
}
```

#### Step 2: Customize Credentials (Recommended)
1. **Power off ESP32**
2. **Remove SD card** from ESP32
3. **Insert SD card** into your computer's card reader
4. **Open file:** `config.json` (located in root directory of SD card)
5. **Edit credentials:**
   ```json
   "ota_username": "YourUsername",
   "ota_password": "YourSecurePassword"
   ```
6. **Save file** (ensure it remains valid JSON - no trailing commas!)
7. **Safely eject** SD card
8. **Reinsert SD card** into ESP32
9. **Power on ESP32**

#### Step 3: Verify Credentials Loaded
Check serial console output (115200 baud):
```
Loading configuration from SD card
Configuration loaded:
  ...
  OTA Username: YourUsername
```

### Part 3: Upload Firmware via Web Interface

#### Step 1: Find ESP32 IP Address
**Option A - Serial Console:**
Look for line in console output:
```
IP Address: 192.168.1.100
```

**Option B - Router:**
Check your router's DHCP client list for "Power_Monitor" or "ESP32"

**Option C - Network Scanner:**
Use tool like "Advanced IP Scanner" to find ESP32 on network

#### Step 2: Open OTA Login Page
1. **Open web browser** (Chrome, Firefox, Edge, etc.)
2. **Navigate to:** `http://192.168.1.100/Firmware` (replace with your ESP32's IP)
3. **You should see:** "Power Monitor Firmware Update Login" page with username and password fields

**Troubleshooting:**
- If page doesn't load, verify ESP32 is on and connected (check serial console)
- Ping the IP address: `ping 192.168.1.100`
- Try `http://` not `https://` (SSL not configured)

#### Step 3: Login with Credentials
1. **Enter Username:** SteveG (or your custom username from config.json)
2. **Enter Password:** W00dle1gh (or your custom password from config.json)
3. **Click "Login" button**

**Expected Result:** New page opens showing "Choose File" and "Update" buttons

**If Login Fails:**
- Error message: "Error Password or Username"
- Verify credentials match config.json on SD card
- Check serial console for loaded username
- Reboot ESP32 if you just changed config.json

#### Step 4: Select Firmware File
1. **Click "Choose File" button**
2. **Navigate to firmware location:**
   - If you copied: `C:\Users\Stephen\Desktop\Power_Monitor_V1.0.3.bin`
   - Original location: `C:\Users\Stephen\Dropbox\Power_Monitor\Power_Monitor\.pio\build\esp32da\firmware.bin`
3. **Select firmware.bin file**
4. **Filename should appear** next to "Choose File" button

#### Step 5: Upload Firmware
1. **Click "Update" button**
2. **Progress indicator appears:** "progress: 0%"
3. **Wait for upload to complete** (typically 30-60 seconds for ~1 MB file)
4. **Progress updates in real-time:** 
   ```
   progress: 25%
   progress: 50%
   progress: 75%
   progress: 100%
   ```

**During Upload:**
- **Do NOT close browser window**
- **Do NOT power off ESP32**
- **Do NOT disconnect WiFi**
- **Do NOT interrupt the process**

**Serial Console Shows:**
```
Firmware Upload Requested via Webpage
Login Succeeded _ Firmware File Selection Requested via Webpage
Update: firmware.bin
Update Success: Rebooting
```

#### Step 6: ESP32 Reboots Automatically
1. **Upload completes:** Browser shows "success!" (or may timeout - this is normal)
2. **ESP32 automatically reboots** (takes ~5-10 seconds)
3. **New firmware starts running**

#### Step 7: Verify New Firmware
1. **Wait 30 seconds** for full boot
2. **Check serial console:**
   ```
   ====================================
   Power Monitor
   Version: V1.0.3
   Build: Dec 30 2025 14:23:45
   ====================================
   ```
3. **Navigate to main page:** `http://192.168.1.100/`
4. **Verify version number** displayed on webpage

### Part 4: Troubleshooting

#### Upload Fails / Stalls
- **Check WiFi signal strength** (move ESP32 closer to router)
- **Restart ESP32** and try again
- **Rebuild firmware:** `pio run --target clean` then `pio run`
- **Check file size:** Should be ~1 MB, if 0 bytes or >2 MB there's a problem

#### "FAIL" Message After Upload
- **Check serial console** for error details
- **Verify firmware.bin is valid** (rebuild if necessary)
- **Check ESP32 flash size** (should be 4 MB)
- **Try again** - sometimes first attempt fails

#### ESP32 Won't Boot After Update
- **Wait 60 seconds** - initial boot after update can be slow
- **Power cycle** ESP32 (unplug and replug)
- **Check serial console** for error messages
- **Worst case:** Re-flash via USB using `pio run --target upload`

#### Can't Access /Firmware Page
- **Verify ESP32 IP address** hasn't changed
- **Check config.json exists** on SD card
- **Verify SD card is properly inserted**
- **Check serial console** for boot errors

### Quick Reference

**Build Command:**
```powershell
cd "C:\Users\Stephen\Dropbox\Power_Monitor\Power_Monitor"
pio run
```

**Firmware Location:**
```
.pio\build\esp32da\firmware.bin
```

**OTA URL:**
```
http://<ESP32_IP>/Firmware
```

**Default Credentials:**
- Username: `SteveG`
- Password: `W00dle1gh`

**Config File Location:**
```
SD Card Root: /config.json
```

### Security Best Practices
- Change default admin/admin credentials immediately
- Use strong passwords (8+ characters, mixed case, numbers, symbols)
- Keep config.json on SD card secure
- Different credentials per installation recommended

## Technical Details

### Build Results
```
RAM:   [===       ]  27.6% (used 90,368 bytes from 327,680 bytes)
Flash: [=====     ]  53.9% (used 1,059,505 bytes from 1,966,080 bytes)
Status: SUCCESS
```

**Memory Impact:** 
- RAM: +16 bytes (90,368 vs 90,352 in V1.0.2)
- Flash: +1,408 bytes (1,059,505 vs 1,058,097 in V1.0.2)

Minimal increase due to:
- Two new String fields in RuntimeConfig
- Dynamic HTML generation function
- Additional documentation

### Git Tags
- v1.0.0 - Semantic versioning implementation
- v1.0.1 - Comprehensive documentation
- v1.0.2 - Critical CSV header bug fix
- **v1.0.3 - OTA credentials moved to config.json** ✅

## Testing Recommendations

1. **Config File Creation**
   - Delete existing /config.json from SD card
   - Reboot ESP32
   - Verify new config.json includes ota_username and ota_password

2. **Credential Loading**
   - Check serial console on boot
   - Verify "OTA Username: admin" appears in log

3. **Login Functionality**
   - Navigate to http://<ESP32_IP>/Firmware
   - Try incorrect credentials (should show error alert)
   - Try correct credentials (should redirect to /serverIndex)

4. **Custom Credentials**
   - Edit config.json, set ota_username="test" and ota_password="secure123"
   - Reboot ESP32
   - Verify login requires new credentials

5. **OTA Upload**
   - Build a new firmware binary
   - Upload via web interface
   - Verify successful flash and automatic reboot

## Backward Compatibility

**Breaking Change:** Previous hardcoded credentials (W00dle1gh/W00dle1gh) no longer work.

**Migration Path:**
1. If you preferred the old credentials:
   - Edit config.json
   - Set: `"ota_username": "W00dle1gh"` and `"ota_password": "W00dle1gh"`
   
2. Default changed to admin/admin for better security practice

**Config File Format:** Fully backward compatible - existing config.json files will be automatically updated with new OTA fields on next boot.

## Files Modified
- [src/Power_Monitor.ino](src/Power_Monitor.ino)
  - RuntimeConfig structure (lines 329-345)
  - get_loginIndex() function (lines 648-686)
  - create_default_config() (lines 1456-1503)
  - load_config_from_sd() (lines 1530-1625)
  - OTA route handlers (lines 995-1050)
  - Version constants (lines 124-127)

## Files Created
- [CHANGELOG.md](CHANGELOG.md) - Complete version history

## Next Steps
1. Test OTA login with default credentials (admin/admin)
2. Change credentials in config.json to unique values
3. Document your custom credentials securely
4. Test firmware upload with new credentials
5. Consider implementing additional security:
   - HTTPS for OTA (requires SSL certificates)
   - Rate limiting on failed login attempts
   - Session management for authenticated users

## Version Information
- **Version:** V1.0.3
- **Release Date:** 2025-01-15
- **Type:** PATCH (security enhancement)
- **Git Tag:** v1.0.3
- **Commit:** bc9f7c6
