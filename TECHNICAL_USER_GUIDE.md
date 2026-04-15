# Power Monitor Technical User Guide
## Version 1.1.1

## Changelog

### Version 1.1.1 (April 15, 2026)
- **Web Chart Improvements**:
  - Dynamic linear Y-axis scaling for improved detail in the normal 2-4A range
  - Horizontal slider retained for panning older/newer data windows
- **Feature Cleanup**:
  - Gas monitoring disabled in runtime logic
  - CSV file format preserved for backward compatibility
- **OTA Reliability**:
  - Upload response completes before reboot to reduce false failure reports
  - Browser reconnect flow handles reboot window more reliably

### Version 1.0.5 (January 5, 2026)
- **RS485 Communication Improvements**:
  - Separated DE (Driver Enable, pin 22) and RE (Receiver Enable, pin 25) control lines
  - Both pins configured as Active HIGH
  - Implemented 3-attempt retry logic with adaptive delays (75ms, 100ms, 125ms)
  - Added per-field error tracking and statistics reporting
  - Increased inter-request delay from 50ms to 75ms for improved reliability
- **System Enhancements**:
  - Watchdog timer fully operational with proper refresh in all functions
  - Blue LED (pin 4) now flashes on SD card write operations (100ms visible pulse)
  - WhatsApp reboot notifications active
- **Web Interface**:
  - Fixed menu wrapping issue - all items now fit on single line
  - Reduced menu padding from 25px to 15px
  - Fixed HTML typo in ZipAllFiles menu link

## 1. System Overview

### 1.1 Purpose
The Power Monitor is a comprehensive energy monitoring system that:
- Monitors single-phase or three-phase AC power consumption
- Logs voltage, current, power, energy (kWh), and temperature data
- Stores data to SD card in CSV format (daily and monthly files)
- Provides real-time web interface for monitoring and control
- Sends WhatsApp notifications for critical events
- Integrates weather data for environmental correlation
- Supports over-the-air (OTA) firmware updates

### 1.2 Key Features
- **Real-time Monitoring**: 5-second polling interval (configurable)
- **Data Logging**: CSV files with 20 data fields per record
- **Web Interface**: Full-featured HTTP server with multiple pages
- **Chart Navigation**: Dynamic linear Y-axis scaling and horizontal time-window slider
- **Remote Notifications**: WhatsApp integration via CallMeBot API
- **Weather Correlation**: OpenWeatherMap API integration
- **File Management**: Download, delete, and consolidate data files
- **OTA Updates**: Wireless firmware updates via web interface
- **Automatic Recovery**: Self-healing WiFi and SD card error handling

### 1.3 System Specifications
- **Microcontroller**: ESP32-WROOM-DA (240MHz, 320KB RAM, 4MB Flash)
- **Power Meter**: KWS-AC301L-100A (RS485/MODBUS interface)
- **Communication**: 
  - RS485 at 9600 baud (to power meter)
  - WiFi 802.11 b/g/n (to network)
  - Serial console at 115200 baud (debugging)
- **Storage**: MicroSD card (FAT32, recommended 4GB+)
- **Power**: 5V via USB or external supply

## 2. Theory of Operation

### 2.1 Main Program Flow

**POWER-ON RESET → setup() Function:**
- Initialize serial console (115200 baud)
- Configure GPIO pins (LEDs, switches, RS485 enable)
- Connect to WiFi (retry with automatic recovery)
- Start web server (routes: /, /Firmware, /Display, etc.)
- Initialize RS485 port (9600 baud)
- Initialize SD card (retry up to 3 times)
- Load config.json from SD card
- Synchronize time via NTP
- Load weather data (if enabled)
- Open or create daily CSV data file
- Send WhatsApp boot notification

**loop() Function (continuous):**
- Check timing intervals (main_cycle_ms = 5000ms)
- Call main_programme() every 5 seconds
- Handle web server requests
- Monitor WiFi connection status
- Update health check timer

**main_programme() Function (every 5 seconds):**
1. Check for day rollover (create new CSV file)
2. Check for month rollover (trigger month-end process)
3. Send 8 RS485 requests to KWS meter:
   - Voltage (0x0E)
   - Current (0x0F)
   - Watts (0x11)
   - Uptime (0x19)
   - kWh (0x17)
   - Power Factor (0x1D)
   - Frequency (0x1E)
   - Temperature (0x1A)
