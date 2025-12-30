# Changelog

All notable changes to the Power Monitor project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
