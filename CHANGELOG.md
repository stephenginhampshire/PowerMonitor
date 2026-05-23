## [1.3.4] - 2026-05-23

### Added
- Persisted software reboot detail across restart using EEPROM (written before intentional reboot, read once on next boot)
- ESP32 hardware reset-cause mapping for reboot diagnostics (POWERON, SOFTWARE, WATCHDOG, BROWNOUT, etc.)

### Changed
- Reboot WhatsApp notification now uses compact structured format:
  - `cause=<esp_reset_reason>`
  - `detail=<stored app reboot reason>` when available
  - `web=<device URL/hostname block>`
- Centralized most software-triggered restarts through a single reboot helper so reason capture is consistent

## [1.3.3] - 2026-05-09

### Changed
- Web UI x-axis now uses stable minute labels (categorical, not datetime) for correct tick spacing at all slider positions
- Vertical gridlines restored for all visible windows
- Web page banner now shows UK date format (DD/MM/YYYY) with time at end

# Changelog

All notable changes to the Power Monitor project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.2] - 2026-04-14

### Added
- Unique scanner-friendly device hostname generated from chip ID (format: `power-monitor-XXXXXX`)
- mDNS HTTP service advertisement for direct hostname access (`http://power-monitor-XXXXXX.local/`)

### Changed
- Daily WhatsApp notifications now include both:
  - Current web IP URL (`http://<ip>/`)
  - mDNS hostname URL (`http://power-monitor-XXXXXX.local/`)
- Reboot WhatsApp notification now includes the same IP + hostname web address block

## [1.3.1] - 2026-02-15

### Fixed
- **CRITICAL**: WiFi crash on boot (Guru Meditation Error) caused by premature web server access
  - Removed `smartDelay()` call during WiFi initialization (server not yet initialized)
  - Removed premature watchdog timer access (Timer0_Cfg NULL during startup)
  - WiFi now connects reliably on first attempt
- WiFi reconnection reliability after firmware uploads
  - Added complete WiFi reset sequence: disconnect(true) → mode(WIFI_OFF) → mode(WIFI_STA)
  - Disabled WiFi sleep mode for better connection stability
  - Extended connection timeout from 60s to 120s (40 attempts × 3s)

### Changed
- Date display format changed from yyyy/mm/dd to dd/mm/yy (British format)
  - Applies to all web page headers and information page statistics
  - Internal date format (yyyy/mm/dd) preserved for file operations and CSV storage
  - Added `Format_Date_For_Display()` helper function for consistent formatting

### Notes
This release resolves critical WiFi stability issues that prevented the system from reconnecting after firmware uploads and fixes a boot crash related to initialization order.

## [1.1.0] - 2026-01-06

### Fixed
- **CRITICAL**: RS485 receive buffer initialization (required_bytes was 0, causing loop exit after 1 byte)
- System now receives full 7-9 byte sensor responses instead of just device address
- Resolves blank webpage issue where all sensor readings showed zero

### Changed
- Blue LED toggles every 2.5 seconds during CSV prefill (was per-record)
- Differentiates prefill activity (fast 2.5s blink) from normal operation (5s per sensor reading)
- Makes prefill progress more visible without overwhelming LED flashing

### Removed
- RS485 TX/RX hex debug output cleaned up for production readiness

### Notes
This is a major milestone release after extensive RS485 debugging (versions 1.0.12-1.0.19). The device is now fully functional with reliable sensor communication.

## [1.0.19] - 2026-01-06

### Fixed
- **CRITICAL**: Receive() function was exiting after 1 byte due to required_bytes starting at 0
- Changed initial required_bytes from 0 to 9 (maximum possible packet size)
- The 3rd byte updates required_bytes to actual value (7 or 9)
- Previous code would exit after 1st byte because (1 < 0) = false
- This bug was preventing all sensor data from being received

## [1.0.18] - 2026-01-06

### Fixed
- **REVERTED**: RS485 RE pin logic back to active-HIGH (module-specific behavior)

## [1.0.17] - 2026-01-06