4. Parse and store responses in Readings_Table
5. Update min/max values (lowest voltage, highest current)
6. Append data record to daily CSV file
7. Verify write operation
8. Update weather data (if interval elapsed)
9. Monitor SD card space (warn if <10% free)
10. Loop back to loop() function

### 2.2 Data Acquisition Cycle

Every 5 seconds (configurable via `config.main_cycle_ms`):

1. **RS485 Transaction** (per request, with retry logic):
   - Enable RS485 transmitter (GPIO 22 DE=HIGH, GPIO 25 RE=HIGH)
   - Send 8-byte request packet (2ms delay between bytes)
   - Disable transmitter (GPIO 22 DE=LOW)
   - Enable receiver (GPIO 25 RE=HIGH for RX)
   - Wait for response (timeout: 500ms)
   - Parse response packet
   - Extract data value
   - Apply scaling factor
   - **Retry Logic**: Up to 3 attempts per request with adaptive delays (75-125ms)
   - **Error Tracking**: Per-field failure statistics reported every 10 failures

2. **Data Processing**:
   - Store in `Readings_Table[8]` structure array
   - Calculate derived values (averages, trends)
   - Update min/max tracking variables
   - Format values for CSV output

3. **Storage**:
   - Flash blue LED (SD activity indicator)
   - Open daily CSV file in append mode
   - Write comma-separated record (20 fields)
   - Verify write operation
   - Close file
   - Update file size tracking

### 2.3 Timing and Intervals

|        Task         | Default Interval  | Configurable |          Purpose          |
|---------------------|-------------------|--------------|---------------------------|
|      Main cycle      |   5000ms (5 sec)  |     Yes      |    Data sampling rate     |
|   Weather update     | 600000ms (10 min) |     Yes      |   API query frequency     |
|    Health check      |  60000ms (1 min)  |     Yes      |   System diagnostics      |
|     WiFi retry       | 30000ms (30 sec)  |      No      |   Connection recovery     |
|   SD retry delay     |   1000ms (1 sec)  |      No      |  File operation retry     |
| WhatsApp debounce    |   1000ms (1 sec)  |      No      |    Message rate limit     |
| WhatsApp dup block   | 300000ms (5 min)  |      No      |      Prevent spam         |

## 3. Hardware Architecture

### 3.1 ESP32 Pin Assignments

| GPIO Pin |    Function    | Direction |          Description           |
|----------|----------------|-----------|--------------------------------|
|    2     |  Onboard LED   |   Output  |    ESP32 module status LED     |
|    4     |   Blue LED     |   Output  |  SD card activity indicator    |
|    5     |   SS (CS)      |   Output  |   SD card SPI chip select      |
|    16    |   RS485 RX     |   Input   | RS485 receive from KWS meter   |
|    17    |   RS485 TX     |   Output  | RS485 transmit to KWS meter    |
|    18    |      SCK       |   Output  |     SD card SPI clock          |
|    19    |     MISO       |   Input   |     SD card SPI data in        |
|    21    |  Reserved      |   Input   | Reserved (gas feature disabled) |
|    22    |  RS485 DE      |   Output  | RS485 Driver Enable (TX ctrl)   |
|    23    |     MOSI       |   Output  |    SD card SPI data out        |
|    25    |  RS485 RE      |   Output  | RS485 Receiver Enable (RX ctrl) |
|    26    |   Green LED    |   Output  |      Status indicator          |
|    27    | Yellow Switch  |   Input   |     User input button          |
|    32    |  Blue Switch   |   Input   |     User input button          |
|    33    |   Red Switch   |   Input   |     User input button          |

### 3.2 Hardware Connections

#### KWS-AC301L-100A Power Meter
- **Interface**: RS485 (differential signaling, A/B twisted pair)
- **Protocol**: MODBUS RTU variant
- **Baud Rate**: 9600 bps, 8N1 (8 data bits, no parity, 1 stop bit)
- **Slave Address**: 0x02
- **Wiring**:
  - ESP32 GPIO 17 (TX) → RS485 transceiver DI
  - ESP32 GPIO 16 (RX) → RS485 transceiver RO
  - ESP32 GPIO 22 (DE) → RS485 transceiver DE (Driver Enable - Active HIGH)
  - ESP32 GPIO 25 (RE) → RS485 transceiver RE (Receiver Enable - Active HIGH)
  - RS485 A → KWS meter A terminal
  - RS485 B → KWS meter B terminal