### Added
- Detailed RS485 communication logging showing hex bytes sent and received
- TX log shows all 8 bytes being transmitted to sensor
- RX log shows response bytes, count, and validation status
- Helps diagnose RS485 wiring issues, wrong sensor address, or protocol mismatches

## [1.0.16] - 2026-01-06

### Added
- Debug console output in Get_Variable_Data() to display actual sensor readings (Voltage, Amperage, Wattage) after each RS485 fetch
- Helps verify RS485 communication is working and shows actual values being written to CSV files
- Output format: "SENSOR: V=XXX.XXV, I=X.XXXA, W=XXX.XW"

## [1.0.15] - 2026-01-06

### Fixed
- **CRITICAL**: Added watchdog timer feeding (timerWrite) in Get_Variable_Data() before and after long operations
- Prevents watchdog timer crashes during RS485 sensor communication (which can take several seconds with retries)
- System now runs stably without unexpected reboots during first loop iteration
- Three watchdog feed points: before long operations, after weather fetch, after sensor fetch

## [1.0.14] - 2026-01-06

### Changed
- Removed test data injection code
- Cleaned up zero-detection logic to check all entries with early exit optimization

## [1.0.13] - 2026-01-06

### Changed
- Enhanced zero-detection in i_Display() to check all data entries with early exit when non-zero value found
- Improved efficiency of RS485 error detection logic

## [1.0.12] - 2026-01-06

### Changed
- Modified RS485 error detection in i_Display() to check last 10 entries instead of just most recent
- More reliable detection of RS485 communication failures

## [1.0.11] - 2026-01-06

### Fixed
- **CRITICAL**: Blank webpage when RS485 sensor communication fails (all readings = 0.00)
- Graph page now detects when all sensor data is zero and displays diagnostic message
- Prevents Google Charts from failing to render logarithmic scale with zero values

### Added
- RS485 troubleshooting message with pin connection checklist:
  - DE pin (GPIO 22) - Driver Enable
  - RE pin (GPIO 25) - Receiver Enable  
  - TX/RX pins
  - KWS sensor power supply
- Link to Information page and system.log for detailed RS485 error checking
- Data record count displayed even when values are zero

## [1.0.10] - 2026-01-06

### Changed
- **UX Improvement**: OTA firmware upload page now automatically handles reboot process
- After successful upload, page polls device every 3 seconds waiting for reboot completion
- Auto-redirects to main page when device comes back online (eliminates manual refresh)
- Shows countdown timer during reboot wait (up to 2 minutes)
- Improved upload page styling with better progress indicators and status messages

### Added
- Visual feedback during entire OTA process: upload → reboot wait → redirect
- File type restriction (.bin only) on file selector
- Timeout handling with helpful error messages

## [1.0.9] - 2026-01-06

### Fixed
- **CRITICAL**: CSV files created with placeholder birthday date (19511118.csv) before NTP time synchronization
- Added time validation check (Year >= 2020) before creating date-based CSV files
- System now waits up to 30 seconds for valid NTP time before creating files
- Added `.log` files to download list - `system.log` now accessible via web interface

### Added
- Time synchronization logging shows attempt count and current year value
- Warning/success messages for NTP sync status during boot

## [1.0.8] - 2026-01-06

### Fixed
- **CRITICAL**: Webpage displaying no data after reboot when CSV file exists but contains no data rows yet
- Added "Waiting for Data..." message with diagnostic information when `Data_Table_Pointer` is 0
- Shows data collection cycle time and refresh reminder to improve user experience

### Added
- Enhanced logging in `Prefill_Data_Array()` function:
  - Now logs `Data_Table_Pointer` status after loading data
  - Displays error message if file fails to open
  - Provides informative note when no data is loaded yet
- Diagnostic information displayed on webpage when waiting for first data collection

## [1.0.7] - 2026-01-06

### Fixed
- **CRITICAL**: Watchdog timer causing system reboot during OTA firmware uploads
- Added `UPLOAD_FILE_ABORTED` handler to properly clean up failed uploads

### Changed
- Watchdog timer now **DISABLED** during firmware upload process (re-enabled on completion/abort/failure)
- Added progress logging every 100KB during OTA upload for better monitoring
- Added WhatsApp notification on successful firmware update