- **Note**: DE and RE are separate pins (both Active HIGH). During TX: both HIGH. During RX: DE=LOW, RE=HIGH.

#### SD Card Module
- **Interface**: SPI (Serial Peripheral Interface)
- **Format**: FAT32
- **Recommended Size**: 4GB or larger
- **Wiring**:
  - GPIO 5 (SS)  SD card CS
  - GPIO 18 (SCK)  SD card CLK  
  - GPIO 19 (MISO)  SD card DO
  - GPIO 23 (MOSI)  SD card DI
  - 3.3V  SD card VCC
  - GND  SD card GND

## 4. Software Architecture

### 4.1 Data Structures

#### RuntimeConfig Structure
Location: Lines 329-345 in Power_Monitor.ino

```cpp
struct RuntimeConfig {
    String wifi_ssid;              // WiFi network name
    String wifi_password;          // WiFi password
    String ntp_server;             // Time server (e.g., pool.ntp.org)
    String weather_city;           // City for weather data
    String weather_region;         // Region code (e.g., "uk")
    bool weather_enabled;          // Enable/disable weather fetching
    bool whatsapp_enabled;         // Enable/disable WhatsApp notifications
    int debug_level;               // 0=none, 1=errors, 2=info, 3=verbose
    unsigned long main_cycle_ms;   // Data sampling interval (default: 5000)
    unsigned long weather_update_ms; // Weather update interval (default: 600000)
    unsigned long health_check_ms; // Health check interval (default: 60000)
    String ota_username;           // OTA login username (default: "SteveG")
    String ota_password;           // OTA login password (default: "W00dle1gh")
};
```

#### Readings_Table Structure
Stores sensor data from KWS meter:

```cpp
struct Readings_Table_struct {
    double voltage;                // Volts (V)
    double amperage;               // Amps (A)
    double wattage;                // Watts (W)
    double uptime;                 // Operating minutes
    double kilowatthour;           // Energy consumption (kWh)
    double power_factor;           // 0.00 - 1.00
    double frequency;              // Hertz (Hz)
    double temperature;            // Celsius (C)
};

Readings_Table_struct Readings_Table[8];  // Ring buffer for 8 samples
```

### 4.2 Memory Usage (V1.1.1)

```
RAM:   [===       ]  28.2% (used 92,316 bytes from 327,680 bytes)
Flash: [======    ]  56.2% (used 1,104,137 bytes from 1,966,080 bytes)
```

## 5. Web Interface Guide

The system provides a comprehensive web interface accessible at `http://<ESP32_IP>/`

### 5.1 Available Routes

|          Route           | Method |              Description              |    Authentication     |
|--------------------------|--------|---------------------------------------|-----------------------|
|            `/`           |  GET   |   Main dashboard (redirects to /Display)  |          No           |
|       `/Display`         |  GET   | Real-time monitoring with current readings|          No           |
|     `/Information`       |  GET   | System info (version, uptime, memory, WiFi)|         No           |
|    `/DownloadFiles`      |  GET   | File download interface - list and select |          No           |
|       `/GetFile`         |  GET   |    Download selected file (CSV or ZIP)    |          No           |
|     `/DeleteFiles`       |  GET   | File deletion interface - list and select |          No           |
|       `/DelFile`         |  POST  |         Delete selected file              |          No           |
|  `/DeletePreviousMonth`  |  GET   |  Bulk delete previous month's daily files |          No           |
|`/ConfirmDeletePrevMonth` |  POST  |         Confirm bulk deletion             |          No           |
|        `/Reset`          |  GET   |       Remotely reboot the ESP32           |          No           |
|      `/WhatsApp`         |  GET   |      Send test WhatsApp message           |          No           |
|    `/ConfigUpload`       |  GET   | Configuration file upload interface       |          No           |
|    `/DownloadConfig`     |  GET   |  Download current config.json to PC       |          No           |
|    `/UploadConfig`       |  POST  | Upload new config.json from PC and reboot |          No           |
|      `/Firmware`         |  GET   |            OTA login page                 | **Yes - OTA creds**   |
|     `/serverIndex`       |  GET   |         OTA file upload page              | **Yes - Post-login**  |
|       `/update`          |  POST  |       OTA firmware upload handler         | **Yes - Post-login**  |

### 5.2 Main Dashboard (/Display)

**Displays:**
- Current Date and Time (synchronized via NTP)
- Current chart with dynamic linear Y-axis scaling for improved low-range visibility
- Horizontal slider for panning through time windows
- Real-time sensor readings:
  - Voltage (V)
  - Current (A)
  - Power (W)
  - Energy (kWh)
  - Power Factor
  - Frequency (Hz)
  - Temperature (C)
- Min/Max statistics:
  - Lowest/Highest Voltage with timestamps
  - Highest Current with timestamp
- Weather information (if enabled):
  - Temperature (current, feels like, min, max)
  - Atmospheric pressure
  - Humidity
  - Wind speed and direction
- System status:
  - WiFi signal strength
  - SD card usage
  - Uptime

**Buttons:**
- Information - View system details
- Download Files - Access data files
- Delete Files - Manage storage
- Delete Previous Month - Bulk cleanup
- Send WhatsApp - Test notifications
- Reset - Reboot system
- Firmware Update - OTA upload

### 5.3 Information Page (/Information)

**Displays:**
- Software version (e.g., V1.1.1)
- Build timestamp
- ESP32 hardware details
- RAM usage (bytes and percentage)
- Flash usage (bytes and percentage)
- WiFi SSID and signal strength (RSSI)
- IP address
- MAC address
- SD card status and space
- Configuration settings
- Last boot time

### 5.4 File Management Pages

#### Download Files (/DownloadFiles)
- Lists all CSV and ZIP files on SD card
- Shows file sizes
- Click filename to download
- Supports daily files (DF_YYYYMMDD.csv) and monthly files (MF_MM.zip)

#### Delete Files (/DeleteFiles)
- Lists all files with checkboxes
- Select multiple files for deletion
- Confirmation required before delete
- Shows freed space after deletion

### 5.5 Remote Control Pages

#### Reset (/Reset)
- Sends WhatsApp notification: "Resetting Processor from Webpage at HH:MM:SS"
- Performs ESP.restart() after 2 seconds
- System reboots and resumes normal operation

#### WhatsApp Test (/WhatsApp)
- Sends test message: "Sending Test Message from Webpage at HH:MM:SS"
- Confirms WhatsApp API connectivity
- Useful for troubleshooting notifications

## 6. OTA Firmware Updates

For complete OTA instructions, see OTA_SECURITY_UPDATE.md

### 6.1 Quick Reference

**Access:** `http://<ESP32_IP>/Firmware`

**Default Credentials:**
- Username: `SteveG`
- Password: `W00dle1gh`

**Build Firmware:**
```powershell
cd "C:\Users\Stephen\Documents\Power Monitor\Power_Monitor"
pio run
```

**Firmware Location:**
```
.pio\build\esp32da\firmware.bin
```

**Upload Process:**
1. Navigate to /Firmware
2. Enter credentials
3. Select firmware.bin file
4. Click Update
5. Wait for upload completion and success message
6. ESP32 automatically reboots
7. Browser may briefly disconnect during reboot, then reconnect

## 7. Configuration System

### 7.1 config.json File

**Location:** `/config.json` (SD card root directory)

**Format:** JSON (human-readable, editable with any text editor)

**Sample Configuration:**
```json
{
  "wifi_ssid": "Woodleigh",
  "wifi_password": "2008198399",
  "ntp_server": "pool.ntp.org",
  "weather_city": "Basingstoke",
  "weather_region": "uk",
  "weather_enabled": true,
  "whatsapp_enabled": true,
  "gas_meter_enabled": false,
  "debug_level": 2,
  "main_cycle_ms": 5000,
  "weather_update_ms": 600000,
  "health_check_ms": 60000,
  "ota_username": "SteveG",
  "ota_password": "W00dle1gh"
}
```

### 7.2 Configuration Parameters

|     Parameter      |  Type   |     Default     |             Description              |
|--------------------|---------|-----------------|--------------------------------------|
|    wifi_ssid       | String  |  "Woodleigh"    |    WiFi network name (SSID)          |
|  wifi_password     | String  | "2008198399"    |    WiFi password (WPA/WPA2)          |
|    ntp_server      | String  | "pool.ntp.org"  |    NTP time server address           |
|   weather_city     | String  | "Basingstoke"   |    City name for weather data        |
|  weather_region    | String  |      "uk"       | Country/region code (ISO 3166)       |
| weather_enabled    | Boolean |      true       |  Enable/disable weather fetching     |
| whatsapp_enabled   | Boolean |      true       |  Enable/disable notifications        |
| gas_meter_enabled  | Boolean |      false      | Legacy config key (runtime gas feature disabled) |
|   debug_level      | Integer |       2         | 0=none, 1=errors, 2=info, 3=verbose |
|  main_cycle_ms     | Integer |      5000       | Data sampling interval (milliseconds)|
| weather_update_ms  | Integer |     600000      | Weather API query (10 minutes)       |
| health_check_ms    | Integer |     60000       | System health check (1 minute)       |
|   ota_username     | String  |   "SteveG"      |    OTA firmware update login         |
|   ota_password     | String  | "W00dle1gh"     |    OTA firmware update password      |