## [1.0.6] - 2026-01-06

### Fixed
- **CRITICAL**: Webpage graph displaying no data after reboot/reinstall
- Removed incorrect `Clear_Arrays()` call that was resetting `Data_Table_Pointer` to 0 after `Prefill_Data_Array()`

## [1.0.5] - 2026-01-01

### Fixed
- **CRITICAL**: Watchdog timer timeout during RS485 communication causing system crashes
- Added watchdog feeding in RS485 receive wait loop to prevent timeout
- Improved RS485 transmission timing with proper flush() and delays
- Moved RS485 mode switching from Receive() to Send_Request() for better timing
- Added RS485 debug output to diagnose communication issues

### Changed
- Increased delays in Send_Request() after transmission (2ms → 10ms) and before receive (5ms → 10ms)
- Enhanced RS485 error logging with hex dump of transmitted/received bytes

## [1.0.4] - 2025-12-31

### Fixed
- String concatenation compilation errors in `i_Config_Upload()` function
- Fixed ambiguous type conversion when mixing F() macros with String variables
- Wrapped F() macros in String() constructors for WiFi SSID, Weather Location, Weather Enabled, WhatsApp Enabled, and Gas Meter Enabled display rows

## [1.0.3] - 2025-01-15

### Security
- Moved OTA firmware update credentials from hardcoded values to config.json
- Added `ota_username` and `ota_password` fields to RuntimeConfig structure
- Credentials now configurable per installation without recompilation

### Changed
- Converted `loginIndex` from const char* to dynamic `get_loginIndex()` function
- Updated `create_default_config()` to include OTA credentials (default: admin/admin)
- Updated `load_config_from_sd()` to read OTA credentials from config.json

### Added
- Comprehensive Doxygen documentation for OTA route handlers:
  - `/Firmware` (GET) - Login page endpoint
  - `/serverIndex` (GET) - Upload page endpoint  
  - `/update` (POST) - Firmware upload handler
- `get_loginIndex()` function with @brief, @return, @note, @see tags

## [1.0.2] - 2025-01-15

### Fixed
- **CRITICAL**: ZipAllFilesToSD() CSV header corrected from 8 to 20 columns
- Changed hardcoded header to use Column_Field_Names array dynamically
- Prevented data misalignment in monthly consolidated files

### Documentation
- Comprehensive sanity check performed on monthly file consolidation logic
- Verified CSV formatting, file operations, and error handling

## [1.0.1] - 2025-01-14

### Added
- Comprehensive Doxygen-style documentation for 30+ functions
- Documentation tags: @brief, @param, @return, @note, @see
- Function groups documented:
  - Core operations (setup, loop, main_programme)
  - Helper functions (get_version_string, console_print, etc.)
  - Logging and diagnostics
  - Configuration management
  - WiFi and networking
  - File operations (CSV, monthly consolidation)
  - RS485 communication
  - Weather integration

## [1.0.0] - 2025-01-14

### Changed
- **BREAKING**: Implemented semantic versioning (MAJOR.MINOR.PATCH)
- Replaced single version string with VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH constants
- Updated all version displays throughout application:
  - Console startup banner
  - Web interface
  - CSV log files
  - System information displays

### Added
- `get_version_string()` helper function with build timestamp
- VERSIONING.md documentation guide
- Semantic versioning constants for programmatic access

### Documentation
- Created comprehensive versioning guide (VERSIONING.md)
- Examples of version bumping scenarios
- Git tagging guidelines

---

## Previous Versions (Pre-Semantic Versioning)

### V12.37 - 2024-01-01
- Multiple incremental improvements and bug fixes
- Stable production release before semantic versioning adoption

### V12.5 - 2023-08-01
- Added OTA (Over-The-Air) firmware update capability
- Web interface for firmware uploads via WiFi

### Earlier Versions
- Various features including:
  - ESP32 power monitoring
  - RS485 Modbus communication
  - SD card logging (daily and monthly files)
  - Weather API integration
  - WhatsApp notifications
  - Web server with file management
  - NTP time synchronization