### 7.3 Gas Meter Configuration

Gas monitoring is currently disabled in runtime logic.

- The **gas_meter_enabled** key is retained for configuration compatibility.
- CSV structure remains unchanged for historical file compatibility.
- Runtime gas tracking and active gas monitoring behavior are disabled.

**Note:** You can keep this key in config.json; it is currently informational.

### 7.4 Modifying Configuration

**Web-Based Method (Recommended):**

1. Navigate to `http://<ESP32_IP>/ConfigUpload`
2. Click **"Download config.json"** to download current configuration to your PC
3. Edit the file on your PC using any text editor (Notepad, VS Code, etc.)
4. Maintain valid JSON format (proper quotes, commas, brackets)
5. Return to `http://<ESP32_IP>/ConfigUpload`
6. Click **"Choose File"** and select your edited config.json
7. Click **"Upload and Reboot"**
8. System will:
   - Validate the file (checks for required fields: wifi_ssid, wifi_password)
   - Backup old config to `config_backup.json`
   - Install new configuration
   - Reboot automatically to apply changes

**Manual SD Card Method (If ESP32 Not Accessible):**

1. Power off ESP32
2. Remove SD card and insert into computer
3. Open `/config.json` with text editor
4. Edit values (maintain valid JSON format!)
5. Save file
6. Safely eject SD card
7. Reinsert into ESP32
8. Power on (new config loaded automatically)

**Important Notes:**
- If config.json is missing or corrupted, the system creates a new default config.json automatically
- Old configuration is backed up to `config_backup.json` when using web upload
- Invalid configurations will be rejected with error message
- ESP32 reboots automatically after successful upload to apply new settings

## 8. KWS-AC301L Protocol

**MODBUS RTU Communication:**
- Baud: 9600, 8N1
- Slave Address: 0x02
- Function: 0x03 (Read Holding Registers)

**8 Active Commands (Lines 560-569):**
| Register | Command |  Data  | Units |      Purpose       |
|----------|---------|--------|-------|--------------------|
|   0x0E   | Voltage |  ÷10   |   V   |   Line voltage     |
|   0x0F   | Current | ÷1000  |   A   |   Current draw     |
|   0x11   |  Power  |  ÷10   |   W   |   Active power     |
|   0x19   | Uptime  |   ×1   |  min  |  Operating time    |
|   0x17   | Energy  | ÷1000  |  kWh  | Cumulative energy  |
|   0x1D   |   PF    |  ÷100  |  0-1  |   Power factor     |
|   0x1E   |  Freq   |  ÷10   |  Hz   |  Line frequency    |
|   0x1A   |  Temp   |   ×1   |  °C   |   Sensor temp      |

**Reset Capability:** ❌ NOT IMPLEMENTED. KWS_Protocol.h shows write password (0x00) exists but no web interface for resetting energy counter. Would require password unlock + write command implementation.

## 9. Data Flow and Storage

**Daily Files:** `DF_YYYYMMDD.csv` (created at midnight)
**Monthly Files:** `MF_MM.csv` (auto-created 1st of month)

**CSV Format (20 fields):**
```
Date,Time,V1,V2,V3,A1,A2,A3,Eff Temp,Wattage,KWH,Hz,Pwr Factor,Time Stamp,Weather Temp,Feels Like,Max Temp,Min Temp,Pressure,Humidity,Wind Speed,Direction,Gas Volume
```

**Sampling:** Every 5 seconds = ~17,280 records/day

**Month-End Process:**
1. Midnight on 1st triggers consolidation
2. All daily files merged into MF_MM.csv
3. Daily files auto-deleted
4. WhatsApp notifications sent
5. ~50MB space freed per month

## 10. Function Reference

**Core Functions:**
- `setup()` - Initialize all subsystems
- `loop()` - Event handler + timing
- `main_programme()` - Data acquisition cycle (every 5 sec)
- `Send_Request()` - RS485 transmit
- `Receive()` - RS485 receive + parse
- `Write_New_Data_Record_to_Data_Files()` - Append CSV
- `Create_New_Data_File()` - New daily file
- `ZipAllFilesToSD()` - Monthly consolidation
- `Send_WhatsApp_Message()` - Notifications
- `Get_Time()` - NTP sync
- `Get_Weather()` - API fetch

**Web Handlers:**
- `i_Display()` - Main dashboard
- `i_Information()` - System info
- `i_Download_Files()` - File list
- `i_Delete_Files()` - File management
- `i_Web_Reset()` - Remote reboot
- `i_WhatsApp()` - Test message
- `get_loginIndex()` - OTA login (dynamic)

## 11. WhatsApp Integration

**CallMeBot API Configuration:**
- Phone: +447785938200
- API Key: 8876034
- Endpoint: https://api.callmebot.com/whatsapp.php

**Hardcoded** (Lines 1843-1845) - NOT in config.json

**Auto Notifications:**
- System boot/reboot
- SD card errors
- File creation
- Month-end process
- WiFi failures
- Low disk space
- Write errors

**Deduplication:** Blocks identical messages within 5 minutes (Issue #13)

**To Change:** Edit source code, rebuild firmware (future: add to config.json)

## 12. Weather Integration

**OpenWeatherMap API:**
- City: Basingstoke, UK (configurable in config.json)
- API Key: 917ddeff21dff2cfc5e57717f809d6ad
- Endpoint: http://api.openweathermap.org/data/2.5/weather
- Update: Every 10 minutes (configurable)

**Data Fields:** Temperature, feels like, min/max, pressure, humidity, wind speed/direction

**Disable:** Set `weather_enabled: false` in config.json

## 13. Troubleshooting

**Won't Boot:** Check power supply (5V 1A), USB cable, try re-flash

**No WiFi:** Verify SSID/password in config.json, use 2.4GHz network, check signal strength

**No SD Card:** Format as FAT32, use 4-32GB card, check insertion, verify SPI wiring

**No Data Logging:** Check RS485 connections (A/B), verify KWS meter powered, check baud rate

**WhatsApp Fails:** Verify phone/API key in code (lines 1843-1845), check internet, test with /WhatsApp button

**OTA Fails:** Verify credentials (config.json), rebuild firmware (`pio run`), check WiFi signal

**Debug:** Set `debug_level: 3` in config.json for verbose console logging (115200 baud)

## 14. Advanced Operations

### KWS Meter Reset

**Status:** ❌ NOT CURRENTLY AVAILABLE

**Research:** KWS_Protocol.h shows write password register (0x00, command 0x37) exists, suggesting write capability for resetting energy counters, but:
- No web interface implemented
- No write functions coded
- Requires password unlock sequence
- Needs thorough testing
- May violate metering regulations

**To Implement:** Would need unlock function + MODBUS write command + web route + safety confirmations

### Remote Access

**Options:**
1. Port forwarding (simple, less secure)
2. VPN (recommended - WireGuard/OpenVPN)
3. Cloud tunnel (ngrok, Cloudflare Tunnel)

### Data Analysis

Download CSV files via /DownloadFiles, analyze in Excel or Python:
```python
import pandas as pd
df = pd.read_csv('DF_20251230.csv')
daily_kwh = df['KWH'].iloc[-1] - df['KWH'].iloc[0]
print(f"Daily: {daily_kwh:.2f} kWh")
```

### Backup Strategy

- Monthly: Download MF_XX.csv files
- Quarterly: Backup config.json
- Annually: Save .pio folder

## Quick Reference Card

```
SERIAL:     115200 baud (console), 9600 baud (RS485)
WEB:        http://<ESP32_IP>/
OTA:        /Firmware → SteveG / W00dle1gh
BUILD:      cd Power_Monitor; pio run
FIRMWARE:   .pio\build\esp32da\firmware.bin
CONFIG:     /config.json on SD card
WHATSAPP:   +447785938200 (hardcoded)
WEATHER:    Basingstoke, UK (config.json)
SAMPLING:   5 seconds (config.json: main_cycle_ms)
CSV FORMAT: 20 fields, ~17,280 records/day
```

**End of Technical User Guide V1.1.1**
