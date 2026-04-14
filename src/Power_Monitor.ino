/*------------
Name:		Power_Monitor.ino
Created : 9 / 10 / 2022 12 : 54 : 46 PM
Author : Stephen Gould
Version : 9.0   Compatible with Hardware Schematic V9
Change Record
24/10/2022  1.0 First Release
17/11/2022  9.0 Wipe SD Button added, Wipe SD Functionality added to Webpage
09/12/2022  9.1 Webpage Graphs now display 1 to 50Amps
10/12/2022  9.2 Webpage Reset function added
11/12/2022  9.3 SD saving and Webpage viewing of console messages added
14/12/2022  9.4 Added weather information to data file
16/12/2022  9.5 Add more information to information web page
17/12/2022  9.6 Corrected issue with Delete Files, where operational files were displayed after file removed
20/12/2022  9.7 Corrected an issue where if the date changed new file creation would be continuously repeated
22/12/2022  9.8 Web data arrays cleared when date changed, SD flashes during Preloading
23/12/2022  9.9 Requirement to press start button removed
28/12/2022  9.10 Attempt to recover network if lost (up to 20 attempts = 10 seconds
30/12/2022  9.11 Added autoscaling to web chart display
06/01/2023  10.0 Radical rewrite of Data handling
09/01/2023  10.1 Bug Fix
20/01/2023  11.0 Radical rewrite of Data handling.
21/01/2023  11.1 Coverted receive milli Amperage to Amperage in amps.
25/01/2023  11.2 Removed milliseconds from console records
28/01/2023  11.3 Fixed Time to cope with hours <10 & changed width to show the y axis labels
15/03/2023  11.4 Gas Meter Pulse Output monitoring added, 1 pulse = 0.1 cubic metres of gas used
16/04/2023  11.5 Added creation of monthly file
07/05/2023  11.6 Added watchdog timer, reboots if processor lock or no WiFi for over 20 seconds
07/07/2023  12.0 Removed console file and tidy up
11/07/2023  12.1 Month end routines
14/07/2023  12.2 Deletes old files at end of the Month End Routine
18/07/2023  12.3 If Highest and Lowest Voltage are 0 set them to the first read voltage
18/07/2023  12.4 Now sorts the files into date order for Download and Delete
02/08/2023  12.5 Added Over the Air (OTA) technology
03/08/2023  12.6 OTA Fully Debugged
05/08/2023  12.7 WhatsApp used to notify start/end of Month End Process
05/08/2023  12.8 WhatsApp Message sent if "wait for user" error flagged
05/08/2023  12.9 Firmware Update Login page now shows Power Monitor title
16/08/2023  12.10   depreciated
18/08/2023  12.11   Whats app message sent when new day file created
01/10/2023  12.12 Bug in Month End Process corrected (position in File name of month and .csv)
28/10/2023  12:13 Omitted
28/10/2023  12.14 Command added to send a test whatsapp message, and IP Hostname customized.
01/11/2023  12.15 Kilowatt hour value on Information corrected
06/01/2024  12.16 Whatsapp message now sent on reboot, Time added to Standard_Format_Date_Time
05/03/2025  12.17 Month end file creation replaced with Zip file creation
30/12/2025  12.18 Watchdog timer implemented, all manual switch wait states replaced with automatic reboot
30/12/2025  12.19 SD card retry logic implemented - 3 retry attempts before reboot
30/12/2025  12.20 Code quality improvements: magic numbers replaced with constants, global variables organized into structs
30/12/2025  12.21 String concatenation heap fragmentation fixed - reserve() calls added to all webpage and WhatsApp functions
30/12/2025  12.22 Error handling improved - Get_Weather(), Get_Sensor(), Parse_Weather_info(), and Receive() now return bool status
30/12/2025  12.23 Month-end auto-delete implemented - daily files automatically deleted after successful consolidation, plus web button for manual bulk delete
30/12/2025  12.24 Weather API rate limiting - reduced from every 5 seconds (720 calls/hour) to every 10 minutes (6 calls/hour)
30/12/2025  12.25 WhatsApp message deduplication - prevents identical messages within 5 minutes to avoid spam and rate limiting
30/12/2025  12.26 Gas volume persistence - saves to EEPROM every 0.1m³ change, survives reboots with validation
30/12/2025  12.27 Long function refactoring (Issue #15) - Prefill_Data_Array() reduced from 326 lines to ~90 lines by extracting CSV parsing into reusable helper functions
30/12/2025  12.28 Code duplication eliminated (Issue #16) - Created toggleLED() helper function, replaced 4 instances of digitalWrite(pin, !digitalRead(pin))
30/12/2025  12.29 Header guards added (Issue #17) - Added traditional #ifndef/#define/#endif guards to Configuration.h and KWS_Protocol.h for maximum portability
30/12/2025  12.30 Debug output standardized (Issue #18) - Implemented DEBUG_LEVEL system with DEBUG_ERROR, DEBUG_INFO, DEBUG_VERBOSE macros for consistent debug control
30/12/2025  12.31 Blocking delays eliminated (Issue #19) - Replaced blocking delay() calls with non-blocking patterns to maintain system responsiveness
30/12/2025  12.32 RS485 communication improved (Issue #20) - Added yield() to RS485 receive loop allowing WiFi stack and background tasks during sensor reads
30/12/2025  12.33 WiFi recovery enhanced (Issue #21) - Implemented exponential backoff, WiFi radio restart, disconnect reason logging for robust recovery
30/12/2025  12.34 File write verification added (Issue #22) - All SD card write operations now verified to detect silent failures and ensure data integrity
30/12/2025  12.35 Date/time validation added (Issue #23) - NTP time data validated for reasonable ranges before use, prevents invalid timestamps in data
30/12/2025  12.36 Circular log buffer implemented (Issues #24, #26, #27) - 500-entry circular buffer logs to SD, system health monitoring, version info logged at startup
30/12/2025  12.37 Configuration file system (Issue #25) - Runtime configuration via config.json on SD card, no recompile needed for WiFi, intervals, features
30/12/2025  1.0.0 Semantic versioning adopted (Issue #27) - MAJOR.MINOR.PATCH format for clear version communication, helper function for formatted version display
30/12/2025  1.0.1 Comprehensive function documentation (Issue #28) - Added professional Doxygen-style comments to all major functions: purpose, parameters, return values, side effects
30/12/2025  1.0.2 Critical bug fix - Monthly file consolidation header corrected to match actual CSV format (20 columns instead of 8), preventing data corruption
05/01/2026  1.0.3 RS485 pin separation - Split RS485 enable signals into separate DE (pin 22) and RE (pin 25) for improved signal integrity and proper transceiver control
05/01/2026  1.0.4 RS485 retry logic - Implemented 3-attempt retry mechanism with adaptive delays (75ms, 100ms, 125ms) and per-field error tracking to improve communication reliability
05/01/2026  1.0.5 UI improvements and bug fixes - Blue LED flashes on SD writes (100ms smartDelay), web menu padding reduced (25px to 15px), HTML typo fixed in ZipAllFiles link
06/01/2026  1.0.6 Critical bug fix - Webpage graph now displays data after reboot/reinstall (Clear_Arrays() was incorrectly called after Prefill_Data_Array(), resetting Data_Table_Pointer to 0)
06/01/2026  1.0.7 OTA upload improvements - Watchdog timer now DISABLED during firmware upload to prevent timeout/reboot (re-enabled on completion/abort/failure), added UPLOAD_FILE_ABORTED handler, progress logging every 100KB, WhatsApp notification on success
06/01/2026  1.0.8 Webpage data display fix - Added "Waiting for Data" message when Data_Table_Pointer is 0 (occurs after reboot when CSV file exists but has no data rows yet), enhanced Prefill_Data_Array() logging to show Data_Table_Pointer status
06/01/2026  1.0.9 Critical fixes - 1) Added .log files to download list so system.log is accessible via web interface, 2) Added NTP time validation (Year >= 2020) before creating CSV files to prevent files with placeholder birthday dates (19511118.csv issue)
06/01/2026  1.0.10 OTA UX improvement - Upload page now auto-polls device after reboot and redirects to main page when online (eliminates dead time/manual refresh), improved upload page styling with progress indicators and status messages
06/01/2026  1.0.11 RS485 error detection - Graph page now detects when sensor data is all zeros (RS485 communication failure) and displays helpful diagnostic message instead of blank page, lists RS485 pin connections and troubleshooting steps
06/01/2026  1.0.12 RS485 zero-detection refined - Modified to check last 10 entries instead of just most recent for more reliable RS485 communication failure detection
06/01/2026  1.0.13 RS485 zero-detection optimized - Enhanced to check all data entries with early exit when non-zero value found, improved efficiency
06/01/2026  1.0.14 Test code cleanup - Removed test data injection code, cleaned up zero-detection logic
06/01/2026  1.0.15 Critical watchdog fix - Added watchdog timer feeding (timerWrite) in Get_Variable_Data() before and after long operations to prevent crashes during RS485 sensor communication (which can take several seconds with retries), system now runs stably without unexpected reboots
06/01/2026  1.0.16 RS485 diagnostics added - Debug console output showing actual sensor readings (Voltage, Amperage, Wattage) after each RS485 fetch to verify communication and CSV write operations
06/01/2026  1.0.17 RS485 traffic logging - Detailed hex bytes sent/received for diagnosing wiring issues, wrong sensor address, or protocol mismatches
06/01/2026  1.0.18 RS485 RE pin logic - Attempted fix for receiver enable pin (reverted in next version due to module-specific behavior)
06/01/2026  1.0.19 Critical RS485 receive fix - Changed Receive() function to initialize required_bytes to 9 instead of 0, preventing premature loop exit after 1 byte (was causing all sensor readings to be zero and blank webpage)
06/01/2026  1.1.0 Major milestone - RS485 communication fully functional after critical bug fixes (versions 1.0.12-1.0.19), removed debug output for production readiness, Blue LED toggles every 2.5s during CSV prefill to differentiate from normal 5s operation cycle

SEMANTIC VERSIONING GUIDE FOR FUTURE RELEASES:
- MAJOR: Increment for breaking changes (hardware incompatibility, config file format changes, API changes)
- MINOR: Increment for new features (new sensors, new web pages, new capabilities) that are backwards-compatible
- PATCH: Increment for bug fixes and small improvements that don't add features
*/
// ---- Zip Libraries - TEMPORARILY DISABLED (Issue #11: ZIP implementation incomplete) ----
// #include <mz_zip_rw.h>
// #include <mz_zip.h>
// #include <mz_strm_zstd.h>
// #include <mz_strm_zlib.h>
// #include <mz_strm_wzaes.h>
// #include <mz_strm_split.h>
// #include <mz_strm_pkcrypt.h>
// #include <mz_strm_os.h>
// #include <mz_strm_mem.h>
// #include <mz_strm_lzma.h>
// #include <mz_strm_libcomp.h>
// #include <mz_strm_bzip.h>
// #include <mz_strm_buf.h>
// #include <mz_strm.h>
// #include <mz_os.h>
// #include <mz_crypt.h>
// #include <mz.h>

// compiler directives ------
//#define PRiNT_PREFiLL_DATA_VALUES       //
//#define PRiNT_SHUFFLiNG_DATA_VALUES
//#define PRiNT_MONTH_DATA_VALUES

// Debug Level System (Issue #18 - Standardize Debug Output)
// DEBUG_LEVEL: 0=none, 1=errors, 2=info, 3=verbose
#define DEBUG_LEVEL 2

#if DEBUG_LEVEL >= 1
    #define DEBUG_ERROR(x) console_print(x)
#else
    #define DEBUG_ERROR(x)
#endif

#if DEBUG_LEVEL >= 2
    #define DEBUG_INFO(x) console_print(x)
#else
    #define DEBUG_INFO(x)
#endif

#if DEBUG_LEVEL >= 3
    #define DEBUG_VERBOSE(x) console_print(x)
#else
    #define DEBUG_VERBOSE(x)
#endif

// definitions --------------
#define console Serial
#define RS485_Port Serial2

// Semantic Versioning (MAJOR.MINOR.PATCH) - Issue #27
#define VERSION_MAJOR 1   // Incompatible API changes
#define VERSION_MINOR 3   // Backwards-compatible functionality additions
#define VERSION_PATCH 2   // Backwards-compatible bug fixes
// 14/04/2026  1.3.2 Daily/reboot WhatsApp messages now include web IP and mDNS hostname; unique device hostname advertised for network scanners
#define VERSION_STRING "V1.3.2"

// Timing Constants (Issue #4 - Eliminate Magic Numbers)
#define WDT_TiMEOUT 5                           // 5 second watchdog timeout
#define SD_RETRY_ATTEMPTS 3                     // Number of retry attempts for SD card operations
const unsigned long MAIN_CYCLE_INTERVAL_MS = 5000;      // Main loop cycle time (5 seconds)
const unsigned long RS485_TIMEOUT_MS = 500;             // RS485 receive timeout
const unsigned long RS485_BYTE_DELAY_MS = 2;            // Delay between RS485 bytes
const unsigned long SD_RETRY_DELAY_MS = 2000;           // Delay between SD init retries
const unsigned long SD_FILE_RETRY_DELAY_MS = 1000;      // Delay between file operation retries
const unsigned long REBOOT_DELAY_MS = 5000;             // Delay before automatic reboot
const unsigned long WIFI_RETRY_DELAY_MS = 3000;         // Delay between WiFi retries
const unsigned long WHATSAPP_DEBOUNCE_MS = 1000;        // Minimum time between WhatsApp messages
const unsigned long STARTUP_DELAY_MS = 4000;            // Initial startup delay
const unsigned long DEBOUNCE_INTERVAL_MS = 10;          // Button debounce interval
const unsigned long WEATHER_UPDATE_INTERVAL_MS = 600000; // Weather API call interval (10 minutes)
const unsigned long WHATSAPP_DUPLICATE_INTERVAL_MS = 300000; // Prevent duplicate messages within 5 minutes
const double GAS_VOLUME_SAVE_THRESHOLD = 0.1;        // Save to EEPROM every 0.1m³ change
const int EEPROM_SIZE = 512;                         // EEPROM size in bytes
const int EEPROM_GAS_VOLUME_ADDR = 0;                // Address for gas volume (8 bytes double)
const int EEPROM_GAS_MAGIC_ADDR = 8;                 // Address for magic number (4 bytes)
const uint32_t EEPROM_MAGIC_NUMBER = 0xDEADBEEF;     // Magic number to validate EEPROM data
const int MAX_WIFI_RETRY_ATTEMPTS = 20;                 // Maximum WiFi connection attempts

// Circular Log Buffer Constants (Issue #24, #26, #27)
const int LOG_BUFFER_SIZE = 500;                     // Maximum log entries in circular buffer (reduced for RAM)
const String LOG_FILENAME = "/system.log";           // System log file on SD card
const unsigned long HEALTH_CHECK_INTERVAL_MS = 60000; // Check system health every minute

// Configuration File (Issue #25)
const String CONFIG_FILENAME = "/config.json";       // Configuration file on SD card

// includes --
#include <vfs_api.h>
#include <FSimpl.h>
#include <sd_diskio.h>
#include <sd_defines.h>
#include <SD.h>
#include <FS.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include <ESP32Time.h>
#include <Bounce2.h>
#include <Uri.h>
#include <HTTP_Method.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <stdio.h>
#include <ArduinoSort.h>
#include <HttpsOTAUpdate.h>
#include <Update.h>
#include <ESPmDNS.h>
#include <UrlEncode.h>
#include <EEPROM.h>
// -----------

// ================================================================================
// VERSION MANAGEMENT (Issue #27 - Semantic Versioning)
// ================================================================================

/**
 * @brief Get formatted version string with build information
 * 
 * Returns the current software version in semantic versioning format (MAJOR.MINOR.PATCH)
 * along with the compilation date and time. This function is used for console output,
 * web pages, and system logging.
 * 
 * @return String Formatted version string (e.g., "Version V1.0.0 (Built: Dec 30 2025 14:30:15)")
 * 
 * @note Called during system startup and displayed on web interface footer
 * @see VERSION_STRING, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH
 */
String get_version_string() {
    String version = "Version " VERSION_STRING;
    version += " (Built: " + String(__DATE__) + " " + String(__TIME__) + ")";
    return version;
}

// ================================================================================
// SYSTEM CONFIGURATION STRUCTURES (Issue #5 - Reduce Global Variables)
// ================================================================================

// System Configuration - Readonly values set at compile time or startup
struct SystemConfig {
    const char* hostname_prefix = "Power Monitor";
    const char* ssid;
    const char* password;
    const char* ntp_server = "pool.ntp.org";
    const char* weather_city;
    const char* weather_region;
    const char* weather_api_key = "917ddeff21dff2cfc5e57717f809d6ad";
    bool weather_enabled;
    bool whatsapp_enabled;
    long gmt_offset_sec = 0;
    int daylight_offset_sec = 3600;
    double gas_volume_per_pulse = 0.10;
    double temperature_calibration = 16.5 / 22.0;  // Sensor correction factor
    double critical_sd_freespace_mb = 1.0;
};

// System State - Runtime variables that change during operation
struct SystemState {
    unsigned long last_cycle = 0;
    unsigned long last_weather_read = 0;
    unsigned long last_time_update = 0;
    unsigned long whatsapp_time = 0;
    unsigned long reset_time = 0;
    bool setup_complete = false;
    bool month_end_in_progress = false;
    int wifi_retry_counter = 0;
    int wifi_signal_strength = 0;
    unsigned long sd_freespace = 0;
    double sd_freespace_mb = 0.0;
};

// System Statistics - Min/Max values and timing
struct SystemLimits {
    double lowest_voltage = 0;
    String time_of_lowest_voltage = "00:00:00";
    String date_of_lowest_voltage = "0000/00/00";
    double highest_voltage = 0;
    String time_of_highest_voltage = "00:00:00";
    String date_of_highest_voltage = "0000/00/00";
    double highest_amperage = 0;
    String time_of_highest_amperage = "00:00:00";
    String date_of_highest_amperage = "0000/00/00";
};

// Latest Readings - Most recent sensor and weather data
struct LatestReadings {
    String time = "00:00:00";
    double kilowatthour = 0;
    double weather_temperature = 0;
    double weather_feels_like = 0;
    double weather_temp_max = 0;
    double weather_temp_min = 0;
    double atmospheric_pressure = 0;
    double relative_humidity = 0;
    double wind_speed = 0;
    double wind_direction = 0;
    String weather_description = "                                     ";
};

// Gas Tracking Data
struct GasData {
    double cumulative_volume = 0;
    double latest_volume = 0;
    String latest_date = "          ";
    String latest_time = "        ";
};

// Date/Time Tracking
struct DateTimeStrings {
    String current_date_with = "1951/11/18";
    String current_date_without = "19511118";
    String current_time_with = "00:00:00";
    String current_time_without = "000000";
    String this_date_with = "1951/11/18";
    String this_date_without = "19511116";
    String this_time_with = "00:00:00";
    String this_time_without = "000000";
    String standard_format = "          ";
    String last_boot_date = "2022/29/12";
    String last_boot_time = "12:12:12";
    int this_day = 0;
};

// Global instances of structured data
SystemConfig g_config;
SystemState g_state;
SystemLimits g_limits;
LatestReadings g_latest;
GasData g_gas;
DateTimeStrings g_datetime;

// ================================================================================
// REMAINING GLOBAL VARIABLES (To be further refactored if needed)
// ================================================================================

// WhatsApp Message Deduplication (Issue #13)
struct WhatsAppMessageCache {
    String last_message = "";
    unsigned long last_message_time = 0;
};
WhatsAppMessageCache whatsapp_cache;

// Circular Log Buffer (Issue #24, #26, #27)
struct LogEntry {
    unsigned long timestamp;
    String message;
};
LogEntry log_buffer[LOG_BUFFER_SIZE];
int log_buffer_index = 0;
int log_buffer_count = 0;
unsigned long last_health_check = 0;

// Runtime Configuration (Issue #25 - Configuration File)
struct RuntimeConfig {
    String wifi_ssid = "Woodleigh";
    String wifi_password = "2008198399";
    String ntp_server = "pool.ntp.org";
    String weather_city = "Basingstoke";
    String weather_region = "uk";
    bool weather_enabled = true;
    bool whatsapp_enabled = true;
    bool gas_meter_enabled = false;  // Gas meter connected and monitoring enabled
    int debug_level = 2;  // 0=none, 1=errors, 2=info, 3=verbose
    unsigned long main_cycle_ms = 5000;
    unsigned long weather_update_ms = 600000;
    unsigned long health_check_ms = 60000;
    String ota_username = "SteveG";  // OTA firmware update login
    String ota_password = "W00dle1gh";  // OTA firmware update password
};
RuntimeConfig config;

//constexpr int eeprom_size = 30;      // year = 4, month = 4, day = 4, hour = 4, minute = 4, second = 4e
const char* host = "esp32";
// WiFi credentials now loaded from config.json (Issue #25)
const char* SSID = "Woodleigh";  // Fallback if config not loaded
const char* Password = "2008198399";  // Fallback if config not loaded
const char* NtpServer = "pool.ntp.org";  // Fallback if config not loaded
const char Weather_City[] = { "Basingstoke\0" };  // Fallback if config not loaded
const char Weather_Region[] = { "uk\0" };  // Fallback if config not loaded
bool Weather_Enabled = true;
bool WhatsApp_Enabled = true;
//bool WhatsApp_Debug = true;
//String WhatsApp_url_1 = "https://api.callmebot.com/whatsapp.php?phone=+447785938200&text=";
//String WhatsApp_url_2 = "&apikey=8876034";
String hostname = "power-monitor";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;                         // offset for the date and time function
const String WiFi_Status_Message[7] = {
                    "WL_iDLE_STATUS",   // temporary status assigned when WiFi.begin() is called
                    "WL_NO_SSiD_AVAiL", // when no SSiD are available
                    "WL_SCAN_COMPLETED",// scan networks is completed
                    "WL_CONNECTED",     // when connected to a WiFi network
                    "WL_CONNECT_FAiLED",// when the connection fails for all the attempts
                    "WL_CONNECTiON_LOST", // when the connection is lost
                    "WL_DiSCONNECTED",  // when disconnected from a network
};
const char  incomplete_Weather_Api_Link[] = { "http://api.openweathermap.org/data/2.5/weather?q=#&APPiD=917ddeff21dff2cfc5e57717f809d6ad\0" };
constexpr long console_Baudrate = 115200;
constexpr long RS485_Baudrate = 9600;
// ESP32 Pin Definitions ----
constexpr int Blue_Switch_pin = 32;
constexpr int Red_Switch_pin = 33;
constexpr int Green_led_pin = 26;
constexpr int Yellow_Switch_pin = 27;
constexpr int Blue_led_pin = 4;
constexpr int RS485_DE_pin = 22;        // RS485 Driver Enable (TX) - Active HIGH
constexpr int RS485_RE_pin = 25;        // RS485 Receiver Enable (RX) - Active HIGH
constexpr int RS485_Tx_pin = 17;
constexpr int RS485_Rx_pin = 16;
constexpr int SS_pin = 5;
constexpr int SCK_pin = 18;
constexpr int MiSO_pin = 19;
constexpr int MOSi_pin = 23;
constexpr int ONBOARDLED = 2;
constexpr int Gas_Switch_pin = 21;
constexpr double Gas_Volume_Per_Sensor_Rise = (double).10;
// Date and Time Fields -----
struct Date_Time {
    int Second;                 // [0 - 3]
    int Minute;                 // [4 - 7]
    int Hour;                   // [8 - 11]
    int Day;                    // [12 - 15]
    int Month;                  // [16 - 19]
    int Year;                   // [20 - 23]
    double Gas_Volume;          // [24 - 27]
}__attribute__((packed));
constexpr int Date_Time_Record_Length = 27;
union Date_Time_Union {
    Date_Time field;
    unsigned char character[Date_Time_Record_Length + 1];
};
//Date_Time_Union Today_Date_Time_Data;
Date_Time_Union Last_Gas_Date_Time_Data;
Date_Time_Union New_Date_Time_Data;
Date_Time_Union Current_Date_Time_Data;
Date_Time_Union Month_End_Date_Time_Data;
String Month_Names[] = { "","January","February","March","April","May","June","July","August","September","October","November","December" };
String Current_Date_With = "1951/11/18";
String Current_Date_Without = "19511118";
String Current_Time_With = "00:00:00";
String Current_Time_Without = "000000";
String Latest_Gas_Date_With = "          ";
String Latest_Gas_Time_With = "        ";
double Latest_Gas_Volume = 0;
String This_Date_With = "1951/11/18";
String This_Date_Without = "19511116";
String This_Time_With = "00:00:00";
String This_Time_Without = "000000";
String Standard_Format_Date_Time = "          ";
unsigned long WhatsApp_Time = 0;                                // ensure we do not double send Whatsapp messages
unsigned long Reset_Time = 0;
constexpr int Days_n_Month[13][2] = {
    // Leap,Normal
            {00,00},
            {31,31},// January
            {29,28},// February
            {31,31},// March                         
            {30,30},// April
            {31,31},// May
            {30,30},// June
            {31,31},// July
            {31,31},// August
            {30,30},// September
            {31,31},// October
            {30,30},// November
            {31,31},// December
};
// instantiations -----------
struct tm timeinfo;
Bounce Red_Switch = Bounce();
Bounce Blue_Switch = Bounce();
Bounce Yellow_Switch = Bounce();
Bounce Gas_Switch = Bounce();
hw_timer_t* Timer0_Cfg = NULL;
WebServer server(80);               // WebServer(HTTP port, 80 is defAult)
WiFiClient client;
HTTPClient http;
// RS485 Error Tracking
int RS485_field_failures[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // Track failures per field
int RS485_total_attempts = 0;                             // Total request attempts
int RS485_total_failures = 0;                             // Total failed requests
// DataFile constants and variables --------
File DataFile;                     // Full data file, holds all readings from KWS-AC301L
String DataFileName = "20220101";       // .cvs
constexpr int Number_of_Column_Field_Names = 20;            // indicate the available addresses [0 - 19]
String Column_Field_Names[Number_of_Column_Field_Names] = {
                        "Date",                 // [0]
                        "Time",                 // [1]
                        "Voltage",              // [2]
                        "Amperage",             // [3]
                        "Wattage",              // [4]
                        "Up Time",              // [5]
                        "kiloWattHours",        // [6]
                        "Power Factor",         // [7]
                        "Frequency",            // [8]
                        "Sensor Temperature",   // [9]
                        "Weather Temperature",  // [10]
                        "Temperature_Feels_Like", // [11]
                        "Temperature Maximum",  // [12]
                        "Temperature Minimum",  // [13]
                        "Atmospheric Pressure", // [14]
                        "Relative Humidity",    // [15]
                        "Wind Speed",           // [16]
                        "Wind Direction",       // [17]
                        "Gas Volume",           // [18]
                        "Weather Description",  // [19]
};
struct Data_Record_Values {
    char ldate[11];             //  [0 - 10]  [00]  date record was taken
    char ltime[9];              //  [11 - 19] [01]  time record was taken
    double Voltage;             //  [20 - 23] [02]
    double Amperage;            //  [24 - 27] [03]
    double wattage;             //  [28 - 31] [04]
    double uptime;              //  [32 - 35] [05]
    double kilowatthour;        //  [36 - 39] [06]
    double powerfactor;         //  [40 - 43] [07]
    double frequency;           //  [44 - 47] [08]
    double sensor_temperature;  //  [48 - 51] [09]
    double weather_temperature; //  [52 - 55] [10]  Weather temperature
    double temperature_feels_like;//  [56 - 59] [11]  temperature feels like
    double temperature_maximum; //  [60 - 63] [12]  temperature maximum
    double temperature_minimum; //  [64 - 67] [13]  temperature minimum
    double atmospheric_pressure;//  [68 - 71] [14]  atmospheric pressure
    double relative_humidity;   //  [72 - 75] [15]  relative humidity
    double wind_speed;          //  [76 - 79] [16]  wind speed
    double wind_direction;      //  [80 - 83] [17]  wind direction
    double gas_volume;          //  [84 - 87] [18]  gas volume 
    char weather_description[20]; //  [88 - 110] [19] weather description
}__attribute__((packed));
constexpr char Data_Record_Length = 111;
union Data_Record_Union {
    Data_Record_Values field;
    unsigned int character[Data_Record_Length];
};
Data_Record_Union Current_Data_Record;
Data_Record_Union Concatenation_Data_Record;
// Data Table ---------------------------------------------------------------------------------------------------------
int Data_Table_Pointer = 0;          // points to the next index of Data_Table
//int Month_Table_Pointer = 0;
int Data_Record_Count = 0;           // running total of records written to Data Table
constexpr char Data_Table_Size = 60;
constexpr int Month_Table_Size = (24 * 31);
struct Data_Table_Record {
    char ldate[11];                 //  [0 - 10]  date record was taken
    char ltime[9];                  //  [11 - 19]  time record was taken
    double Voltage;                 //  [20 - 23]
    double Amperage;                //  [24 - 27]
    double wattage;                 //  [28 - 31]
    double uptime;                  //  [32 - 35]
    double kilowatthour;            //  [36 - 39]
    double powerfactor;             //  [40 - 43]
    double frequency;               //  [44 - 47]
    double sensor_temperature;      //  [48 - 51]
    double weather_temperature;     //  [52 - 54]
    double temperature_feels_like;  //  [55 - 58]
    double temperature_maximum;     //  [59 - 63]
    double temperature_minimum;     //  [64 - 67]
    double atmospheric_pressure;    //  [68 - 71]
    double relative_humidity;       //  [72 - 75]
    double wind_speed;              //  [76 - 79]
    double wind_direction;          //  [80 - 83]
    char weather_description[20];   //  [84 - 103]
    double gas_volume;              //  [104 - 107]
}__attribute__((packed));
constexpr int Data_Table_Record_Length = 108;
union Data_Table_Union {
    Data_Table_Record field;
    unsigned int characters[Data_Table_Record_Length];
};
Data_Table_Union Readings_Table[Data_Table_Size];
int Wattage_Readings_Count = 0;
/// ConcatenationFile Constants and Variables -------------
String ZipFileName = "M202501";                     // file name of the created zip file (all the previous months dailt files, ready to be downloaded
bool Month_End_Process_Status = false;
String Month_End_Process_Started = "1951/11/18 20:00";
constexpr int Chunk_Size = 512;
// KWS Request Field Numbers 
constexpr int Request_Voltage = 0;
constexpr int Request_Amperage = 1;
constexpr int Request_Wattage = 2;
constexpr int Request_UpTime = 3;
constexpr int Request_Kilowatthour = 4;
constexpr int Request_Power_Factor = 5;
constexpr int Request_Frequency = 6;
constexpr int Request_Sensor_Temperature = 7;
constexpr int Number_of_RS485_Requests = 7;
constexpr byte RS485_Requests[Number_of_RS485_Requests + 1][8] = {
                        {0x02,0x03,0x00,0x0E,0x00,0x01,0xE5,0xFA},	// [0]  Request Voltage, in tenths of a volt
                        {0x02,0x03,0x00,0x0F,0x00,0x02,0xF4,0x3B},	// [1]  Request Amperage, in milli-amps
                        {0x02,0x03,0x00,0x11,0x00,0x02,0x94,0x3D},	// [2]  Request Watts, in tenths of a watt
                        {0x02,0x03,0x00,0x19,0x00,0x01,0x55,0xFE},	// [3]  Request uptime, in minutes
                        {0x02,0x03,0x00,0x17,0x00,0x02,0x74,0x3C},// [4]  Request kilo_watt_hour, in milli-watts
                        {0x02,0x03,0x00,0x1D,0x00,0x01,0x14,0x3F},// [5]  Request power factor, in one hundredths
                        {0x02,0x03,0x00,0x1E,0x00,0x01,0xE4,0x3F},// [6]  Request Hertz, in tenths of a hertz
                        {0x02,0x03,0x00,0x1A,0x00,0x01,0xA5,0xFE},// [7]  Request temperature, in degrees centigrade
};
String FileNames[200];
bool Yellow_Switch_Pressed = false;
bool Green_Switch_Pressed = false;
bool Blue_Switch_Pressed = false;
String site_width = "1060";                 // width of web page
String site_height = "600";                 // height of web page
String webpage;
String lastcall;
double temperature_calibration = (double)16.5 / (double)22.0; // temperature reading = 22, actual temperature = 16.5
String Last_Boot_Time_With = "12:12:12";
String Last_Boot_Date_With = "2022/29/12";
int This_Day = 0;
double Cumulative_Gas_Volume = 0;
double Last_Saved_Gas_Volume = 0;  // Track last saved value to detect threshold changes
// Lowest Voltage -----------
double Lowest_Voltage = 0;
String Time_of_Lowest_Voltage = "00:00:00";
String Date_of_Lowest_Voltage = "0000/00/00";
double Highest_Voltage = 0;
String Time_of_Highest_Voltage = "00:00:00";
String Date_of_Highest_Voltage = "0000/00/00";
double Highest_Amperage = 0;
String Time_of_Highest_Amperage = "00:00:00";
String Date_of_Highest_Amperage = "0000/00/00";
String Time_of_Latest_reading = "00:00:00";
double Latest_weather_temperature = 0;
double Latest_weather_temperature_feels_like = 0;
double Latest_weather_temperature_maximum = 0;
double Latest_weather_temperature_minimum = 0;
double Latest_atmospheric_pressure = 0;
double Latest_relative_humidity = 0;
double Latest_wind_speed = 0;
double Latest_wind_direction = 0;
String Latest_weather_description = "                                     ";
double Latest_Kilowatthour = 0;
constexpr int Days_in_Month[13][2] = {
    // Leap,Normal
            {00,00},// 
            {31,31},// Jan 31/31
            {29,28},// Feb 29/28
            {31,30},// Mar 31/31
            {30,30},// Apr 30/30
            {31,31},// May 31/31
            {30,30},// Jun 30/30
            {31,31},// Jul 31/31
            {31,31},// Aug 31/31
            {30,30},// Sep 30/30
            {31,31},// Oct 31/31
            {30,30},// Nov 30/30
            {31,31} // Dec 31/31
};
typedef struct {
    double temp;
    double temp_min;
    double temp_max;
    double temp_feel;
    int pressure;
    int humidity;
    double wind_speed;
    int wind_direction;
    String weather;
} weather_record_type;
weather_record_type Weather_Record;
unsigned long Last_Cycle = 0;
unsigned long Last_Weather_Read = 0;
unsigned long SD_freespace = 0;
constexpr double Critical_SD_Freespace = 1;                     // bottom limit of SD space in MB
double SD_freespace_double = 0;
String Temp_Message;
bool Setup_Complete = false;
char Complete_Weather_Api_Link[120];
String Message;
char Parse_Output[25];
String Csv_File_Names[31];                          // possibly 31 csv files
int WiFi_Signal_Strength = 0;
int WiFi_Retry_Counter = 0;
unsigned long Last_Time_Update = 0;
// Debug / Test Variables ---
bool Once = false;
// --- Firware Update Pages
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
String get_loginIndex() {
    String page = "<form name='loginForm'>";
    page += "<table width='20%' bgcolor='A09F9F' align='center'>";
    page += "<tr><td colspan=2>";
    page += "<center><font size=4><b>Power Monitor Firmware Update Login</b></font></center>";
    page += "<br></td><br><br></tr>";
    page += "<tr><td>Username:</td>";
    page += "<td><input type='text' size=25 name='userid'><br></td></tr>";
    page += "<br><br>";
    page += "<tr><td>Password:</td>";
    page += "<td><input type='Password' size=25 name='pwd'><br></td>";
    page += "<br><br></tr>";
    page += "<tr><td><input type='submit' onclick='check(this.form)' value='Login'></td></tr>";
    page += "</table></form>";
    page += "<script>";
    page += "function check(form) {";
    page += "if(form.userid.value=='" + config.ota_username + "' && form.pwd.value=='" + config.ota_password + "') {";
    page += "window.open('/serverIndex')";
    page += "} else {";
    page += "alert('Error Password or Username')";
    page += "}";
    page += "}";
    page += "</script>";
    return page;
}

const char* serverIndex = {
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<style>"
        "body{font-family:Arial;text-align:center;padding:50px;}"
        "#prg{font-size:24px;margin:20px;color:#0066cc;}"
        "#status{font-size:18px;margin:20px;color:#ff6600;}"
        "input[type='file']{padding:10px;margin:10px;}"
        "input[type='submit']{padding:10px 30px;background:#0066cc;color:white;border:none;border-radius:5px;cursor:pointer;font-size:16px;}"
        "input[type='submit']:hover{background:#0052a3;}"
    "</style>"
    "<h1>Power Monitor Firmware Update</h1>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
        "<input type='file' name='update' accept='.bin'>"
        "<input type='submit' value='Upload Firmware'>"
    "</form>"
    "<div id='prg'>Select firmware file and click Upload</div>"
    "<div id='status'></div>"
    "<script>"
        "var pollInterval;"
        "var pollAttempts = 0;"
        "function pollDevice(){"
            "$.ajax({"
                "url: '/',"
                "type: 'GET',"
                "timeout: 2000,"
                "success: function(){"
                    "clearInterval(pollInterval);"
                    "$('#status').html('✓ Device online - Redirecting...');"
                    "setTimeout(function(){ window.location.href='/'; }, 1000);"
                "},"
                "error: function(){"
                    "pollAttempts++;"
                    "var elapsed = Math.floor(pollAttempts * 3);"
                    "$('#status').html('⏳ Waiting for device to reboot... (' + elapsed + 's)');"
                    "if(pollAttempts > 40){"  // 2 minutes max
                        "clearInterval(pollInterval);"
                        "$('#status').html('⚠ Timeout - Please refresh manually or check device');"
                    "}"
                "}"
            "});"
        "}"
        "$('form').submit(function(e){"
            "e.preventDefault();"
            "var form = $('#upload_form')[0];"
            "var data = new FormData(form);"
            "$('#prg').html('Uploading firmware...');"
            "$.ajax({"
                "url: '/update',"
                "type: 'POST',"
                "data: data,"
                "contentType: false,"
                "processData:false,"
                "xhr: function(){"
                    "var xhr = new window.XMLHttpRequest();"
                    "xhr.upload.addEventListener('progress', function(evt){"
                        "if(evt.lengthComputable){"
                            "var per = evt.loaded / evt.total;"
                            "$('#prg').html('Upload progress: ' + Math.round(per*100) + '%');"
                        "}"
                    "}, false);"
                    "return xhr;"
                "},"
                "success:function(d, s){"
                    "$('#prg').html('✓ Upload complete!');"
                    "$('#status').html('Device is rebooting...');"
                    "setTimeout(function(){"
                        "pollAttempts = 0;"
                        "pollInterval = setInterval(pollDevice, 3000);"  // Poll every 3 seconds
                    "}, 5000);"  // Wait 5 seconds before starting to poll
                "},"
                "error: function(a, b, c){"
                    "$('#prg').html('✗ Upload failed: ' + c);"
                    "$('#status').html('Please try again or check console for errors.');"
                "}"
            "});"
        "});"
    "</script>" };
// Watchdog Timer ISR -----
void IRAM_ATTR resetModule() {
    ets_printf("Watchdog Timer Triggered - Rebooting\n");
    esp_restart();
}
// setup -----
//------------

/**
 * @brief Save gas volume to EEPROM for persistence across reboots
 * 
 * Writes the current gas volume reading to EEPROM along with a magic number
 * for validation. This ensures gas meter readings survive power cycles and
 * system reboots. The EEPROM is only written when volume changes by at least
 * 0.1 m³ to minimize wear on EEPROM memory.
 * 
 * @param volume The gas volume in cubic meters to save
 * 
 * @note Side effect: Updates Last_Saved_Gas_Volume global variable
 * @note EEPROM has ~100,000 write cycle limit; writes are throttled to 0.1 m³ changes
 * @see Load_Gas_Volume_From_EEPROM(), GAS_VOLUME_SAVE_THRESHOLD
 */
void Save_Gas_Volume_To_EEPROM(double volume) {
    EEPROM.put(EEPROM_GAS_VOLUME_ADDR, volume);
    EEPROM.put(EEPROM_GAS_MAGIC_ADDR, EEPROM_MAGIC_NUMBER);
    EEPROM.commit();
    Last_Saved_Gas_Volume = volume;
    console_print("Gas volume saved to EEPROM: " + String(volume, 2) + " m³");
}

/**
 * @brief Load gas volume from EEPROM with validation
 * 
 * Reads the stored gas volume from EEPROM and validates it using a magic number.
 * If the magic number is incorrect (first boot or corrupted data), initializes
 * to 0.0. Performs sanity checks to ensure the value is valid (positive, finite,
 * and less than 100,000 m³).
 * 
 * @return double The gas volume in cubic meters, or 0.0 if EEPROM data is invalid
 * 
 * @note Called once during setup() to restore state after reboot
 * @note Validation checks: magic number match, NaN check, range 0-100000 m³
 * @see Save_Gas_Volume_To_EEPROM(), EEPROM_MAGIC_NUMBER
 */
double Load_Gas_Volume_From_EEPROM() {
    uint32_t magic;
    EEPROM.get(EEPROM_GAS_MAGIC_ADDR, magic);
    
    if (magic != EEPROM_MAGIC_NUMBER) {
        console_print("EEPROM gas volume not initialized (magic mismatch), starting from 0");
        Save_Gas_Volume_To_EEPROM(0.0);
        return 0.0;
    }
    
    double volume;
    EEPROM.get(EEPROM_GAS_VOLUME_ADDR, volume);
    
    // Sanity check: volume should be positive and reasonable
    if (isnan(volume) || volume < 0 || volume > 100000) {
        console_print("EEPROM gas volume invalid: " + String(volume) + ", resetting to 0");
        Save_Gas_Volume_To_EEPROM(0.0);
        return 0.0;
    }
    
    console_print("Gas volume loaded from EEPROM: " + String(volume, 2) + " m³");
    return volume;
}

/**
 * @brief Toggle LED state (helper to eliminate code duplication)
 * 
 * Reads the current state of the specified pin and inverts it (HIGH→LOW or LOW→HIGH).
 * This replaces multiple instances of digitalWrite(pin, !digitalRead(pin)) throughout
 * the codebase.
 * 
 * @param pin The GPIO pin number of the LED to toggle
 * 
 * @note Used for Blue, Green, and Red LED indicators during various operations
 * @see Blue_led_pin, Green_led_pin, Red_led_pin
 */
void toggleLED(int pin) {
    digitalWrite(pin, !digitalRead(pin));
}

/**
 * @brief Non-blocking delay helper that checks if interval has elapsed
 * 
 * Compares current time against a reference time and determines if the specified
 * interval has passed. When the interval elapses, automatically updates the reference
 * time for the next cycle. This replaces blocking delay() calls and keeps the system
 * responsive to web requests, watchdog, and other tasks.
 * 
 * @param lastTime Reference to the timestamp of last execution (will be updated)
 * @param interval Minimum time in milliseconds between executions
 * 
 * @return true if interval has elapsed (time to execute), false otherwise
 * 
 * @note lastTime parameter is passed by reference and updated when interval elapses
 * @note Handles millis() overflow correctly (wraps at ~49 days)
 * @see smartDelay()
 */
bool nonBlockingDelay(unsigned long &lastTime, unsigned long interval) {
    unsigned long currentTime = millis();
    if (currentTime - lastTime >= interval) {
        lastTime = currentTime;
        return true;
    }
    return false;
}

/**
 * @brief Smart delay that maintains system responsiveness
 * 
 * Delays for the specified duration while continuously feeding the watchdog timer,
 * handling web server requests, and allowing ESP32 background tasks to run. This
 * prevents system hangs and watchdog resets during intentional delays (e.g., waiting
 * for WiFi connection, SD card retries).
 * 
 * @param ms Duration to delay in milliseconds
 * 
 * @note Side effects: Feeds watchdog, processes web requests, yields to RTOS
 * @note Use this instead of delay() anywhere system responsiveness is needed
 * @note Busy-wait loop, does not enter power-saving mode
 * @see nonBlockingDelay() for event-based timing
 */
void smartDelay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        timerWrite(Timer0_Cfg, 0);  // Reset watchdog timer
        server.handleClient();       // Handle web requests
        yield();                     // Allow ESP32 background tasks
    }
}

/**
 * @brief Verify SD card write operation succeeded
 * 
 * Checks if a file write operation wrote the expected number of bytes. Detects
 * silent write failures that can occur when SD card is full, corrupted, or removed.
 * Logs error messages for debugging when verification fails.
 * 
 * @param file Reference to the File object (for potential future checks)
 * @param filename Name of the file being written (for error messages)
 * @param bytesWritten Number of bytes reported written by print/write operation
 * @param operation Description of the operation (e.g., "header write", "data append")
 * 
 * @return true if write succeeded (bytesWritten > 0), false if write failed
 * 
 * @note Does not trigger reboot; caller is responsible for error handling
 * @note All SD write operations should use this for verification
 * @see Write_New_Data_Record_to_Data_Files()
 */
bool verifyFileWrite(File &file, const String &filename, size_t bytesWritten, const String &operation) {
    if (bytesWritten == 0) {
        DEBUG_ERROR("Write verification failed for " + filename + " during " + operation);
        return false;
    }
    return true;
}

/**
 * @brief Arduino setup function - System initialization
 * 
 * Initializes all hardware peripherals, communication interfaces, and system services:
 * - Serial console and RS485 communication
 * - LED indicators and buttons with debouncing
 * - SD card with retry logic
 * - EEPROM and gas volume restoration
 * - WiFi connection and web server
 * - NTP time synchronization
 * - Over-the-air (OTA) update capability
 * - Watchdog timer
 * - Configuration file loading
 * - Data file creation/restoration
 * 
 * Runs once at power-on or after reset. If critical initialization fails (SD card,
 * WiFi), the system will automatically reboot after sending WhatsApp notification.
 * 
 * @note Blocking function - must complete before loop() begins
 * @note All console output uses console_print() for log buffer capture
 * @note LEDs provide visual feedback during initialization steps
 * @see loop(), load_config_from_sd()
 */
void setup() {
    console.begin(console_Baudrate);                                        // enable the console
    while (!console);                                                       // wait for port to settle
    delay(STARTUP_DELAY_MS);
    
    console_print("\n\nPower Monitor System Starting...");
    console_print(get_version_string());
    
    // Initialize EEPROM for gas volume persistence
    EEPROM.begin(EEPROM_SIZE);
    Current_Data_Record.field.gas_volume = Load_Gas_Volume_From_EEPROM();
    Latest_Gas_Volume = Current_Data_Record.field.gas_volume;
    
    // Initialize system configuration structures
    g_config.ssid = SSID;
    g_config.password = Password;
    g_config.weather_city = Weather_City;
    g_config.weather_region = Weather_Region;
    g_config.weather_enabled = Weather_Enabled;
    g_config.whatsapp_enabled = WhatsApp_Enabled;
    
    console.print(millis(), DEC); console.println("\tCommencing Setup");
    console.print(millis(), DEC); console.println("\tIO Configuration Commenced");
    pinMode(Blue_led_pin, OUTPUT);
    pinMode(Red_Switch_pin, INPUT_PULLUP);
    pinMode(Blue_Switch_pin, INPUT_PULLUP);
    pinMode(Yellow_Switch_pin, INPUT_PULLUP);
    pinMode(Green_led_pin, OUTPUT);
    pinMode(Gas_Switch_pin, INPUT_PULLUP);
    console_print("Switch attachment commenced");
    Gas_Switch.attach(Gas_Switch_pin);
    Red_Switch.attach(Red_Switch_pin); // setup defaults for debouncing switches
    Blue_Switch.attach(Blue_Switch_pin);
    Yellow_Switch.attach(Yellow_Switch_pin);
    Gas_Switch.interval(100);
    Red_Switch.interval(DEBOUNCE_INTERVAL_MS);              // sets debounce time
    Blue_Switch.interval(DEBOUNCE_INTERVAL_MS);
    Gas_Switch.update();
    Red_Switch.update();
    Blue_Switch.update();
    console_print("Switch attachment complete");
    //   digitalWrite(0, HIGH);
    digitalWrite(Green_led_pin, LOW);
    digitalWrite(Blue_led_pin, LOW);
    console_print("IO Configuration Complete");
    // WiFi and Web Setup -------------
    console_print("WiFi Configuration Commenced");
    WiFi.mode(WIFI_STA);
    hostname = Build_Device_Hostname();
    console_print("Device Hostname: " + hostname);
    WiFi.setHostname(hostname.c_str());         //define hostname
    StartWiFi(SSID, Password);                  // Start WiFi
    initTime("GMT0BST,M3.5.0/1,M10.5.0");       // initialise Time library to London, GMT0BST,M3.5.0/1,M10.5.0
    console_print("Starting Server");
    server.begin();                             // Start Webserver
    console_print("Server Started");
    StartMDNSService();
    server.on("/", i_Display);
    /**
     * @brief OTA firmware update login endpoint
     * 
     * Serves the login page for OTA firmware updates. Credentials are validated
     * client-side using JavaScript against values from config.json. Provides
     * authentication before allowing access to firmware upload interface.
     * 
     * @note Route: GET /Firmware
     * @note Credentials: config.ota_username and config.ota_password
     * @see get_loginIndex(), /serverIndex
     */
    server.on("/Firmware", HTTP_GET, []() {
        console_print("Firmware Upload Requested via Webpage");
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", get_loginIndex());
        });
    /**
     * @brief OTA firmware upload page endpoint
     * 
     * Serves the file selection and upload interface for OTA firmware updates.
     * Accessed only after successful authentication at /Firmware. Uses jQuery
     * for AJAX upload with progress indicator. POSTs firmware binary to /update.
     * 
     * @note Route: GET /serverIndex
     * @note Upload target: POST /update
     * @see serverIndex, /Firmware, /update
     */
    server.on("/serverIndex", HTTP_GET, []() {
        console_print("Login Succeeded _ Firmware File Selection Requested via Webpage");
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
        });
    server.on("/Display", i_Display);               // display the main web page
    server.on("/Information", i_Information);       // display information
    server.on("/DownloadFiles", i_Download_Files);  // select a file to download
    server.on("/GetFile", i_Download_File);         // download the selected file
    server.on("/DeleteFiles", i_Delete_Files);      // select a file to delete
    server.on("/DelFile", i_Del_File);              // delete the selected file
    server.on("/DeletePreviousMonth", i_Delete_Previous_Month);  // bulk delete previous month's files
    server.on("/ConfirmDeletePreviousMonth", i_Confirm_Delete_Previous_Month);  // confirm bulk delete
    server.on("/Reset", i_Web_Reset);               // reset the processor from the webpage
    server.on("/WhatsApp", i_WhatsApp);             // send a test Whatsapp message from the webpage
    server.on("/ConfigUpload", i_Config_Upload);    // configuration file upload page
    server.on("/UploadConfig", HTTP_POST, i_Upload_Config_Complete, i_Handle_Config_Upload);  // handle config file upload
    server.on("/DownloadConfig", i_Download_Config); // download current config.json
    /**
     * @brief OTA firmware update handler endpoint
     * 
     * Processes uploaded firmware binary and flashes ESP32 over-the-air. Handles
     * multi-part upload in chunks using Arduino Update library. Validates firmware,
     * writes to flash, and automatically reboots on success. Reports FAIL/OK status.
     * 
     * Upload lifecycle:
     * 1. UPLOAD_FILE_START: Initialize Update with unknown size
     * 2. UPLOAD_FILE_WRITE: Write chunks to flash progressively, reset watchdog
     * 3. UPLOAD_FILE_END: Finalize update, verify integrity, reboot
     * 4. UPLOAD_FILE_ABORTED: Clean up failed upload
     * 
     * @note Route: POST /update
     * @note Content-Type: multipart/form-data
     * @note Auto-reboots on success
     * @note Watchdog timer reset every chunk to prevent timeout
     * @note Prints errors to Serial on failure
     * @see Update.h, /Firmware, /serverIndex
     */
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
        }, []() {
            static size_t totalReceived = 0;
            HTTPUpload& upload = server.upload();
            if (upload.status == UPLOAD_FILE_START) {
                totalReceived = 0;
                timerAlarmDisable(Timer0_Cfg);  // Disable watchdog during OTA upload
                console_print("OTA Update Started: " + upload.filename);
                console_print("Watchdog timer disabled for OTA upload");
                console_print("File size: " + String(upload.totalSize) + " bytes");
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                    console_print("OTA Update.begin() failed");
                    timerAlarmEnable(Timer0_Cfg);  // Re-enable watchdog on failure
                }
            }
            else if (upload.status == UPLOAD_FILE_WRITE) {
                totalReceived += upload.currentSize;
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                    Update.printError(Serial);
                    console_print("OTA Write failed at " + String(totalReceived) + " bytes");
                } else {
                    // Log progress every 100KB
                    if (totalReceived % 102400 < upload.currentSize) {
                        console_print("OTA Progress: " + String(totalReceived / 1024) + " KB received");
                    }
                }
            }
            else if (upload.status == UPLOAD_FILE_END) {
                console_print("OTA Upload complete: " + String(totalReceived) + " bytes");
                if (Update.end(true)) {
                    console_print("OTA Update Success: Rebooting in 2 seconds");
                    Send_WhatsApp_Message("Power Monitor - Firmware Updated Successfully - Rebooting");
                    // No need to re-enable watchdog - device will reboot
                    i_Display();
                }
                else {
                    Update.printError(Serial);
                    console_print("OTA Update.end() failed");
                    timerAlarmEnable(Timer0_Cfg);  // Re-enable watchdog on failure
                }
            }
            else if (upload.status == UPLOAD_FILE_ABORTED) {
                Update.abort();
                console_print("OTA Upload Aborted at " + String(totalReceived) + " bytes");
                console_print("Device still running previous firmware - no changes made");
                timerAlarmEnable(Timer0_Cfg);  // Re-enable watchdog after abort
            }
            });
        console_print("Starting Server");
        server.begin();                             // Start Webserver
        console_print("Server Started");
        RS485_Port.begin(RS485_Baudrate, SERIAL_8N1, RS485_Rx_pin, RS485_Tx_pin);
        pinMode(RS485_DE_pin, OUTPUT);      // DE - Driver Enable (TX)
        pinMode(RS485_RE_pin, OUTPUT);      // RE - Receiver Enable (RX)
        digitalWrite(RS485_DE_pin, LOW);    // Disable TX initially
        digitalWrite(RS485_RE_pin, HIGH);   // Enable RX initially (Active HIGH, so HIGH=enabled)
        delay(10);
        console_print("SD Configuration Commenced");
        bool sd_initialized = false;
        for (int retry = 1; retry <= SD_RETRY_ATTEMPTS; retry++) {
            if (SD.begin(SS_pin)) {
                sd_initialized = true;
                break;
            }
            console_print("SD Drive Begin Failed - Retry " + String(retry) + " of " + String(SD_RETRY_ATTEMPTS));
            if (retry < SD_RETRY_ATTEMPTS) {
                SD.end();
                delay(2000);  // Wait 2 seconds before retry
            }
        }
        if (!sd_initialized) {
            console_print("SD Drive Begin Failed after " + String(SD_RETRY_ATTEMPTS) + " attempts - Rebooting in 5 seconds");
            Send_WhatsApp_Message("Power Monitor - SD Drive Begin Failed after " + String(SD_RETRY_ATTEMPTS) + " attempts - Automatic Reboot");
            delay(5000);
            ESP.restart();
        }
        else {
            console_print("SD Drive Begin Succeeded");
            char cardType = SD.cardType();
            bool card_detected = false;
            for (int retry = 1; retry <= SD_RETRY_ATTEMPTS; retry++) {
                if (SD.cardType() != CARD_NONE) {
                    card_detected = true;
                    cardType = SD.cardType();
                    break;
                }
                console_print("No SD Card Found - Retry " + String(retry) + " of " + String(SD_RETRY_ATTEMPTS));
                if (retry < SD_RETRY_ATTEMPTS) {
                    delay(2000);  // Wait 2 seconds before retry
                }
            }
            if (!card_detected) {
                console_print("No SD Card Found after " + String(SD_RETRY_ATTEMPTS) + " attempts - Rebooting in 5 seconds");
                Send_WhatsApp_Message("Power Monitor - No SD Card Found after " + String(SD_RETRY_ATTEMPTS) + " attempts - Automatic Reboot");
                delay(5000);
                ESP.restart();
            }
            String card;
            if (cardType == CARD_MMC) {
                card = "MMC";
            }
            else if (cardType == CARD_SD) {
                card = "SDSC";
            }
            else if (cardType == CARD_SDHC) {
                card = "SDH//C";
            }
            else {
                card = "UNKNOWN";
            }
            DEBUG_INFO("SD Card Type: " + card);
            unsigned long cardSize = SD.cardSize() / (1024 * 1024);
            DEBUG_INFO("SD Card Size : " + String(cardSize) + "MBytes");
            console_print("SD Total Bytes : " + String(SD.totalBytes()));
            console_print("SD Used bytes : " + String(SD.usedBytes()));
            DEBUG_INFO("SD Card initialisation Complete");
        }
        DEBUG_INFO("SD Configuration Complete");
        
        // Load configuration from SD card (Issue #25)
        load_config_from_sd();
        
        // Apply configuration to runtime variables
        SSID = config.wifi_ssid.c_str();
        Password = config.wifi_password.c_str();
        NtpServer = config.ntp_server.c_str();
        Weather_Enabled = config.weather_enabled;
        WhatsApp_Enabled = config.whatsapp_enabled;
        
        console_print("Date and Time Update");
        Update_Current_Timeinfo();                                              // update This date time info, no /s
        
        // Ensure we have valid time before creating date-based files
        if (New_Date_Time_Data.field.Year < 2020) {
            console_print("WARNING: NTP time not synchronized yet (Year: " + String(New_Date_Time_Data.field.Year) + "), waiting for valid time...");
            int time_sync_attempts = 0;
            while (New_Date_Time_Data.field.Year < 2020 && time_sync_attempts < 30) {
                smartDelay(1000);  // Wait 1 second
                Update_Current_Timeinfo();
                time_sync_attempts++;
                console_print("Time sync attempt " + String(time_sync_attempts) + ": Year = " + String(New_Date_Time_Data.field.Year));
            }
            if (New_Date_Time_Data.field.Year < 2020) {
                console_print("ERROR: Failed to get valid time after 30 seconds - proceeding anyway");
            } else {
                console_print("SUCCESS: Valid time obtained (Year: " + String(New_Date_Time_Data.field.Year) + ")");
            }
        }
        
        console_print("Setting Previous Date and Time to Today Date and Time");
        Current_Date_Time_Data.field.Day = New_Date_Time_Data.field.Day;
        Current_Date_Time_Data.field.Month = New_Date_Time_Data.field.Month;
        Last_Boot_Time_With = Current_Time_With;
        Last_Boot_Date_With = Current_Date_With;
        This_Date_With = Current_Date_With;
        This_Time_With = Current_Time_With;
        Create_or_Open_New_Data_File();
        if (Weather_Enabled) {
            console_print("Preparing Customised Weather Request");
            int count = 0;
            for (int x = 0; x <= 120; x++) {
                if (incomplete_Weather_Api_Link[x] == '\0') break;
                if (incomplete_Weather_Api_Link[x] != '#') {
                    Complete_Weather_Api_Link[count] = incomplete_Weather_Api_Link[x];
                    count++;
                }
                else {
                    for (int y = 0; y < strlen(Weather_City); y++) {
                        Complete_Weather_Api_Link[count++] = Weather_City[y];
                    }
                    Complete_Weather_Api_Link[count++] = ',';
                    for (int y = 0; y < strlen(Weather_Region); y++) {
                        Complete_Weather_Api_Link[count++] = Weather_Region[y];
                    }
                }
            }
            console_print("Weather Request Created: " + String(Complete_Weather_Api_Link));
        }
        else {
            console_print("Weather not Enabled");
        }
        // Initialize Watchdog Timer
        Timer0_Cfg = timerBegin(0, 80, true);                              // Timer 0, prescaler 80 (1MHz), count up
        timerAttachInterrupt(Timer0_Cfg, &resetModule, true);              // Attach ISR
        timerAlarmWrite(Timer0_Cfg, WDT_TiMEOUT * 1000000, false);         // Set alarm value in microseconds
        timerAlarmEnable(Timer0_Cfg);                                      // Enable the alarm
        console_print("Watchdog Timer Initialized - " + String(WDT_TiMEOUT) + " second timeout");
        digitalWrite(Blue_led_pin, LOW);
        
        // Log system information at startup (Issue #27 - Version Management)
        console_print("=== SYSTEM STARTUP ===");
        console_print(get_version_string());
        console_print("ESP32 Chip ID: " + String((uint32_t)ESP.getEfuseMac(), HEX));
        console_print("SDK Version: " + String(ESP.getSdkVersion()));
        log_system_health();
        console_print("======================");
        
        console_print("End of Setup");
        console_print("Running in Full Function Mode");
} // end of Setup
//------------

/**
 * @brief Arduino main loop - Continuous system operation
 * 
 * Executes the main operational cycle at configurable intervals (default 5 seconds):
 * - Feeds watchdog timer to prevent automatic reboot
 * - Handles web server requests
 * - Checks for date/time changes (new day, new month)
 * - Reads sensor data via RS485
 * - Updates weather information (every 10 minutes)
 * - Writes data to SD card
 * - Updates web display arrays
 * - Monitors system health (heap memory)
 * - Writes log buffer to SD card (every 60 seconds)
 * - Checks user buttons (gas meter, blue switch, etc.)
 * 
 * Timing is controlled by config.main_cycle_ms from configuration file.
 * All operations use non-blocking patterns to maintain system responsiveness.
 * 
 * @note Runs continuously after setup() completes
 * @note Watchdog timer will reboot system if loop() hangs for >5 seconds
 * @see Get_Sensor(), Get_Weather(), Write_New_Data_Record_to_Data_Files()
 */
void loop() {
    if (!Once) {
        console_print("Main Process Now Running");
        Once = true;
        digitalWrite(Green_led_pin, HIGH);
        Send_WhatsApp_Message("Power Monitor - System Rebooted " + Standard_Format_Date_Time + " | " + Get_Web_Interface_Address());
    }
    timerWrite(Timer0_Cfg, 0);                                      // Reset watchdog timer
    Check_WiFi();                                                   // check that the WiFi is still connected
    Check_Blue_Switch(false);
    Check_Gas_Switch();                                             // check if the gas switch sense pin has risen
    Check_NewDay();                                                 // check if the date has changed
    server.handleClient();                                          // handle any messages from the website
    if (millis() > Last_Cycle + config.main_cycle_ms) {            // configurable main cycle interval
        Get_Variable_Data();
    }
    Check_SD_Space();
    
    // Periodic system health logging and log file update (Issue #26)
    if (millis() - last_health_check >= config.health_check_ms) {  // configurable health check interval
        last_health_check = millis();
        log_system_health();
        write_log_to_sd();  // Write circular buffer to SD card
    }
}
//------------
void Get_Variable_Data() {
    Last_Cycle = millis();
    timerWrite(Timer0_Cfg, 0);  // Feed watchdog before long operations
    
    // Only fetch weather data every 10 minutes (not every cycle)
    bool weather_ok = true;
    if (millis() - Last_Weather_Read > config.weather_update_ms) {  // configurable weather interval
        weather_ok = Get_Weather();
        if (weather_ok) {
            Last_Weather_Read = millis();
        } else {
            console_print("Warning: Weather data fetch failed, using previous values");
        }
        timerWrite(Timer0_Cfg, 0);  // Feed watchdog after weather fetch
    }
    
    bool sensor_ok = Get_Sensor();
    timerWrite(Timer0_Cfg, 0);  // Feed watchdog after sensor fetch
    if (!sensor_ok) {
        console_print("Warning: Sensor data fetch failed, using previous values");
    }
    
    // Only write data if we have at least sensor data (sensor is critical, weather is optional)
    if (sensor_ok) {
        Update_Current_Timeinfo();
        Current_Date_With.toCharArray(Current_Data_Record.field.ldate, Current_Date_With.length() + 1);
        Current_Time_With.toCharArray(Current_Data_Record.field.ltime, Current_Time_With.length() + 1);
        
        // DEBUG: Log sensor readings to verify RS485 is working
        console_print("SENSOR: V=" + String(Current_Data_Record.field.Voltage, 2) + 
                     "V, I=" + String(Current_Data_Record.field.Amperage, 3) + 
                     "A, W=" + String(Current_Data_Record.field.wattage, 1) + "W");
        
        Write_New_Data_Record_to_Data_Files();
        Add_New_Data_Record_to_Display_Table();
    }
}

/**
 * @brief Read all sensor data via RS485 communication
 * 
 * Sends RS485 requests to KWS power monitoring hardware and receives sensor readings
 * including voltage, current, power, and energy data. Performs multiple requests
 * (Number_of_RS485_Requests) to read all available fields. Includes yield() in
 * receive loop to maintain system responsiveness.
 * 
 * @return true if all RS485 requests succeeded, false if any request failed
 * 
 * @note Timeout: RS485_TIMEOUT_MS (500ms) per request
 * @note Updates Current_Data_Record global structure with sensor values
 * @note Failure triggers automatic reboot after notification
 * @see Send_Request(), Receive(), KWS_Protocol.h
 */
bool Get_Sensor() {
    const int MAX_RETRIES = 3;
    for (int i = 0; i <= Number_of_RS485_Requests; i++) {
        bool success = false;
        for (int retry = 0; retry < MAX_RETRIES; retry++) {
            RS485_total_attempts++;
            Send_Request(i);
            if (Receive(i)) {
                success = true;
                break;
            }
            // Failed - wait before retry with increasing delay
            if (retry < MAX_RETRIES - 1) {
                delay(75 + (retry * 25));  // 75ms, 100ms, 125ms
            }
        }
        if (!success) {
            RS485_field_failures[i]++;
            RS485_total_failures++;
            console_print("RS485 - Failed to receive data for request " + String(i) + " after " + String(MAX_RETRIES) + " attempts");
            // Report statistics every 10 failures
            if (RS485_total_failures % 10 == 0) {
                console_print("RS485 Statistics: " + String(RS485_total_failures) + "/" + String(RS485_total_attempts) + 
                             " failed (" + String((RS485_total_failures * 100) / RS485_total_attempts) + "%)" );
                console_print("Field failures: " + String(RS485_field_failures[0]) + "," + String(RS485_field_failures[1]) + "," + 
                             String(RS485_field_failures[2]) + "," + String(RS485_field_failures[3]) + "," + 
                             String(RS485_field_failures[4]) + "," + String(RS485_field_failures[5]) + "," + 
                             String(RS485_field_failures[6]) + "," + String(RS485_field_failures[7]));
            }
            return false;
        }
        delay(75);  // Delay between successful requests (increased from 50ms)
    }
    return true;
}

/**
 * @brief Fetch current weather data from OpenWeatherMap API
 * 
 * Performs HTTP GET request to OpenWeatherMap API using the configured city and
 * region from config.json. Retrieves temperature, humidity, pressure, and weather
 * description. Rate-limited to one call per config.weather_update_ms (default
 * 10 minutes) to avoid API limits.
 * 
 * @return true if weather data retrieved and parsed successfully, false on any error
 * 
 * @note Requires WiFi connection to be active
 * @note Updates Weather_Temperature, Weather_Humidity globals
 * @note Can be disabled via config.weather_enabled = false
 * @note API key defined in SystemConfig structure
 * @see Parse_Weather_info(), Complete_Weather_Api_Link, config.weather_update_ms
 */
bool Get_Weather() {
    Update_Current_Timeinfo();
    HTTPClient http;
    if (!http.begin(Complete_Weather_Api_Link)) {
        console_print("Weather API - Failed to begin HTTP connection");
        return false;
    }
    
    int httpCode = http.GET();
    if (httpCode <= 0) {
        console_print("Weather API - HTTP GET failed, error: " + http.errorToString(httpCode));
        http.end();
        return false;
    }
    
    if (httpCode != HTTP_CODE_OK) {
        console_print("Obtaining Weather information Failed, Return code: " + String(httpCode));
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    if (payload.length() == 0) {
        console_print("Weather API - Empty response payload");
        return false;
    }
    
    return Parse_Weather_info(payload);
}

/**
 * @brief Detect day changes and create new data file
 * 
 * Compares current date against previous date to detect midnight rollover.
 * When a new day is detected:
 * - Creates a new daily CSV file with date in filename
 * - Sends WhatsApp notification
 * - Clears in-memory display arrays
 * - Checks for month change (triggers month-end processing)
 * 
 * Special handling for system reboot: If previous date is invalid (1-31 range),
 * assumes reboot and reopens existing file instead of creating new one.
 * 
 * @note Called from main loop every cycle
 * @note Side effect: Updates Current_Date_Time_Data.field.Day
 * @note Triggers month-end processing if month also changed
 * @see Check_NewMonth(), Create_or_Open_New_Data_File(), Update_Current_Timeinfo()
 */
void Check_NewDay() {
    Update_Current_Timeinfo();                                                      // update the Today_Date and Time
    if ((New_Date_Time_Data.field.Day != Current_Date_Time_Data.field.Day)) {       // is the day different to previous day
        String webAddress = Get_Web_Interface_Address();
        if (Current_Date_Time_Data.field.Day < 1 || Current_Date_Time_Data.field.Day > 31) { // if the Previous Date not set, we have rebooted
            console_print("Reopening Data File");
            Create_or_Open_New_Data_File();                                         // open the day file, do not create a new one
            Send_WhatsApp_Message("Power Monitor - Day Data File Reopened " + Standard_Format_Date_Time + " | " + webAddress);
            // Do NOT clear arrays after reboot - we just loaded data from SD card!
        }
        else {
            console_print("New Day Detected");                                      // proven day change
            Create_or_Open_New_Data_File();                                         // create a new day file
            unsigned long freeSpaceMB = (SD.totalBytes() - SD.usedBytes()) / (1024 * 1024);
            Send_WhatsApp_Message("Power Monitor - New Day Data File Opened " + Standard_Format_Date_Time + " | SD Free: " + String(freeSpaceMB) + "MB | " + webAddress);
            Check_NewMonth();                                                       // if the date changed check if a new month started
            Clear_Arrays();                                                         // clear memory for new day
        }
        Current_Date_Time_Data.field.Day = New_Date_Time_Data.field.Day;         // so set yesterday month
    }
}
/**
 * @brief Detect month changes and trigger month-end processing
 * 
 * Compares current month against previous month. When a new month is detected,
 * captures the last day's date/time and initiates month-end processing which:
 * - (Planned) Creates ZIP archive of previous month's daily files
 * - (Optionally) Deletes previous month's daily files after successful archive
 * - Sends WhatsApp notifications for month-end status
 * 
 * @note Called by Check_NewDay() when date changes
 * @note Side effect: Updates Current_Date_Time_Data.field.Month and .Year
 * @note Month-end processing runs asynchronously to avoid blocking main loop
 * @see Month_End_Process(), Check_NewDay()
 */
void Check_NewMonth() {
    if (New_Date_Time_Data.field.Month != Current_Date_Time_Data.field.Month) {  // month change
        console_print("New Month Detected");
        Month_End_Date_Time_Data.field.Day = Current_Date_Time_Data.field.Day;
        Month_End_Date_Time_Data.field.Month = Current_Date_Time_Data.field.Month;
        Month_End_Date_Time_Data.field.Year = Current_Date_Time_Data.field.Year;
        Month_End_Process();
        Current_Date_Time_Data.field.Month = New_Date_Time_Data.field.Month;     // so set yesterday month
        console_print("Current Month now set to " + New_Date_Time_Data.field.Month);
        Current_Date_Time_Data.field.Year = New_Date_Time_Data.field.Year;
    }
}

/**
 * @brief Add message to circular log buffer
 * 
 * Stores a log message with timestamp in the circular buffer. When buffer fills
 * (LOG_BUFFER_SIZE entries), oldest messages are overwritten. Buffer contents are
 * periodically written to SD card by write_log_to_sd().
 * 
 * @param message The message text to store (without timestamp)
 * 
 * @note Timestamp is automatically added using millis()
 * @note Buffer size: 500 entries (configurable via LOG_BUFFER_SIZE)
 * @note Side effect: Increments log_buffer_index and log_buffer_count
 * @see console_print(), write_log_to_sd(), LOG_BUFFER_SIZE
 */
void add_to_log_buffer(String message) {
    log_buffer[log_buffer_index].timestamp = millis();
    log_buffer[log_buffer_index].message = message;
    log_buffer_index = (log_buffer_index + 1) % LOG_BUFFER_SIZE;
    if (log_buffer_count < LOG_BUFFER_SIZE) {
        log_buffer_count++;
    }
}

/**
 * @brief Write entire log buffer to SD card
 * 
 * Writes all log entries from the circular buffer to /system.log on SD card.
 * Handles buffer wraparound correctly. Called automatically every 60 seconds
 * by the main loop to persist logs for post-mortem analysis after reboots.
 * 
 * @note Silently fails if SD card unavailable (to avoid recursion in error handling)
 * @note Overwrites previous /system.log file with current buffer snapshot
 * @note File format: timestamp(ms)\tmessage\n
 * @see add_to_log_buffer(), console_print(), LOG_FILENAME
 */
void write_log_to_sd() {
    File logFile = SD.open(LOG_FILENAME, FILE_WRITE);
    if (!logFile) {
        return;  // Silently fail to avoid recursion
    }
    
    // Calculate starting point for circular buffer
    int start_index = (log_buffer_count < LOG_BUFFER_SIZE) ? 0 : log_buffer_index;
    
    // Write all log entries
    for (int i = 0; i < log_buffer_count; i++) {
        int idx = (start_index + i) % LOG_BUFFER_SIZE;
        logFile.print(log_buffer[idx].timestamp);
        logFile.print("\t");
        logFile.println(log_buffer[idx].message);
    }
    
    logFile.close();
}

/**
 * @brief Print message to console with timestamp and log to buffer
 * 
 * Primary logging function used throughout the system. Prints message to serial
 * console with millisecond timestamp and tab separator, then stores in circular
 * log buffer for SD card persistence. All system messages should use this function
 * instead of direct Serial.print().
 * 
 * @param Monitor_Message The message text to print and log
 * 
 * @note Format: "<millis>\t<message>\n"
 * @note Side effect: Message is added to log buffer for SD card writing
 * @note Thread-safe for ESP32 RTOS tasks
 * @see add_to_log_buffer(), write_log_to_sd()
 */
void console_print(String Monitor_Message) {
    console.print(millis(), DEC); 
    console.println("\t" + Monitor_Message);
    add_to_log_buffer(Monitor_Message);
}

/**
 * @brief Log current system health metrics
 * 
 * Reports ESP32 heap memory statistics including free heap, minimum free heap
 * (lowest point since boot), and percentage utilization. Used to detect memory
 * leaks and heap fragmentation issues. Called at startup and periodically
 * every config.health_check_ms (default 60 seconds).
 * 
 * @note Logged via console_print() so appears in both console and log buffer
 * @note Healthy operation: >30% free heap, min_free_heap stable
 * @note Warning signs: free heap decreasing, min_free_heap dropping
 * @see ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize()
 */
void log_system_health() {
    unsigned long free_heap = ESP.getFreeHeap();
    unsigned long heap_size = ESP.getHeapSize();
    unsigned long min_free_heap = ESP.getMinFreeHeap();
    int heap_percent = (free_heap * 100) / heap_size;
    
    String health_msg = "HEALTH: Free=" + String(free_heap) + 
                       " Min=" + String(min_free_heap) + 
                       " (" + String(heap_percent) + "%)";
    console_print(health_msg);
}

/**
 * @brief Create default configuration file on SD card
 * 
 * Writes a default config.json file with hardcoded values for WiFi credentials,
 * NTP server, weather settings, timing intervals, and feature enables. Called
 * automatically if config.json is missing during startup. File is human-readable
 * and editable with any text editor.
 * 
 * Default configuration includes:
 * - WiFi: SSID, password
 * - NTP: server address
 * - Weather: city, region, enable flag
 * - WhatsApp: enable flag
 * - Debug: level (0-3)
 * - Intervals: main_cycle_ms, weather_update_ms, health_check_ms
 * - OTA: username, password for firmware updates
 * 
 * @note File format: JSON with standard formatting
 * @note File location: /config.json on SD card root
 * @see load_config_from_sd(), CONFIG_FILENAME, RuntimeConfig
 */
void create_default_config() {
    console_print("Creating default configuration file");
    
    File configFile = SD.open(CONFIG_FILENAME, FILE_WRITE);
    if (!configFile) {
        console_print("ERROR: Failed to create config file");
        return;
    }
    
    // Write default JSON configuration
    configFile.println("{");
    configFile.println("  \"wifi_ssid\": \"Woodleigh\",");
    configFile.println("  \"wifi_password\": \"2008198399\",");
    configFile.println("  \"ntp_server\": \"pool.ntp.org\",");
    configFile.println("  \"weather_city\": \"Basingstoke\",");
    configFile.println("  \"weather_region\": \"uk\",");
    configFile.println("  \"weather_enabled\": true,");
    configFile.println("  \"whatsapp_enabled\": true,");
    configFile.println("  \"gas_meter_enabled\": false,");
    configFile.println("  \"debug_level\": 2,");
    configFile.println("  \"main_cycle_ms\": 5000,");
    configFile.println("  \"weather_update_ms\": 600000,");
    configFile.println("  \"health_check_ms\": 60000,");
    configFile.println("  \"ota_username\": \"SteveG\",");
    configFile.println("  \"ota_password\": \"W00dle1gh\"");
    configFile.println("}");
    
    configFile.close();
    console_print("Default configuration file created");
}

/**
 * @brief Load configuration from SD card and apply to system
 * 
 * Reads config.json from SD card, parses each configuration parameter, and loads
 * into the global 'config' structure. Supports strings, booleans, and integers.
 * Uses manual JSON parsing to avoid library dependency and memory overhead.
 * If config file is missing, creates a default configuration.
 * 
 * Configurable parameters:
 * - wifi_ssid, wifi_password: WiFi credentials
 * - ntp_server: Time server address
 * - weather_city, weather_region: Location for weather API
 * - weather_enabled, whatsapp_enabled: Feature toggles
 * - debug_level: 0=none, 1=errors, 2=info, 3=verbose
 * - main_cycle_ms: Main loop interval (default 5000)
 * - weather_update_ms: Weather API interval (default 600000)
 * - health_check_ms: Health monitoring interval (default 60000)
 * - ota_username, ota_password: OTA firmware update credentials
 * 
 * @return true if config loaded successfully, false if file missing (default created)
 * 
 * @note All loaded values are logged to console for verification
 * @note Caller must apply config values to runtime variables (SSID, Password, etc.)
 * @see create_default_config(), CONFIG_FILENAME, RuntimeConfig
 */
bool load_config_from_sd() {
    if (!SD.exists(CONFIG_FILENAME)) {
        console_print("Config file not found, creating default");
        create_default_config();
        return false;
    }
    
    File configFile = SD.open(CONFIG_FILENAME, FILE_READ);
    if (!configFile) {
        console_print("ERROR: Failed to open config file");
        return false;
    }
    
    console_print("Loading configuration from SD card");
    
    // Parse JSON manually (simple parser for our specific format)
    String line;
    while (configFile.available()) {
        line = configFile.readStringUntil('\n');
        line.trim();
        
        if (line.indexOf("wifi_ssid") >= 0) {
            int start = line.indexOf("\"", line.indexOf(":")) + 1;
            int end = line.indexOf("\"", start);
            config.wifi_ssid = line.substring(start, end);
        }
        else if (line.indexOf("wifi_password") >= 0) {
            int start = line.indexOf("\"", line.indexOf(":")) + 1;
            int end = line.indexOf("\"", start);
            config.wifi_password = line.substring(start, end);
        }
        else if (line.indexOf("ntp_server") >= 0) {
            int start = line.indexOf("\"", line.indexOf(":")) + 1;
            int end = line.indexOf("\"", start);
            config.ntp_server = line.substring(start, end);
        }
        else if (line.indexOf("weather_city") >= 0) {
            int start = line.indexOf("\"", line.indexOf(":")) + 1;
            int end = line.indexOf("\"", start);
            config.weather_city = line.substring(start, end);
        }
        else if (line.indexOf("weather_region") >= 0) {
            int start = line.indexOf("\"", line.indexOf(":")) + 1;
            int end = line.indexOf("\"", start);
            config.weather_region = line.substring(start, end);
        }
        else if (line.indexOf("weather_enabled") >= 0) {
            config.weather_enabled = (line.indexOf("true") >= 0);
        }
        else if (line.indexOf("whatsapp_enabled") >= 0) {
            config.whatsapp_enabled = (line.indexOf("true") >= 0);
        }
        else if (line.indexOf("gas_meter_enabled") >= 0) {
            config.gas_meter_enabled = (line.indexOf("true") >= 0);
        }
        else if (line.indexOf("debug_level") >= 0) {
            int start = line.indexOf(":") + 1;
            config.debug_level = line.substring(start).toInt();
        }
        else if (line.indexOf("main_cycle_ms") >= 0) {
            int start = line.indexOf(":") + 1;
            config.main_cycle_ms = line.substring(start).toInt();
        }
        else if (line.indexOf("weather_update_ms") >= 0) {
            int start = line.indexOf(":") + 1;
            config.weather_update_ms = line.substring(start).toInt();
        }
        else if (line.indexOf("health_check_ms") >= 0) {
            int start = line.indexOf(":") + 1;
            config.health_check_ms = line.substring(start).toInt();
        }
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
    }
    
    configFile.close();
    
    console_print("Configuration loaded:");
    console_print("  WiFi SSID: " + config.wifi_ssid);
    console_print("  NTP Server: " + config.ntp_server);
    console_print("  Weather: " + config.weather_city + ", " + config.weather_region);
    console_print("  Weather Enabled: " + String(config.weather_enabled ? "Yes" : "No"));
    console_print("  WhatsApp Enabled: " + String(config.whatsapp_enabled ? "Yes" : "No"));
    console_print("  Gas Meter Enabled: " + String(config.gas_meter_enabled ? "Yes" : "No"));
    console_print("  Debug Level: " + String(config.debug_level));
    console_print("  Main Cycle: " + String(config.main_cycle_ms) + "ms");
    console_print("  OTA Username: " + config.ota_username);
    
    return true;
}

void Month_End_Process() {
    char ZipFileName[20];
    char month_end_file_month[3];
    String this_file_month = "00";
    console_print("Commencing Month End Process for Month " + String(Month_End_Date_Time_Data.field.Month));
    Send_WhatsApp_Message("Power Monitor - Month End Process Commenced for " + String(Month_Names[Month_End_Date_Time_Data.field.Month]));
    Month_End_Process_Status = true;
    if (Month_End_Date_Time_Data.field.Month < 10) {
        snprintf(month_end_file_month, sizeof(month_end_file_month), "0%d", Month_End_Date_Time_Data.field.Month);
    }
    else {
        snprintf(month_end_file_month, sizeof(month_end_file_month), "%d", Month_End_Date_Time_Data.field.Month);
    }
    snprintf(ZipFileName, sizeof(ZipFileName), "MF%s.zip", month_end_file_month);    // Create the final zip filename
    if (ZipAllFilesToSD(ZipFileName)) {
        console_print("Month End Completed for Month " + String(Month_End_Date_Time_Data.field.Month));
        Send_WhatsApp_Message("Power Monitor - Month End Process Completed for " + String(Month_Names[Month_End_Date_Time_Data.field.Month]));
    }
    else {
        console_print("Month End Process Failed for Month " + String(Month_End_Date_Time_Data.field.Month));
        Send_WhatsApp_Message("Power Monitor - Month End Process Failed for " + String(Month_Names[Month_End_Date_Time_Data.field.Month]));
    }
}
int Delete_Previous_Month_Daily_Files(int targetMonth) {
    // Delete all daily files from the target month
    // Returns number of files deleted
    String targetMonthStr = String(targetMonth);
    if (targetMonth < 10) {
        targetMonthStr = "0" + targetMonthStr;
    }
    
    int filesDeleted = 0;
    File root = SD.open("/");
    
    while (true) {
        File file = root.openNextFile();
        if (!file) break;
        
        String fileName = String(file.name());
        
        // Check if this is a daily file for the target month (format: DD_MM_YY.csv)
        if (!file.isDirectory() && fileName.endsWith(".csv") && fileName.indexOf("_" + targetMonthStr + "_") > 0) {
            file.close();
            console_print("Deleting: " + fileName);
            if (SD.remove("/" + fileName)) {
                filesDeleted++;
            } else {
                console_print("WARNING: Failed to delete " + fileName);
            }
        } else {
            file.close();
        }
    }
    
    root.close();
    console_print("Deleted " + String(filesDeleted) + " daily files from month " + String(targetMonth));
    return filesDeleted;
}

bool ZipAllFilesToSD(const char* zipFilename) {
    // Consolidate all previous month's daily files into one monthly file
    // Format: MF01.csv, MF02.csv, etc. (MF = Monthly File)
    String monthlyFileName = String(zipFilename);
    monthlyFileName.replace(".zip", ".csv");  // Use .csv instead of .zip
    
    console_print("Creating monthly consolidated file: " + monthlyFileName);
    
    // Delete existing monthly file if it exists
    if (SD.exists("/" + monthlyFileName)) {
        SD.remove("/" + monthlyFileName);
    }
    
    // Open monthly file for writing
    File monthlyFile = SD.open("/" + monthlyFileName, FILE_WRITE);
    if (!monthlyFile) {
        DEBUG_ERROR("ERROR: Failed to create monthly file: " + monthlyFileName);
        return false;
    }
    
    // Write header (same as daily file header) - FIXED: Use actual column names
    for (int x = 0; x < Number_of_Column_Field_Names - 1; x++) {
        monthlyFile.print(Column_Field_Names[x]);
        monthlyFile.print(",");
    }
    monthlyFile.println(Column_Field_Names[Number_of_Column_Field_Names - 1]);
    
    // Get the month we're consolidating (previous month)
    String targetMonth = String(Month_End_Date_Time_Data.field.Month);
    if (Month_End_Date_Time_Data.field.Month < 10) {
        targetMonth = "0" + targetMonth;
    }
    
    int filesProcessed = 0;
    File root = SD.open("/");
    
    while (true) {
        File file = root.openNextFile();
        if (!file) break;
        
        String fileName = String(file.name());
        
        // Check if this is a daily file for the target month (format: DD_MM_YY.csv)
        if (!file.isDirectory() && fileName.endsWith(".csv") && fileName.indexOf("_" + targetMonth + "_") > 0) {
            console_print("Adding file: " + fileName);
            
            // Open the daily file for reading
            file.seek(0);
            
            // Skip the header line of each daily file
            String headerLine = file.readStringUntil('\n');
            
            // Copy all data lines to monthly file
            while (file.available()) {
                String line = file.readStringUntil('\n');
                if (line.length() > 0) {
                    monthlyFile.println(line);
                }
            }
            
            filesProcessed++;
        }
        
        file.close();
    }
    
    root.close();
    monthlyFile.close();
    
    console_print("Monthly consolidation complete: " + String(filesProcessed) + " files processed");
    console_print("Month End Process for Month " + String(Month_End_Date_Time_Data.field.Month) + " Complete");
    Send_WhatsApp_Message("Power Monitor - Month End Process Complete. " + String(filesProcessed) + " daily files consolidated into " + monthlyFileName);
    
    // Auto-delete the daily files now that they're consolidated
    console_print("Auto-deleting consolidated daily files from month " + String(Month_End_Date_Time_Data.field.Month));
    int deletedCount = Delete_Previous_Month_Daily_Files(Month_End_Date_Time_Data.field.Month);
    Send_WhatsApp_Message("Power Monitor - Auto-deleted " + String(deletedCount) + " daily files after successful consolidation");
    
    return true;
}

/**
 * @brief Send notification via WhatsApp using CallMeBot API
 * 
 * Sends a WhatsApp message to the configured phone number using the CallMeBot
 * web service. Implements duplicate message suppression to prevent spam - identical
 * messages within 5 minutes are blocked. Can be globally disabled via
 * config.whatsapp_enabled flag.
 * 
 * Used for critical notifications:
 * - System reboots and errors
 * - SD card failures
 * - Month-end processing status
 * - New data file creation
 * 
 * @param message The message text to send (will be URL-encoded)
 * 
 * @note Rate limiting: Prevents duplicate messages within WHATSAPP_DUPLICATE_INTERVAL_MS
 * @note Requires WiFi connection to be active
 * @note Can be disabled via config.whatsapp_enabled = false
 * @note Uses CallMeBot API (requires API key and phone number registration)
 * @see WhatsApp_Enabled, WHATSAPP_DUPLICATE_INTERVAL_MS
 */
void Send_WhatsApp_Message(String message) {
    // Prevent duplicate messages within 5 minutes
    if (message == whatsapp_cache.last_message && 
        (millis() - whatsapp_cache.last_message_time) < WHATSAPP_DUPLICATE_INTERVAL_MS) {
        console_print("WhatsApp duplicate message blocked: " + message + " (sent " + 
                     String((millis() - whatsapp_cache.last_message_time) / 1000) + " seconds ago)");
        return;
    }
    
    // https://api.callmebot.com/whatsapp.php?source=web
    // &phone=+447785938200
    // &apikey=8876034&text=
    String This_WhatsApp_url = "https://api.callmebot.com/whatsapp.php";
    This_WhatsApp_url.reserve(200 + message.length());  // Pre-allocate URL string
    This_WhatsApp_url += "?phone=+447785938200";                                        // telephone number
    This_WhatsApp_url += "&apikey=8876034";                                     // app key
    This_WhatsApp_url += "&text=";
    This_WhatsApp_url += urlEncode(message);                                    // message
    HTTPClient http;
    http.begin(This_WhatsApp_url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");    // Specify content-type header
    int httpResponseCode = http.POST(This_WhatsApp_url);      // Send HTTP POST request
    if (httpResponseCode == 200) {
        console_print("WhatsApp Message " + message + " sent successfully at " + Current_Time_With);
        // Update cache on successful send
        whatsapp_cache.last_message = message;
        whatsapp_cache.last_message_time = millis();
    }
    else {
        DEBUG_ERROR("Error sending the WhatsApp message: " + message + " Response Code:" + String(httpResponseCode));
    }
    http.end();
}

/**
 * @brief Write current sensor data to daily CSV file on SD card
 * 
 * Appends the current data record to the daily CSV file with comma-separated values.
 * Includes automatic retry logic (3 attempts) if file operations fail. Verifies
 * all write operations using verifyFileWrite() helper. If all retries fail, sends
 * WhatsApp notification and triggers system reboot.
 * 
 * CSV format includes:
 * - Date/time, voltage (3 phases), current (3 phases)
 * - Power, energy, frequency, temperature
 * - Gas volume, weather data (if enabled)
 * 
 * @note Side effect: Toggles Blue LED during SD card activity
 * @note Retry logic: 3 attempts with SD_FILE_RETRY_DELAY_MS between attempts
 * @note All write operations are verified for success
 * @note Failure triggers automatic reboot after WhatsApp notification
 * @see verifyFileWrite(), DataFileName, Current_Data_Record
 */
void Write_New_Data_Record_to_Data_Files() {
    toggleLED(Blue_led_pin);                              // flash the SD activity LED on
    bool file_opened = false;
    for (int retry = 1; retry <= SD_RETRY_ATTEMPTS; retry++) {
        DataFile = SD.open("/" + DataFileName, FILE_APPEND);
        if (DataFile) {
            file_opened = true;
            break;
        }
        DEBUG_ERROR("Error re-opening DataFile:" + String(DataFileName) + " - Retry " + String(retry) + " of " + String(SD_RETRY_ATTEMPTS));
        if (retry < SD_RETRY_ATTEMPTS) {
            smartDelay(1000);  // Wait 1 second before retry (keeps system responsive)
        }
    }
    if (!file_opened) {
        DEBUG_ERROR("Error re-opening DataFile:" + String(DataFileName) + " after " + String(SD_RETRY_ATTEMPTS) + " attempts - Rebooting in 5 seconds");
        Send_WhatsApp_Message("Power Monitor - Error re-opening DataFile: " + String(DataFileName) + " after " + String(SD_RETRY_ATTEMPTS) + " attempts - Automatic Reboot");
        smartDelay(5000);
        ESP.restart();
    }
    console_print("New Data Record Written to " + DataFileName);
    
    // Write data record with verification (Issue #22 - File Operations Lack Verification)
    size_t written = 0;
    bool write_success = true;
    
    written = DataFile.print(Current_Data_Record.field.ldate);
    write_success &= verifyFileWrite(DataFile, DataFileName, written, "date");
    written = DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.ltime);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.Voltage);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.Amperage);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.wattage);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.uptime);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.kilowatthour);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.powerfactor);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.frequency);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.sensor_temperature);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.weather_temperature);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.temperature_feels_like);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.temperature_maximum);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.temperature_minimum);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.atmospheric_pressure);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.relative_humidity);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.wind_speed);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.wind_direction);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.gas_volume);
    written += DataFile.print(",");
    written += DataFile.print(Current_Data_Record.field.weather_description);
    written += DataFile.println();
    
    write_success &= verifyFileWrite(DataFile, DataFileName, written, "data record");
    
    if (!write_success) {
        DEBUG_ERROR("Data record write verification failed for " + DataFileName);
        DataFile.close();
        Send_WhatsApp_Message("Power Monitor - Data write verification failed - " + DataFileName);
        smartDelay(2000);
        ESP.restart();
    }
    
    DataFile.close();
    DataFile.flush();
    Data_Record_Count++;                                                      // increment the current record count
    if (Current_Data_Record.field.Voltage >= Highest_Voltage || Highest_Voltage == 0) {
        Date_of_Highest_Voltage = Current_Data_Record.field.ldate;
        Time_of_Highest_Voltage = Current_Data_Record.field.ltime;
        Highest_Voltage = Current_Data_Record.field.Voltage;                   // update the largest current value
    }
    if (Current_Data_Record.field.Voltage <= Lowest_Voltage || Lowest_Voltage == 0) {
        Date_of_Lowest_Voltage = Current_Data_Record.field.ldate;
        Time_of_Lowest_Voltage = Current_Data_Record.field.ltime;
        Lowest_Voltage = Current_Data_Record.field.Voltage;                    // update the largest current value
    }
    if (Current_Data_Record.field.Amperage >= Highest_Amperage || Highest_Amperage == 0) {
        Date_of_Highest_Amperage = Current_Data_Record.field.ldate;
        Time_of_Highest_Amperage = Current_Data_Record.field.ltime;
        Highest_Amperage = Current_Data_Record.field.Amperage;                  // update the largest current value
    }
    Latest_Kilowatthour = Current_Data_Record.field.kilowatthour;
    Time_of_Latest_reading = Current_Data_Record.field.ltime;
    Latest_weather_temperature = Current_Data_Record.field.weather_temperature;
    Latest_weather_temperature_feels_like = Current_Data_Record.field.temperature_feels_like;
    Latest_weather_temperature_maximum = Current_Data_Record.field.temperature_maximum;
    Latest_weather_temperature_minimum = Current_Data_Record.field.temperature_minimum;
    Latest_atmospheric_pressure = Current_Data_Record.field.atmospheric_pressure;
    Latest_relative_humidity = Current_Data_Record.field.relative_humidity;
    Latest_wind_speed = Current_Data_Record.field.wind_speed;
    Latest_wind_direction = Current_Data_Record.field.wind_direction;
    Latest_weather_description = Current_Data_Record.field.weather_description;
    if (Current_Data_Record.field.gas_volume != Latest_Gas_Volume || Latest_Gas_Volume == 0) {
        Latest_Gas_Volume = Current_Data_Record.field.gas_volume;
        if (Latest_Gas_Volume != 0) {
            Latest_Gas_Date_With = Current_Data_Record.field.ldate;
            Latest_Gas_Time_With = Current_Data_Record.field.ltime;
        }
        else {
            Latest_Gas_Date_With = "";
            Latest_Gas_Time_With = "";
        }
    }
    smartDelay(100);                                        // Keep LED visible for 100ms (maintains responsiveness)
    toggleLED(Blue_led_pin);                                // Turn LED back off
}
void Check_SD_Space() {
    SD_freespace = (SD.totalBytes() - SD.usedBytes());
    SD_freespace_double = (double)SD_freespace / 1000000;
    if (SD_freespace < Critical_SD_Freespace) {
        console_print("WARNING - SD Free Space critical " + String(SD_freespace) + "MBytes");
        Send_WhatsApp_Message("Power Monitor - SD Space Critical " + String(SD.usedBytes() / 1000000) + " MBytes used");
    }
}
void Add_New_Data_Record_to_Display_Table() {
    if (Data_Table_Pointer == Data_Table_Size) {                                        // table full, shuffle fifo
        Shuffle_Data_Table();
        Data_Table_Pointer = Data_Table_Size - 1;       // subsequent records will be added at the end of the table
    }
    for (int x = 0; x < 11; x++) {
        Readings_Table[Data_Table_Pointer].field.ldate[x] = Current_Data_Record.field.ldate[x];        // [0]  Date
    }
    for (int x = 0; x < 9; x++) {
        Readings_Table[Data_Table_Pointer].field.ltime[x] = Current_Data_Record.field.ltime[x];        // [1]  time
    }
    Readings_Table[Data_Table_Pointer].field.Voltage = Current_Data_Record.field.Voltage;           // [2]  Voltage
    Readings_Table[Data_Table_Pointer].field.Amperage = Current_Data_Record.field.Amperage;        // [3]  Amperage
    Readings_Table[Data_Table_Pointer].field.wattage = Current_Data_Record.field.wattage;           // [4]  wattage
    Readings_Table[Data_Table_Pointer].field.uptime = Current_Data_Record.field.uptime;              // [5]  uptime
    Readings_Table[Data_Table_Pointer].field.kilowatthour = Current_Data_Record.field.kilowatthour;// [6] kilowatthours
    Readings_Table[Data_Table_Pointer].field.powerfactor = Current_Data_Record.field.powerfactor;// [7]  power factor
    Readings_Table[Data_Table_Pointer].field.frequency = Current_Data_Record.field.frequency;     // [8]  frequency
    Readings_Table[Data_Table_Pointer].field.sensor_temperature = Current_Data_Record.field.sensor_temperature;      // [9] sensor temperature
    Readings_Table[Data_Table_Pointer].field.weather_temperature = Current_Data_Record.field.weather_temperature;    // [10] weather temperature
    Readings_Table[Data_Table_Pointer].field.temperature_feels_like = Current_Data_Record.field.temperature_feels_like;// [11] temperature feels like
    Readings_Table[Data_Table_Pointer].field.temperature_maximum = Current_Data_Record.field.temperature_maximum;    // [12] temperature maximum
    Readings_Table[Data_Table_Pointer].field.temperature_minimum = Current_Data_Record.field.temperature_minimum;    // [13] temperature minimum
    Readings_Table[Data_Table_Pointer].field.atmospheric_pressure = Current_Data_Record.field.atmospheric_pressure;  // [14] atmospheric pressure
    Readings_Table[Data_Table_Pointer].field.relative_humidity = Current_Data_Record.field.relative_humidity;        // [15] relative humidity
    Readings_Table[Data_Table_Pointer].field.wind_speed = Current_Data_Record.field.wind_speed;                      // [16] wind speed
    Readings_Table[Data_Table_Pointer].field.weather_temperature = Current_Data_Record.field.weather_temperature;    // [17] wind direction
    for (int x = 0; x < strlen(Current_Data_Record.field.weather_description); x++) {
        Readings_Table[Data_Table_Pointer].field.weather_description[x] = Current_Data_Record.field.weather_description[x];
    }
    Readings_Table[Data_Table_Pointer].field.gas_volume = Current_Data_Record.field.gas_volume;                      // [19] gas volume
    Data_Table_Pointer++;
}
void Create_or_Open_New_Data_File() {
    Update_Current_Timeinfo();
    DataFileName = String(Current_Date_Without) + ".csv";                           // create the file name for the current date
    if (!SD.exists("/" + DataFileName)) {
        bool file_created = false;
        for (int retry = 1; retry <= SD_RETRY_ATTEMPTS; retry++) {
            DataFile = SD.open("/" + DataFileName, FILE_WRITE);
            if (DataFile) {
                file_created = true;
                break;
            }
            DEBUG_ERROR("Error creating Data file: " + String(DataFileName) + " - Retry " + String(retry) + " of " + String(SD_RETRY_ATTEMPTS));
            if (retry < SD_RETRY_ATTEMPTS) {
                smartDelay(1000);  // Wait 1 second before retry (keeps system responsive)
            }
        }
        if (!file_created) {
            DEBUG_ERROR("Error creating Data file: " + String(DataFileName) + " after " + String(SD_RETRY_ATTEMPTS) + " attempts - Rebooting in 5 seconds");
            Send_WhatsApp_Message("Power Monitor - Error creating Data file: " + String(DataFileName) + " after " + String(SD_RETRY_ATTEMPTS) + " attempts - Automatic Reboot");
            smartDelay(5000);
            ESP.restart();
        }
        console_print("Day Data File " + DataFileName + " created");
        
        // Write column headings with verification (Issue #22 - File Operations Lack Verification)
        size_t header_written = 0;
        for (int x = 0; x < Number_of_Column_Field_Names - 1; x++) {
            header_written += DataFile.print(Column_Field_Names[x]);
            header_written += DataFile.print(",");
        }
        header_written += DataFile.println(Column_Field_Names[Number_of_Column_Field_Names - 1]);
        
        if (!verifyFileWrite(DataFile, DataFileName, header_written, "column headers")) {
            DEBUG_ERROR("Column header write verification failed for " + DataFileName);
            DataFile.close();
            Send_WhatsApp_Message("Power Monitor - Header write failed - " + DataFileName);
            smartDelay(2000);
            ESP.restart();
        }
        
        console_print("Column Headings written to Day Data File");
        DataFile.close();
        DataFile.flush();
        console_print("Day Data File Closed and Flushed");
        Data_Record_Count = 0;
    }
    else {                                                                          // file already exists, so open it for append
        bool file_reopened = false;
        for (int retry = 1; retry <= SD_RETRY_ATTEMPTS; retry++) {
            DataFile = SD.open("/" + DataFileName, FILE_APPEND);
            if (DataFile) {
                file_reopened = true;
                break;
            }
            console_print("Error opening Data file: " + String(DataFileName) + " - Retry " + String(retry) + " of " + String(SD_RETRY_ATTEMPTS));
            if (retry < SD_RETRY_ATTEMPTS) {
                smartDelay(1000);  // Wait 1 second before retry (keeps system responsive)
            }
        }
        if (!file_reopened) {
            console_print("Error reopening Data file: " + String(DataFileName) + " after " + String(SD_RETRY_ATTEMPTS) + " attempts - Rebooting in 5 seconds");
            Send_WhatsApp_Message("Power Monitor - Error reopening Data File " + String(DataFileName) + " after " + String(SD_RETRY_ATTEMPTS) + " attempts - Automatic Reboot");
            smartDelay(5000);
            ESP.restart();
        }
        console_print("Day Data File " + DataFileName + " reopened for append");
        DataFile.close();
        DataFile.flush();
        Prefill_Data_Array();
    }
    This_Date_With = Current_Date_With;                                             // update the current date
    This_Date_Without = Current_Date_Without;
    This_Day = Current_Date_Time_Data.field.Day;
}

/**
 * @brief Monitor and recover WiFi connection with exponential backoff
 * 
 * Continuously monitors WiFi status and attempts recovery if connection is lost.
 * Implements intelligent retry strategy:
 * - Exponential backoff: 3s → 6s → 12s → 24s → 48s → max 60s between attempts
 * - After 5 failures: Restart WiFi radio (hard reset)
 * - After 10 attempts (>60s): Reboot entire system
 * 
 * Logs disconnect reasons for debugging:
 * - SSID not available (router offline)
 * - Connection failed (wrong password)
 * - Connection lost (interference/range)
 * - Disconnected (normal disconnect)
 * 
 * @note Called from main loop every cycle (non-blocking)
 * @note Static variables maintain state across calls
 * @note Sends WhatsApp notification before system reboot
 * @see StartWiFi(), smartDelay()
 */
void Check_WiFi() {
    static int wifi_connection_attempts = 0;
    static unsigned long lastWiFiCheck = 0;
    static unsigned long retry_interval = 3000;  // Start with 3 seconds
    static int consecutive_failures = 0;
    
    // Exponential backoff WiFi recovery (Issue #21 - WiFi Recovery Logic)
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastWiFiCheck >= retry_interval) {
            wifi_connection_attempts++;
            consecutive_failures++;
            lastWiFiCheck = millis();
            
            // Log disconnect reason for debugging
            wl_status_t status = WiFi.status();
            String status_str = (status == WL_NO_SSID_AVAIL) ? "SSID not available" :
                               (status == WL_CONNECT_FAILED) ? "Connection failed" :
                               (status == WL_CONNECTION_LOST) ? "Connection lost" :
                               (status == WL_DISCONNECTED) ? "Disconnected" : "Unknown";
            
            console_print("WiFi Connection Failed (" + status_str + "), Attempting to Reconnect");
            console_print("Connection attempt " + String(wifi_connection_attempts) + 
                         " with " + String(retry_interval/1000) + "s backoff");
            
            // After 5 failures, restart WiFi radio
            if (consecutive_failures == 5) {
                console_print("5 consecutive WiFi failures - Restarting WiFi radio");
                WiFi.disconnect(true);
                delay(1000);
                WiFi.mode(WIFI_OFF);
                delay(1000);
                WiFi.mode(WIFI_STA);
                delay(1000);
            }
            
            // After 10 failures (>60 seconds), full system reboot
            if (wifi_connection_attempts >= 10) {
                console_print("Network Error, WiFi lost >60 seconds, Restarting Processor");
                Send_WhatsApp_Message("Power Monitor - WiFi Recovery Failed - Automatic Reboot");
                smartDelay(2000);
                ESP.restart();
            }
            
            StartWiFi(SSID, Password);
            
            // Exponential backoff: 3s, 6s, 12s, 24s, 48s, max 60s
            retry_interval = min(retry_interval * 2, 60000UL);
        }
    } else {
        // Reset counters and backoff when connected
        if (wifi_connection_attempts > 0) {
            console_print("WiFi reconnected successfully after " + String(wifi_connection_attempts) + " attempts");
        }
        wifi_connection_attempts = 0;
        consecutive_failures = 0;
        retry_interval = 3000;  // Reset to 3 seconds
    }
}
/**
 * @brief Initiate WiFi connection attempt
 * 
 * Starts a new WiFi connection to the specified SSID and password. If already
 * connected, disconnects first to ensure clean connection state. Uses non-blocking
 * pattern with smartDelay() to keep system responsive during connection attempt.
 * 
 * Connection process:
 * 1. Check current WiFi status
 * 2. Disconnect if already connected
 * 3. Begin new connection with credentials
 * 4. Wait with smartDelay() (feeds watchdog, handles web requests)
 * 5. Return final connection status
 * 
 * @param ssid The WiFi network SSID to connect to
 * @param password The WiFi network password
 * 
 * @return WiFi connection status code (WL_CONNECTED, WL_NO_SSID_AVAIL, etc.)
 * 
 * @note Uses smartDelay() to maintain system responsiveness during connection
 * @note Non-blocking after initial attempt (Check_WiFi handles retries)
 * @see Check_WiFi(), WiFi.status(), WiFi.begin()
 */
int StartWiFi(String ssid, String password) {
    console_print("WiFi Connecting to " + String(ssid));
    if (WiFi.status() == WL_CONNECTED) {                                // disconnect to start new wifi connection
        console_print("WiFi Already Connected");
        console_print("Disconnecting");
        WiFi.disconnect(true);
        console_print("Disconnected");
    }
    WiFi.begin(SSID, Password);                                                     // connect to the wifi network
    int WiFi_Status = WiFi.status();
    console_print("WiFi Status: " + String(WiFi_Status) + " " + WiFi_Status_Message[WiFi_Status]);
    int wifi_connection_attempts = 0;                                                  // zero the attempt counter
    while (WiFi.status() != WL_CONNECTED) {                              // whilst it is not connected keep trying
        delay(3000);
        console_print("Connection attempt " + String(wifi_connection_attempts));
        if (wifi_connection_attempts++ > 20) {
            console_print("Network Error, Not able to open WiFi, Restarting Processor");
            ESP.restart();
        }
    }
    WiFi_Status = WiFi.status();
    console_print("WiFi Status: " + String(WiFi_Status) + " " + WiFi_Status_Message[WiFi_Status]);
    WiFi_Signal_Strength = (int)WiFi.RSSI();
    console_print("WiFi Signal Strength:" + String(WiFi_Signal_Strength));
    console_print("WiFi IP Address: " + String(WiFi.localIP().toString().c_str()));
    return true;
}
String Get_Web_Interface_Address() {
    if (WiFi.status() != WL_CONNECTED) {
        return "Web: unavailable";
    }

    String localIp = WiFi.localIP().toString();
    if (localIp.length() == 0 || localIp == "0.0.0.0") {
        return "Web: unavailable";
    }

    String webAddress = "Web: http://" + localIp + "/";
    if (hostname.length() > 0) {
        webAddress += " | Name: http://" + hostname + ".local/";
    }

    return webAddress;
}
String Build_Device_Hostname() {
    uint32_t suffix = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF);
    char hostnameBuffer[32];
    snprintf(hostnameBuffer, sizeof(hostnameBuffer), "power-monitor-%06X", suffix);
    return String(hostnameBuffer);
}
void StartMDNSService() {
    if (!MDNS.begin(hostname.c_str())) {
        console_print("mDNS responder failed for " + hostname);
        return;
    }

    MDNS.addService("http", "tcp", 80);
    console_print("mDNS responder started: http://" + hostname + ".local/");
}
void initTime(String timezone) {
    console_print("Starting Time Server");
    configTime(gmtOffset_sec, daylightOffset_sec, NtpServer);
    setenv("TZ", timezone.c_str(), 1);                                        // adjust time settings to London
    tzset();
    console_print("Time Server Started");
}

/**
 * @brief Parse string field from CSV and copy to destination (helper for Prefill_Data_Array)
 * 
 * Extracts a string field from CSV data and safely copies it to the destination buffer
 * with size checking. Optionally prints debug output if PRiNT_PREFiLL_DATA_VALUES is defined.
 * Used during data array prefill to load date/time strings from historical data.
 * 
 * @param field_number The CSV field index (for debug output)
 * @param field Pointer to the CSV field string to parse
 * @param destination Pointer to destination buffer to receive the string
 * @param dest_size Size of destination buffer (for safe copy)
 * 
 * @note Uses strncpy for safe string copy with size limit
 * @note Debug output controlled by PRiNT_PREFiLL_DATA_VALUES compile flag
 * @see Prefill_Data_Array(), Process_Prefill_Field()
 */
void Parse_CSV_Field_String(int field_number, char* field, char* destination, size_t dest_size) {
    strncpy(destination, field, dest_size);
#ifdef PRiNT_PREFiLL_DATA_VALUES
    console_print("\tPrefill Data Array [" + String(Data_Table_Pointer) + "] Field " + String(field_number) + ": " + String(destination));
#endif
}

/**
 * @brief Parse numeric field from CSV and convert to double (helper for Prefill_Data_Array)
 * 
 * Extracts a numeric field from CSV data and converts it to double precision floating point.
 * Handles voltage, current, power, energy, temperature, and other sensor readings.
 * Optionally prints debug output if PRiNT_PREFiLL_DATA_VALUES is defined.
 * 
 * @param field_number The CSV field index (for debug output)
 * @param field Pointer to the CSV field string to parse
 * 
 * @return double The converted numeric value
 * 
 * @note Uses atof() for ASCII-to-float conversion
 * @note Returns 0.0 if field is invalid or empty
 * @note Debug output controlled by PRiNT_PREFiLL_DATA_VALUES compile flag
 * @see Prefill_Data_Array(), Process_Prefill_Field()
 */
double Parse_CSV_Field_Double(int field_number, char* field) {
    double value = atof(field);
#ifdef PRiNT_PREFiLL_DATA_VALUES
    console_print("\tPrefill Data Array [" + String(Data_Table_Pointer) + "] Field " + String(field_number) + ": " + String(value));
#endif
    return value;
}

/**
 * @brief Route CSV field to appropriate data structure member (helper for Prefill_Data_Array)
 * 
 * Large switch statement that maps CSV field numbers to the corresponding members
 * of the Readings_Table data structure. Handles 40+ different fields including:
 * - Date/time (fields 0-1)
 * - Voltage readings 3-phase (fields 2-4)
 * - Current readings 3-phase (fields 5-7)
 * - Power, energy, frequency (fields 8-20)
 * - Temperature and gas volume (fields 21-22)
 * - Weather data if enabled (fields 23-26)
 * 
 * Calls Parse_CSV_Field_String() for string fields and Parse_CSV_Field_Double()
 * for numeric fields.
 * 
 * @param field_number The CSV column index (0-based)
 * @param field Pointer to the CSV field string to process
 * 
 * @note Refactored in V12.27 to eliminate code duplication
 * @note Updates Readings_Table[Data_Table_Pointer] with parsed values
 * @see Parse_CSV_Field_String(), Parse_CSV_Field_Double(), Prefill_Data_Array()
 */
void Process_Prefill_Field(int field_number, char* field) {
    switch (field_number) {
    case 0: // Date
        Parse_CSV_Field_String(field_number, field, Readings_Table[Data_Table_Pointer].field.ldate, 
                               sizeof(Readings_Table[Data_Table_Pointer].field.ldate));
        break;
    case 1: // Time
        Parse_CSV_Field_String(field_number, field, Readings_Table[Data_Table_Pointer].field.ltime, 
                               sizeof(Readings_Table[Data_Table_Pointer].field.ltime));
        break;
    case 2: // Voltage
        Readings_Table[Data_Table_Pointer].field.Voltage = Parse_CSV_Field_Double(field_number, field);
        break;
    case 3: // Amperage
        Readings_Table[Data_Table_Pointer].field.Amperage = Parse_CSV_Field_Double(field_number, field);
        break;
    case 4: // Wattage
        Readings_Table[Data_Table_Pointer].field.wattage = Parse_CSV_Field_Double(field_number, field);
        break;
    case 5: // Uptime
        Readings_Table[Data_Table_Pointer].field.uptime = Parse_CSV_Field_Double(field_number, field);
        break;
    case 6: // Kilowatthour
        Readings_Table[Data_Table_Pointer].field.kilowatthour = Parse_CSV_Field_Double(field_number, field);
        break;
    case 7: // Power factor
        Readings_Table[Data_Table_Pointer].field.powerfactor = Parse_CSV_Field_Double(field_number, field);
        break;
    case 8: // Frequency
        Readings_Table[Data_Table_Pointer].field.frequency = Parse_CSV_Field_Double(field_number, field);
        break;
    case 9: // Sensor temperature
        Readings_Table[Data_Table_Pointer].field.sensor_temperature = Parse_CSV_Field_Double(field_number, field);
        break;
    case 10: // Weather temperature
        Readings_Table[Data_Table_Pointer].field.weather_temperature = Parse_CSV_Field_Double(field_number, field);
        break;
    case 11: // Temperature feels like
        Readings_Table[Data_Table_Pointer].field.temperature_feels_like = Parse_CSV_Field_Double(field_number, field);
        break;
    case 12: // Temperature maximum
        Readings_Table[Data_Table_Pointer].field.temperature_maximum = Parse_CSV_Field_Double(field_number, field);
        break;
    case 13: // Temperature minimum
        Readings_Table[Data_Table_Pointer].field.temperature_minimum = Parse_CSV_Field_Double(field_number, field);
        break;
    case 14: // Atmospheric pressure
        Readings_Table[Data_Table_Pointer].field.atmospheric_pressure = Parse_CSV_Field_Double(field_number, field);
        break;
    case 15: // Relative humidity
        Readings_Table[Data_Table_Pointer].field.relative_humidity = Parse_CSV_Field_Double(field_number, field);
        break;
    case 16: // Wind speed
        Readings_Table[Data_Table_Pointer].field.wind_speed = Parse_CSV_Field_Double(field_number, field);
        break;
    case 17: // Wind direction
        Readings_Table[Data_Table_Pointer].field.wind_direction = Parse_CSV_Field_Double(field_number, field);
        break;
    case 18: // Gas volume
        Readings_Table[Data_Table_Pointer].field.gas_volume = Parse_CSV_Field_Double(field_number, field);
        Current_Data_Record.field.gas_volume = Readings_Table[Data_Table_Pointer].field.gas_volume;
        if (Current_Data_Record.field.gas_volume != 0 && Current_Data_Record.field.gas_volume != Latest_Gas_Volume) {
            Latest_Gas_Volume = Current_Data_Record.field.gas_volume;
            Latest_Gas_Date_With = Readings_Table[Data_Table_Pointer].field.ldate;
            Latest_Gas_Time_With = Readings_Table[Data_Table_Pointer].field.ltime;
        }
        break;
    case 19: // Weather description
        Parse_CSV_Field_String(field_number, field, Readings_Table[Data_Table_Pointer].field.weather_description, 
                               sizeof(Readings_Table[Data_Table_Pointer].field.weather_description));
        break;
    default:
        break;
    }
}

/**
 * @brief Load existing CSV data from SD card into memory for web display
 * 
 * Reads the current day's CSV data file and populates the in-memory display arrays
 * used for the web interface graphs and tables. Parses CSV format line-by-line,
 * extracting fields using helper functions. Handles file wraparound when array
 * fills (Number_of_Records > Max_Table_Size).
 * 
 * Refactored in V12.27 from 326 lines to ~90 lines using helper functions:
 * - Parse_CSV_Field_String() for string fields
 * - Parse_CSV_Field_Double() for numeric fields
 * - Process_Prefill_Field() for field routing
 * 
 * @note Called during startup and when date changes (new file created)
 * @note Side effect: Toggles Blue LED during SD card reading
 * @note Loads up to Max_Table_Size records (most recent if file larger)
 * @note Updates Data_Table_Pointer to track current position
 * @see Parse_CSV_Field_String(), Parse_CSV_Field_Double(), Process_Prefill_Field()
 */
void Prefill_Data_Array() {
    int character_count = 0;
    char field[50];
    int field_number = 0;
    char data_temp;
    String message;
    unsigned long last_led_toggle = millis();  // Track last LED toggle time
    toggleLED(Blue_led_pin);                          // flash the blue led
    console_print("Pre Loading DataFile " + String(DataFileName) + " into Data Array");
    File DataFile = SD.open("/" + DataFileName, FILE_READ);
    if (DataFile) {
        while (DataFile.available()) {                                 // throw the first row, column headers, away
            data_temp = DataFile.read();
            if (data_temp == '\n') break;
        }
        while (DataFile.available()) {                                       // do while there are data available
            // Toggle LED every 2.5 seconds during prefill
            if (millis() - last_led_toggle >= 2500) {
                toggleLED(Blue_led_pin);
                last_led_toggle = millis();
            }
            data_temp = DataFile.read();
            field[character_count++] = data_temp;                        // add it to the csvfield string
            if (data_temp == ',' || data_temp == '\n') {                              // look for end of field
                field[character_count - 1] = '\0';       // insert termination character where the ',' or '\n' was
                Process_Prefill_Field(field_number, field);  // Use helper function (Issue #15)
                field_number++;
                field[0] = '\0';
                character_count = 0;
            }
            if (data_temp == '\n') {               // at this point the obtained record has been saved in the table
                Update_Webpage_Variables_from_Table(Data_Table_Pointer); // update web variables with record just saved
                Data_Table_Pointer++;                                          // increment the table array pointer
                Data_Record_Count++;                                          // increment the running record total
                if (Data_Table_Pointer == Data_Table_Size) {               // if pointer is greater than table size
                    Shuffle_Data_Table();
                    Data_Table_Pointer = Data_Table_Size - 1;
                }
                field_number = 0;
            } // end of end of line detected
        } // end of while
    }
    else {
        console_print("ERROR: Failed to open " + String(DataFileName) + " for prefill");
    }
    DataFile.close();
    console_print("Loaded Data Records: " + String(Data_Record_Count));
    console_print("Data_Table_Pointer now at: " + String(Data_Table_Pointer));
    if (Data_Table_Pointer == 0) {
        console_print("NOTE: No data loaded - webpage will display 'Waiting for data' until first sensor reading");
    }
}
void Shuffle_Data_Table() {
    for (int i = 0; i < (Data_Table_Size - 1); i++) {       // shuffle the rows up, losing row 0, make row [table_size] free
        strncpy(Readings_Table[i].field.ldate, Readings_Table[i + 1].field.ldate, sizeof(Readings_Table[i].field.ldate));                       // [0]  date
        strncpy(Readings_Table[i].field.ltime, Readings_Table[i + 1].field.ltime, sizeof(Readings_Table[i].field.ltime));                       // [1]  time
        Readings_Table[i].field.Voltage = Readings_Table[i + 1].field.Voltage;                          // Voltage
        Readings_Table[i].field.Amperage = Readings_Table[i + 1].field.Amperage;                        // Amperage
        Readings_Table[i].field.wattage = Readings_Table[i + 1].field.wattage;                          // Wattage
        Readings_Table[i].field.uptime = Readings_Table[i + 1].field.uptime;                            // uptime
        Readings_Table[i].field.kilowatthour = Readings_Table[i + 1].field.kilowatthour;                // kwhs
        Readings_Table[i].field.powerfactor = Readings_Table[i + 1].field.powerfactor;                  // pf
        Readings_Table[i].field.frequency = Readings_Table[i + 1].field.frequency;                      // freq
        Readings_Table[i].field.sensor_temperature = Readings_Table[i + 1].field.sensor_temperature;    // senstemp
        Readings_Table[i].field.weather_temperature = Readings_Table[i + 1].field.weather_temperature;  // w temp
        Readings_Table[i].field.temperature_feels_like = Readings_Table[i + 1].field.temperature_feels_like;// tempfl
        Readings_Table[i].field.temperature_maximum = Readings_Table[i + 1].field.temperature_maximum;  // temp max
        Readings_Table[i].field.temperature_minimum = Readings_Table[i + 1].field.temperature_minimum;  // temp min
        Readings_Table[i].field.atmospheric_pressure = Readings_Table[i + 1].field.atmospheric_pressure;// ap
        Readings_Table[i].field.relative_humidity = Readings_Table[i + 1].field.relative_humidity;      // r h
        Readings_Table[i].field.wind_speed = Readings_Table[i + 1].field.wind_speed;                    // wspeed
        Readings_Table[i].field.wind_direction = Readings_Table[i + 1].field.wind_direction;            // wdir
        Readings_Table[i].field.gas_volume = Readings_Table[i + 1].field.gas_volume;                    // gas v
        strncpy(Readings_Table[i].field.weather_description, Readings_Table[i + 1].field.weather_description, sizeof(Readings_Table[i].field.weather_description));// [19] weather description
    }
}
void Update_Webpage_Variables_from_Table(int Data_Table_Pointer) {
    if (Readings_Table[Data_Table_Pointer].field.Voltage >= Highest_Voltage || Highest_Voltage == 0) {
        Date_of_Highest_Voltage = String(Readings_Table[Data_Table_Pointer].field.ldate);
        Time_of_Highest_Voltage = String(Readings_Table[Data_Table_Pointer].field.ltime);
        Highest_Voltage = Readings_Table[Data_Table_Pointer].field.Voltage;
    }
    if ((Readings_Table[Data_Table_Pointer].field.Voltage <= Lowest_Voltage) || Lowest_Voltage == 0) {
        Date_of_Lowest_Voltage = String(Readings_Table[Data_Table_Pointer].field.ldate);
        Time_of_Lowest_Voltage = String(Readings_Table[Data_Table_Pointer].field.ltime);
        Lowest_Voltage = Readings_Table[Data_Table_Pointer].field.Voltage;      // update the largest current value
    }
    if (Readings_Table[Data_Table_Pointer].field.Amperage >= Highest_Amperage || Highest_Amperage == 0) { // load the maximum Amperage value
        Date_of_Highest_Amperage = String(Readings_Table[Data_Table_Pointer].field.ldate);
        Time_of_Highest_Amperage = String(Readings_Table[Data_Table_Pointer].field.ltime);
        Highest_Amperage = Readings_Table[Data_Table_Pointer].field.Amperage;  // update the largest current value
    }
    Latest_Kilowatthour = Readings_Table[Data_Table_Pointer].field.kilowatthour;
    Latest_weather_temperature_feels_like = Readings_Table[Data_Table_Pointer].field.temperature_feels_like;
    Latest_weather_temperature_maximum = Readings_Table[Data_Table_Pointer].field.temperature_maximum;
    Latest_weather_temperature_minimum = Readings_Table[Data_Table_Pointer].field.temperature_minimum;
    Latest_relative_humidity = Readings_Table[Data_Table_Pointer].field.relative_humidity;
    Latest_atmospheric_pressure = Readings_Table[Data_Table_Pointer].field.atmospheric_pressure;
    Latest_wind_speed = Readings_Table[Data_Table_Pointer].field.wind_speed;
    Latest_wind_direction = Readings_Table[Data_Table_Pointer].field.wind_direction;
    Latest_weather_description = Readings_Table[Data_Table_Pointer].field.weather_description;
    if (Readings_Table[Data_Table_Pointer].field.gas_volume != 0) {
        Latest_Gas_Volume = Readings_Table[Data_Table_Pointer].field.gas_volume;
        Latest_Gas_Date_With = Readings_Table[Data_Table_Pointer].field.ldate;
        Latest_Gas_Time_With = Readings_Table[Data_Table_Pointer].field.ltime;
    }
}
void Page_Header(bool refresh, String Header) {
    webpage.reserve(6000);
    webpage = "";
    webpage = "<!DOCTYPE html><head>";
    webpage += F("<html lang='en'>");                                                              // start of HTML section
    if (refresh) webpage += F("<meta http-equiv='refresh' content='20'>");              // 20-sec refresh time
    webpage += F("<meta name='viewport' content='width=");
    webpage += site_width;
    webpage += F(", initial-scale=1'>");
    webpage += F("<meta http-equiv='Cache-control' content='public'>");
    webpage += F("<meta http-equiv='x-Content-Type-Options:nosniff'>;");
    webpage += F("<title>");
    webpage += F("</title>");
    webpage += F("<h1 ");                                                               // start of h1
    webpage += F("style='text-align:center;'>");
    webpage += F("<span style='color:DodgerBlue; font-size:36pt;'>");                          // start of span
    webpage += Header;
    webpage += F("</span>");                                                            // end of span
    webpage += F("<span style='font-size: medium;'><align center=''></align></span>");
    webpage += F("</h1>");                                                              // end of h1
    webpage += F("<style>ul{list-style-type:none;margin:0;padding:0;overflow:hidden;background-color:");
    webpage += F("#31c1f9; font-size:14px}");
    webpage += F("li{float:left;}");
    webpage += F("li a{display:block;text-align:center;padding:5px 15px;text-decoration:none;}");
    webpage += F("li a:hover{background-color:#FFFFFF;}");
    webpage += F("h1{background-color:White;}");
    webpage += F("body{width:");
    webpage += site_width;
    webpage += F("px;margin:0 auto;font-family:arial;font-size:14px;text-align:center;");
    webpage += F("color:#ed6495;background-color:#F7F2Fd;}");
    webpage += F("</style>");                                                           // end of style section
    webpage += F("</head>");                                                            // end of head section
    webpage += F("<body>");                                                             // start of body
}
void Page_Footer() {
    char signature[20] = { 0xA9,0x53,0x74,0x65,0x70,0x68,0x65,0x6E,0x20,0x47,0x6F,0x75,0x6C,
                            0x64,0x20,0x32,0x30,0x32,0x32,0x00 };
    Update_Current_Timeinfo();
    webpage += F("<ul>");
    webpage += F("<li><a href='/Display'>Webpage</a> </li>");
    webpage += F("<li><a href='/Information'>Information</a></li>");
    webpage += F("<li><a href='/DownloadFiles'>Download</a></li>");
    webpage += F("<li><a href='/DeleteFiles'>Delete Files</a></li>");
    webpage += F("<li><a href='/DeletePreviousMonth'>Delete Prev Month</a></li>");
    webpage += F("<li><a href='/WhatsApp'>WhatsApp Test</a></li>");
    webpage += F("<li><a href='/Reset'>Reset</a></li>");
    webpage += F("<li><a href='/Firmware'>Firmware</a></li>");
    webpage += F("<li><a href='/ZipAllFiles'>ZipAllFiles</a></li>");
    webpage += F("</ul>");
    webpage += F("<footer>");
    webpage += F("<p style='text-align: center;'>");
    webpage += F("<span ");
    webpage += F("style = 'color: red;'");
    webpage += F(">");
    webpage += String(signature);
    webpage += F(" (");
    webpage += String(VERSION_STRING);
    webpage += F(") Last Page Update - ");
    webpage += String(Current_Time_With);
    webpage += F(" SD Free Space = ");
    webpage += String(SD_freespace_double, 2) + " MB";
    webpage += F("</span>");
    webpage += F("</p>");
    webpage += F("</footer>");
    webpage += F("</body>");
    webpage += F("</html>");
    server.send(200, "text/html", webpage);
    webpage = "";
}
void i_Display() {
    double maximum_Amperage = 0;
    int non_zero_count = 0;
    console_print("Web Display of Graph Requested via Webpage");
    webpage = "";
    webpage.reserve(8000);  // Pre-allocate for graph data
    Page_Header(true, "Energy Usage Monitor " + String(Current_Date_With) + " " + String(Current_Time_With));
    
    // Check if we have any data to display
    if (Data_Table_Pointer == 0) {
        // No data yet - show waiting message
        webpage += F("<div style='padding:50px; text-align:center;'>");
        webpage += F("<h2 style='color:#FF6600;'>⏳ Waiting for Data...</h2>");
        webpage += F("<p style='font-size:18px;'>The system is collecting sensor data.</p>");
        webpage += F("<p style='font-size:16px;'>Data collection cycle: ");
        webpage += String(config.main_cycle_ms / 1000);
        webpage += F(" seconds</p>");
        webpage += F("<p style='font-size:14px; color:#888;'>This page will refresh automatically in 20 seconds.</p>");
        webpage += F("<p style='font-size:14px; color:#888;'>Data records loaded: ");
        webpage += String(Data_Record_Count);
        webpage += F(" | Data pointer: ");
        webpage += String(Data_Table_Pointer);
        webpage += F("</p>");
        webpage += F("</div>");
    }
    else {
        // Check if we have any valid non-zero sensor data in the entire array
        for (int i = 0; i < Data_Table_Pointer; i++) {
            if (Readings_Table[i].field.Amperage > 0.001) {  // Check for > 1mA to avoid float precision issues
                non_zero_count++;
                break;  // Found at least one non-zero reading, that's enough
            }
        }
        
        if (non_zero_count == 0) {
            // Data exists but all sensor readings are zero - RS485 communication issue
            webpage += F("<div style='padding:50px; text-align:center;'>");
            webpage += F("<h2 style='color:#FF0000;'>⚠ RS485 Communication Issue</h2>");
            webpage += F("<p style='font-size:18px;'>Sensor data is being logged, but all readings are zero.</p>");
            webpage += F("<p style='font-size:16px;'>Check RS485 connections:</p>");
            webpage += F("<ul style='text-align:left; display:inline-block; font-size:14px;'>");
            webpage += F("<li>DE pin (GPIO 22) - Driver Enable</li>");
            webpage += F("<li>RE pin (GPIO 25) - Receiver Enable</li>");
            webpage += F("<li>TX pin - RS485 transmit</li>");
            webpage += F("<li>RX pin - RS485 receive</li>");
            webpage += F("<li>Power supply to KWS sensor</li>");
            webpage += F("</ul>");
            webpage += F("<p style='font-size:14px; color:#888;'>Data records: ");
            webpage += String(Data_Table_Pointer);
            webpage += F(" entries (all zeros)</p>");
            webpage += F("<p style='font-size:14px; color:#888;'>Check <a href='/Information'>Information</a> page or system.log for RS485 errors</p>");
            webpage += F("<p style='font-size:14px; color:#888;'>This page will refresh automatically in 20 seconds.</p>");
            webpage += F("</div>");
        }
        else {
            // We have valid data - display the graph
        webpage += F("<script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>");
        webpage += F("<script type=\"text/javascript\">");
        webpage += F("google.charts.load('current', {packages: ['corechart', 'line']});");
        webpage += F("google.setOnLoadCallback(drawChart);");
        webpage += F("function drawChart() {");
        webpage += F("var data=new google.visualization.DataTable();");
        webpage += F("data.addColumn('timeofday', 'Time');");
        webpage += F("data.addColumn('number', 'Amperage');");
        webpage += F("data.addColumn('number', 'Gas Volume');");
        webpage += F("data.addRows([");
        for (int i = 0; i < (Data_Table_Pointer); i++) {
            if (String(Readings_Table[i].field.ltime) != "") {                     // if the ltime field contains data
                for (int x = 0; x < 8; x++) {                                    // replace the ":"s in ltime with ","
                    if (Readings_Table[i].field.ltime[x] == ':') {
                        Readings_Table[i].field.ltime[x] = ',';
                    }
                }
                webpage += "[[";
                webpage += String(Readings_Table[i].field.ltime) + "],";
                webpage += String(Readings_Table[i].field.Amperage, 1) + ",";
                webpage += String(Readings_Table[i].field.gas_volume, 1) + "]";
                if (Readings_Table[i].field.Amperage > maximum_Amperage) {
                    maximum_Amperage = Readings_Table[i].field.Amperage;
                }
                if (i != Data_Table_Pointer) {
                    webpage += ",";
                }
            }
        }
        webpage += "]);\n";
        webpage += F("var options = {");
        webpage += F("title:'Electrical and Gas Consumption logarithmic scale',");
        webpage += F("titleTextStyle:{");
        webpage += F("fontName:'Arial',");
        webpage += F("fontSize: 20, ");
        webpage += F("color: 'DodgerBlue'");
        webpage += F("},");
        webpage += F("legend:{");
        webpage += F("position:'bottom'");
        webpage += F("},");
        webpage += F("colors: [");
        webpage += F("'red', 'blue'");
        webpage += F("],");
        webpage += F("backgroundColor : '#F3F3F3',");
        webpage += F("chartArea: {");
        webpage += F("width:'80%',");
        webpage += F("height: '55%'");
        webpage += F("}, ");
        webpage += F("hAxis:{");
        webpage += F("slantedText:'true',");
        webpage += F("slantedTextAngle:'90',");
        webpage += F("titleTextStyle:{");
        webpage += F("color:'Purple', ");
        webpage += F("bold: 'true',");
        webpage += F("fontSize: '16'");
        webpage += F("}, ");
        webpage += F("format: 'HH:mm:ss',");
        webpage += F("gridlines: {");
        webpage += F("color:'#333'");
        webpage += F("},");
        webpage += F("showTextEvery:'1'");
        webpage += F("}, ");
        webpage += F("vAxes:");
        webpage += F("{0:{viewWindowMode:'explcit',scaleType:'log',title:'Current Amperage',format:'###.###'},");
        webpage += F(" 1:{scaleType:'log',title:'Daily Cumulative Gas Volume cu.litre',format:'###.###'},},");
        webpage += F("series:{0:{targetAxisindex:0},1:{targetAxisindex:1},curveType:'none'},};");
        webpage += F("var chart = new google.visualization.LineChart(document.getElementById('line_chart'));");
        webpage += F("chart.draw(data, options); ");
        webpage += F("}");
        webpage += F("</script>");
        webpage += F("<div id='line_chart' style='width:960px; height:600px'></div>");
        }  // End of valid data check
    }  // End of Data_Table_Pointer > 0 check
    
    Page_Footer();
    lastcall = "display";
}
void i_WhatsApp() {
    console_print("WhatsApp Test Message Requested via Webpage");
    webpage.reserve(2000);  // Pre-allocate for simple page
    Page_Header(false, "Sending Test WhatsApp  Message" + String(Current_Date_With) + " " + String(Current_Time_With));
    if (millis() > (WhatsApp_Time + 1000)) {
        WhatsApp_Time = millis();
        Send_WhatsApp_Message("Power Monitor - Sending Test Message from Webpage at " + String(Current_Time_With));
    }
    Page_Footer();
}
void i_Web_Reset() {
    console_print("Web Reset of Processor Requested via Webpage");
    webpage.reserve(2000);  // Pre-allocate for simple page
    Page_Header(false, "Resetting Processor at " + String(Current_Date_With) + " " + String(Current_Time_With));
    if (millis() > (Reset_Time + 1000)) {
        Reset_Time = millis();
        Send_WhatsApp_Message("Power Monitor - Resetting Processor from Webpage at " + String(Current_Time_With));
    }
    Page_Footer();
    ESP.restart();
}
void i_Config_Upload() {
    console_print("Config Upload Page Requested via Webpage");
    webpage.reserve(3500);  // Pre-allocate for upload page
    Page_Header(false, "Configuration File Management");
    
    webpage += F("<h3>Current Configuration</h3>");
    webpage += F("<table style='border:1px solid #ccc; padding:10px;'>");
    webpage += String(F("<tr><td><strong>WiFi SSID:</strong></td><td>")) + config.wifi_ssid + String(F("</td></tr>"));
    webpage += String(F("<tr><td><strong>Weather Location:</strong></td><td>")) + config.weather_city + String(F(", ")) + config.weather_region + String(F("</td></tr>"));
    webpage += String(F("<tr><td><strong>Weather Enabled:</strong></td><td>")) + String(config.weather_enabled ? "Yes" : "No") + String(F("</td></tr>"));
    webpage += String(F("<tr><td><strong>WhatsApp Enabled:</strong></td><td>")) + String(config.whatsapp_enabled ? "Yes" : "No") + String(F("</td></tr>"));
    webpage += String(F("<tr><td><strong>Gas Meter Enabled:</strong></td><td>")) + String(config.gas_meter_enabled ? "Yes" : "No") + String(F("</td></tr>"));
    webpage += F("</table><br>");
    
    webpage += F("<h3>Download Current Configuration</h3>");
    webpage += F("<p>Download the current config.json file to edit on your PC:</p>");
    webpage += F("<a href='/DownloadConfig'><button style='padding:10px 20px;'>Download config.json</button></a>");
    webpage += F("<br><br>");
    
    webpage += F("<h3>Upload New Configuration</h3>");
    webpage += F("<p>Upload a modified config.json file. ESP32 will validate and reboot to apply changes.</p>");
    webpage += F("<form method='POST' action='/UploadConfig' enctype='multipart/form-data'>");
    webpage += F("<input type='file' name='config' accept='.json'><br><br>");
    webpage += F("<input type='submit' value='Upload and Reboot' style='padding:10px 20px;'>");
    webpage += F("</form>");
    
    webpage += F("<br><p style='color:red;'><strong>Note:</strong> ESP32 will reboot automatically after successful upload.</p>");
    
    Page_Footer();
}
void i_Download_Config() {
    console_print("Config Download Requested via Webpage");
    
    File configFile = SD.open(CONFIG_FILENAME, FILE_READ);
    if (!configFile) {
        server.send(404, "text/plain", "Config file not found");
        console_print("ERROR: Config file not found for download");
        return;
    }
    
    server.sendHeader("Content-Disposition", "attachment; filename=config.json");
    server.streamFile(configFile, "application/json");
    configFile.close();
    console_print("Config file downloaded successfully");
}
void i_Handle_Config_Upload() {
    HTTPUpload& upload = server.upload();
    static File uploadFile;
    
    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        console_print("Config Upload Start: " + filename);
        
        // Save uploaded file as temporary
        uploadFile = SD.open("/config_new.json", FILE_WRITE);
        if (!uploadFile) {
            console_print("ERROR: Failed to create temporary config file");
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
            console_print("Config Upload Complete: " + String(upload.totalSize) + " bytes");
        }
    }
}
void i_Upload_Config_Complete() {
    webpage.reserve(2500);
    
    // Validate the uploaded config file
    File newConfig = SD.open("/config_new.json", FILE_READ);
    if (!newConfig) {
        Page_Header(false, "Upload Failed");
        webpage += F("<p style='color:red;'><strong>ERROR:</strong> Failed to read uploaded file.</p>");
        webpage += F("<br><a href='/ConfigUpload'>Try Again</a>");
        Page_Footer();
        console_print("ERROR: Failed to open uploaded config for validation");
        return;
    }
    
    // Basic validation - check for key fields
    String content = newConfig.readString();
    newConfig.close();
    
    bool valid = true;
    if (content.indexOf("wifi_ssid") < 0 || content.indexOf("wifi_password") < 0) {
        valid = false;
        console_print("ERROR: Config validation failed - missing required fields");
    }
    
    if (!valid) {
        SD.remove("/config_new.json");
        Page_Header(false, "Upload Failed");
        webpage += F("<p style='color:red;'><strong>ERROR:</strong> Invalid config file. Missing required fields (wifi_ssid, wifi_password).</p>");
        webpage += F("<br><a href='/ConfigUpload'>Try Again</a>");
        Page_Footer();
        return;
    }
    
    // Backup old config
    if (SD.exists(CONFIG_FILENAME)) {
        SD.remove("/config_backup.json");
        SD.rename(CONFIG_FILENAME, "/config_backup.json");
        console_print("Old config backed up to config_backup.json");
    }
    
    // Replace with new config
    SD.rename("/config_new.json", CONFIG_FILENAME);
    console_print("New config installed successfully");
    
    Page_Header(false, "Config Upload Successful");
    webpage += F("<p style='color:green;'><strong>SUCCESS:</strong> Configuration file uploaded and validated.</p>");
    webpage += F("<p>Old configuration backed up to <strong>config_backup.json</strong></p>");
    webpage += F("<p><strong>ESP32 will reboot in 3 seconds to apply new configuration...</strong></p>");
    webpage += F("<script>setTimeout(function(){ window.location.href = '/'; }, 5000);</script>");
    Page_Footer();
    
    console_print("Rebooting to apply new configuration...");
    smartDelay(3000);
    ESP.restart();
}
void i_Information() {                                                    // Display file size of the datalog file
    console_print("Web Display of information Requested via Webpage");
    String ht = String(Date_of_Highest_Voltage + " at " + Time_of_Highest_Voltage + " as ");
    ht += String(Highest_Voltage) + " volts";
    String lt = String(Date_of_Lowest_Voltage + " at " + Time_of_Lowest_Voltage + " as ");
    lt += String(Lowest_Voltage) + " volts";
    String ha = String(Date_of_Highest_Amperage + " at " + Time_of_Highest_Amperage + " as ");
    ha += String(Highest_Amperage) + " amps";
    File DataFile = SD.open("/" + DataFileName, FILE_READ);                // Now read data from FS
    webpage = "";
    webpage.reserve(5000);  // Pre-allocate for information page
    Page_Header(true, "Energy Usage Monitor " + String(Current_Date_With) + " " + String(Current_Time_With));
    // 1.Wifi Signal Strength ---------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true;font-size:14px; '");
    webpage += F(">WiFi Signal Strength = ");
    webpage += String(WiFi_Signal_Strength) + " Dbm";
    webpage += "</span></strong></p>";
    // Row 2.Data File Size -----------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F(">Current Data file size = ");
    webpage += String(DataFile.size());
    webpage += F(" Bytes");
    webpage += "</span></strong></p>";
    // Row 3.Freespace ----------------------------------------------------------------------------
    if (SD_freespace < Critical_SD_Freespace) {
        webpage += F("<p ");
        webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
        webpage += F("bold:true; font-size:14px; '");
        webpage += F("'>SD Free Space = ");
        webpage += String(SD_freespace / 1000000);
        webpage += F(" MB");
        webpage += "</span></strong></p>";
    }
    else {
        webpage += F("<p ");
        webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
        webpage += F("bold:true; font-size:14px; '");
        webpage += F("'>SD Free Space = ");
        webpage += String(SD_freespace / 1000000);
        webpage += F(" MB");
        webpage += "</span></strong></p>";
    }
    // Row 4.File Count ---------------------------------------------------------------------------
    int file_count = Count_Files_on_SD_Drive();
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F("'> Number of Files on SD : ");
    webpage += String(file_count);
    webpage += "</span></strong></p>";
    // Row 5.Last Boot Time -----------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F("'>Date the System was Booted : ");
    webpage += Last_Boot_Date_With + " at " + Last_Boot_Time_With;
    webpage += "</span></strong></p>";
    // Row 6.Data Record Count --------------------------------------------------------------------
    double Percentage = (Data_Record_Count * (double)100) / (double)17280;
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F("'>Number of Data Readings: ");
    webpage += String(Data_Record_Count);
    webpage += F(" Percentage of Full Day: ");
    webpage += String(Percentage, 0);
    webpage += F("%");
    webpage += "</span></strong></p>";
    // Row 7.Highest Voltage ----
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F("'>Greatest Voltage was recorded on ");
    webpage += ht;
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 8.Lowest Voltage -----
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F(">Least Voltage was recorded on ");
    webpage += lt;
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 9.Highest Amperage ---
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F("'>Greatest Amperage was recorded on ");
    webpage += ha;
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 10.Cumulative KiloWattHours ---------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F("'>Current KiloWatt Hours: ");
    webpage += String(Latest_Kilowatthour, 3);
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 11.Weather Latest Weather Temperature Feels Like, Maximum & Minimum --------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F(">Temperature: ");
    webpage += String(Latest_weather_temperature, 2);
    webpage += F("&deg;C,");
    webpage += F("  Feels Like: ");
    webpage += String(Latest_weather_temperature_feels_like, 2);
    webpage += F("&deg;C,");
    webpage += F("  Maximum: ");
    webpage += String(Latest_weather_temperature_maximum, 2);
    webpage += F("&deg;C,");
    webpage += F("  Minimum: ");
    webpage += String(Latest_weather_temperature_minimum, 2);
    webpage += F("&deg;C");
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 12.Weather Latest Weather Relative Humidity --------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F(">Relative Humidity: ");
    webpage += String(Latest_relative_humidity) + "%";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 13.Weather Latest Atmospheric Pressure -------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F(">Atmospheric Pressure: ");
    webpage += String(Latest_atmospheric_pressure) + " millibars";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 14.Weather Latest Wind Speed and Direction ---------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font - size:12px; '");
    webpage += F(">Wind Speed: ");
    webpage += String(Latest_wind_speed) + "m/s,";
    webpage += F(" Direction: ");
    webpage += String(Latest_wind_direction);
    webpage += F("&deg;");
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 15.Weather Latest Weather Description --------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font - size:12px; '");
    webpage += F("'>Weather: ");
    webpage += String(Latest_weather_description);
    webpage += "</span></strong></p>";
    // Row 16.Last Gas Sensor Reading ----------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F(">Date and Time of Last Gas Switch Signal: ");
    webpage += String(Latest_Gas_Date_With) + " at " + Latest_Gas_Time_With;
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 17.Latest Gas Volume -
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F(">Cumulative Gas Volume: ");
    webpage += String(Current_Data_Record.field.gas_volume) + " cubic metres";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 18.Month End Process Status
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    if (Month_End_Process_Status) {
        webpage += F(">Month End Process Active, started: ");
        webpage += Month_End_Process_Started;
    }
    else {
        webpage += F(">Month End Process Inactive");
    }
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 19.Month End CUrrent File being Processed
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    if (Month_End_Process_Status) {
        webpage += F(">Currently processing file: ");
        webpage += ZipFileName;
    }
    else {
        webpage += F(" ");
    }
    webpage += "</span></strong></p>";
    //    webpage += F("<p ");
    //    // Row 20.Month End Records Written Percentage of a month
    //    webpage += F("<p ");
    //    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    //    webpage += F("bold:true; font - size:12px; '");
    //    if (Month_End_Process_Status) {
    //        webpage += F(">Month End Records Written: ");
    //        webpage += String(Month_End_Records_Processed);
    //        webpage += F("Percentage of Full Month : ");
    //        webpage += String(((double)Month_End_Records_Processed * (double)100) / (((double)24 * (double)60 * (double)60 * (double)31) / (double)5)) + "%";
    //    }
    //    else {
    webpage += F(".");
    //    }
    //    webpage += "</span></strong></p>";
    //    webpage += F("<p ");
    //    // 21.Month End Record Written
    //    webpage += F("<p ");
    //    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    //    webpage += F("bold:true; font - size:12px; '");
    //    if (Month_End_Process_Status) {
    //        webpage += F(">Month End Records Written: ");
    //        webpage += String(Month_End_Records_Processed);
    //    }
    //    else {
    //        webpage += F(".");
    //    }
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // -------
    webpage += F("<</h3>");
    DataFile.close();
    Page_Footer();
    lastcall = "display";
}
void i_Download_Files() {
    int file_count = Count_Files_on_SD_Drive();             // this counts and creates an array of file names on SD
    sortArray(FileNames, file_count);         // sort into rising order
    console_print("Download of Files Requested via Webpage");
    webpage.reserve(4000);  // Pre-allocate for file list
    Page_Header(false, "Energy Monitor Download Files");
    if (file_count > 0) {
        for (int i = 0; i < file_count; i++) {
            webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(FileNames[i]) + " ";
            webpage += "&nbsp;<a href=\"/GetFile?file=" + String(FileNames[i]) + " " + "\">Download</a>";
        }
    }
    else {
        webpage += F("<h3 ");
        webpage += F("style='text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:24px;'");
        webpage += F(">No Downloadable Files");
        webpage += F("</span></strong>");
    }
    webpage += F("</h3>");
    Page_Footer();
}
void i_Delete_Previous_Month() {
    console_print("Bulk Delete Previous Month Requested via Webpage");
    webpage.reserve(3000);
    Page_Header(false, "Delete Previous Month's Daily Files");
    
    // Get current month
    Update_Current_Timeinfo();
    int currentMonth = New_Date_Time_Data.field.Month;
    int previousMonth = currentMonth - 1;
    int previousYear = New_Date_Time_Data.field.Year;
    
    if (previousMonth < 1) {
        previousMonth = 12;
        previousYear--;
    }
    
    webpage += F("<h3 style='text-align:left;color:DodgerBlue;font-size:18px';>");
    webpage += "Current Month: " + String(Month_Names[currentMonth]) + "<br>";
    webpage += "Previous Month: " + String(Month_Names[previousMonth]) + " " + String(previousYear) + "<br><br>";
    webpage += F("</h3>");
    
    webpage += F("<h3 style='text-align:center;color:red;font-size:20px';>");
    webpage += F("WARNING: This will delete ALL daily files from the previous month!<br>");
    webpage += F("Make sure the monthly consolidated file has been downloaded first!<br><br>");
    webpage += F("</h3>");
    
    webpage += "<h3 style='text-align:center;'>";
    webpage += "<a href='/ConfirmDeletePreviousMonth?month=" + String(previousMonth) + "' style='background-color:red;color:white;padding:10px 20px;text-decoration:none;border-radius:5px;'>CONFIRM DELETE</a>";
    webpage += "&nbsp;&nbsp;&nbsp;";
    webpage += "<a href='/Display' style='background-color:green;color:white;padding:10px 20px;text-decoration:none;border-radius:5px;'>CANCEL</a>";
    webpage += "</h3>";
    
    Page_Footer();
}
void i_Confirm_Delete_Previous_Month() {
    int monthToDelete = server.arg("month").toInt();
    console_print("Confirmed: Deleting all daily files from month " + String(monthToDelete));
    
    webpage.reserve(3000);
    Page_Header(false, "Deleting Previous Month Files");
    
    webpage += F("<h3 style='text-align:left;color:DodgerBlue;font-size:18px';>");
    webpage += "Deleting daily files from: " + String(Month_Names[monthToDelete]) + "<br><br>";
    webpage += F("</h3>");
    
    int deletedCount = Delete_Previous_Month_Daily_Files(monthToDelete);
    
    webpage += F("<h3 style='text-align:center;color:green;font-size:20px';>");
    webpage += "Successfully deleted " + String(deletedCount) + " daily files<br>";
    webpage += F("</h3>");
    
    webpage += "<h3 style='text-align:center;'>";
    webpage += "<a href='/Display' style='background-color:blue;color:white;padding:10px 20px;text-decoration:none;border-radius:5px;'>Return to Main Page</a>";
    webpage += "</h3>";
    
    Send_WhatsApp_Message("Power Monitor - User deleted " + String(deletedCount) + " daily files from month " + String(monthToDelete));
    Page_Footer();
}
void i_Download_File() {                                                              // download the selected file
    String fileName = server.arg("file");
    console_print("Download of File " + fileName + " Requested via Webpage");
    File DataFile = SD.open("/" + fileName, FILE_READ);                                   // Now read data from FS
    if (DataFile) {                                                                          // if there is a file
        if (DataFile.available()) {                                            // if data is available and present
            String contentType = "application/octet-stream";
            server.sendHeader("Content-Disposition", "attachment; filename=" + fileName);
            if (server.streamFile(DataFile, contentType) != DataFile.size()) {
                Message = "Sent less data (";
                Message += String(server.streamFile(DataFile, contentType));
                Message += ") from ";
                Message += fileName;
                Message += " than expected (";
                Message += String(DataFile.size());
                Message += ")";
                console_print(Message);
            }
        }
    }
    DataFile.close(); // close the file:
    webpage = "";
}
void i_Delete_Files() {                                            // allow the cliet to select a file for deleting
    int file_count = 0;
    file_count = Count_Files_on_SD_Drive();                 // this counts and creates an array of file names on SD
    sortArray(FileNames, file_count);         // sort into rising order
    console_print("Delete Files Requested via Webpage");
    webpage.reserve(4000);  // Pre-allocate for file list
    Page_Header(false, "Energy Monitor Delete Files");
    if (file_count > 0) {
        for (int i = 0; i < file_count; i++) {
            webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(FileNames[i]) + " ";
            webpage += "&nbsp;<a href=\"/DelFile?file=" + String(FileNames[i]) + " " + "\">Delete</a>";
            webpage += "</h3>";
        }
    }
    else {
        webpage += F("<h3 ");
        webpage += F("style='text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:24px;'");
        webpage += F(">No Deletable Files");
        webpage += F("</span></strong></h3>");
    }
    Page_Footer();
}
void i_Del_File() {                                                                // web request to delete a file
    int file_count = 0;
    String fileName = "/20221111.csv";                              // dummy load to get the string space reserved
    fileName = "/" + server.arg("file");
    SD.remove(fileName);
    console_print(fileName + " Removed");
    webpage = "";                               // don't delete this command, it ensures the server works reliably!
    webpage.reserve(4000);  // Pre-allocate for file list
    file_count = Count_Files_on_SD_Drive();                // this counts and creates an array of file names on SD
    Page_Header(false, "Energy Monitor Delete Files");
    if (file_count > 0) {
        for (int i = 0; i < file_count; i++) {
            webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>";
            webpage += String(i) + " " + String(FileNames[i]) + " ";
            webpage += "&nbsp;<a href=\"/DelFile?file=" + String(FileNames[i]) + " " + "\">Delete</a>";
            webpage += "</h3>";
        }
    }
    else {
        webpage += F("<h3 ");
        webpage += F("style='text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:24px;'");
        webpage += F(">No Deletable Files");
        webpage += F("</span></strong></h3>");
    }
    Page_Footer();
}
int Count_Files_on_SD_Drive() {
    int file_count = 0;
    bool files_present = false;
    String filename;
    File root = SD.open("/");                                                         //  Open the root directory
    do {
        File entry = root.openNextFile();                                                   //  get the next file
        if (entry) {
            filename = entry.name();
            files_present = true;
            File DataFile = SD.open("/" + filename, FILE_READ);                          // Now read data from FS
            if (DataFile) {                                                                 // if there is a file
                if ((filename.indexOf(".csv") > 0) || (filename.indexOf(".csm") > 0) || (filename.indexOf(".log") > 0)) {
                    FileNames[file_count] = filename;
                    file_count++;                                                     // increment the file count
                }
            }
            DataFile.close(); // close the file:
        }
        else {
            root.close();
            files_present = false;
        }
    } while (files_present);
    return (file_count);
}

/**
 * @brief Send RS485 request to KWS power monitoring hardware
 * 
 * Transmits a predefined RS485 request byte sequence to the KWS hardware to query
 * sensor data. Sets the RS485 transceiver to transmit mode, sends 8-byte request
 * with delays between bytes, then returns to receive mode for response.
 * 
 * @param field The request index (0 to Number_of_RS485_Requests) to send
 * 
 * @note RS485_DE_pin and RS485_RE_pin control transmit/receive direction
 * @note 2ms delay between bytes for hardware settling time
 * @note Request format defined in RS485_Requests array (KWS protocol)
 * @see Receive(), RS485_Requests[][], RS485_BYTE_DELAY_MS
 */
void Send_Request(int field) {
    digitalWrite(RS485_DE_pin, HIGH);                         // Enable TX (DE active HIGH)
    digitalWrite(RS485_RE_pin, HIGH);                         // Also HIGH during TX
    delayMicroseconds(50);                                    // Small delay for transceiver mode switch
    
    for (int x = 0; x < 7; x++) {
        RS485_Port.write(RS485_Requests[field][x]);                  // write the Request string to the RS485 Port
        delay(2);
    }
    RS485_Port.write(RS485_Requests[field][7]);
    delay(10);                                                 // Wait for transmission to complete
    RS485_Port.flush();                                        // Ensure all bytes transmitted
    digitalWrite(RS485_DE_pin, LOW);                           // Disable TX immediately after flush
    delayMicroseconds(50);                                     // Small delay for transceiver mode switch
    while (RS485_Port.available()) RS485_Port.read();          // Flush any echo from TX buffer
}
/**
 * @brief Receive and parse RS485 response from KWS power monitoring hardware
 * 
 * Receives the RS485 response packet following a Send_Request(). Implements timeout
 * protection (RS485_TIMEOUT_MS) and validates packet structure. Includes yield()
 * in the receive loop to maintain system responsiveness during sensor reads.
 * 
 * Response packet structure (KWS protocol):
 * - Byte 0: Header
 * - Bytes 1-6: Data payload (varies by request type)
 * - Byte 7: Checksum
 * 
 * Parses multi-byte values (16-bit and 32-bit integers) from response and stores
 * in Current_Data_Record structure.
 * 
 * @param field The request index that was sent (determines parsing method)
 * 
 * @return true if valid response received and parsed, false on timeout or error
 * 
 * @note Timeout: RS485_TIMEOUT_MS (500ms)
 * @note Includes yield() for WiFi stack and background tasks
 * @note Updates Current_Data_Record with sensor values
 * @note Failure logged to console and triggers system restart
 * @see Send_Request(), Get_Sensor(), Current_Data_Record
 */
bool Receive(int field) {
    byte pointer = 0;
    byte required_bytes = 9;  // Start with max possible (will be updated when we read byte 3)
    int received_character = 0;
    bool value_status = false;
    unsigned long start_time;                                   // take the start time, used to check for time out
    long value[7] = { 0,0,0,0,0,0,0 };
    digitalWrite(RS485_DE_pin, LOW);                            // Disable TX (DE active HIGH, so LOW=disabled)
    digitalWrite(RS485_RE_pin, HIGH);                           // Enable RX (RE Active HIGH for this module)
    delayMicroseconds(100);                                     // Brief delay for transceiver stabilization
    start_time = millis();                                      // take the start time, used to check for time out
    int bytes_received = 0;  // DEBUG: Count bytes
    byte first_byte = 0;     // DEBUG: Capture first byte
    do {
        timerWrite(Timer0_Cfg, 0);  // Reset watchdog timer
        while (!RS485_Port.available()) {                                          // wait for some data to arrive
            if (millis() > start_time + (unsigned long)500) {         // no data received within 500 ms so timeout
                console_print("RS485 TIMEOUT: field=" + String(field) + " - no response");
                return false;  // Return failure instead of rebooting
            }
            timerWrite(Timer0_Cfg, 0);  // Reset watchdog timer
            yield();  // Allow ESP32 background tasks
        }
        value_status = true;                          // set the value status to true, which is true at this point
        while (RS485_Port.available()) {                                // Data is available so add them to Result
            received_character = RS485_Port.read();                                          // take the character
            bytes_received++;  // DEBUG
            if (bytes_received == 1) first_byte = received_character;  // DEBUG: Save first byte
            switch (pointer) {                            // check the received character is correct at this point
            case 0: {
                if (received_character != 2) {
                    value_status = false;                      // first byte should be a 2, used as sync character
                }
                value[0] = received_character;
                break;
            }
            case 1: {
                if (received_character != 3) {
                    value_status = false;                                             // second byte should be a 3
                }
                value[1] = received_character;
                break;
            }
            case 2: {
                if (received_character == 2) {// third byte should be 2 or 4, which indicates the length of values
                    required_bytes = 7;                                               // total of 7 bytes required       
                }
                else if (received_character == 4) {
                    required_bytes = 9;                                               // total of 9 bytes required
                }
                else {
                    value_status = false;                                          // if not a 2 or 4 value is bad
                }
                value[2] = received_character;
                break;
            }
            case 3: {
                value[3] = received_character;               // received characters 3 to end are part of the value
                break;
            }
            case 4: {
                value[4] = received_character;
                break;
            }
            case 5: {
                value[5] = received_character;
                break;
            }
            case 6: {
                value[6] = received_character;
                break;
            }
            default: {                                                              // throw other characters away
            }
            }
            if (value_status == true) pointer++;                                        // increment the inpointer
        }
    } while (pointer < required_bytes);   // loop until all the required characters have been received, or timeout
    
    if (value_status == true) {                                                          // received value is good
        switch (field) {
        case Request_Voltage: {                                                                    // [0]  Voltage
            double Voltage = (value[3] << 8) + value[4];                   // Received is number of tenths of volt
            Voltage = Voltage / (double)10;                                                    // convert to volts
            Current_Data_Record.field.Voltage = Voltage;                           // Voltage output format double
            break;
        }
        case Request_Amperage: {                                                                  // [1]  Amperage
            double Amperage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);    // milli amps
            Amperage /= (double)1000;                                                 // convert milliamps to amps
            Current_Data_Record.field.Amperage = Amperage;
            break;
        }
        case Request_Wattage: {                                                                    // [2]  Wattage
            double wattage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);// tenths of watts
            Current_Data_Record.field.wattage = wattage;
            break;
        }
        case Request_UpTime: {                                                                      //  [3] Uptime
            double uptime = (double)(value[3] << 8) + (double)value[4];
            uptime = uptime / (double)60;                                                      // convert to hours
            Current_Data_Record.field.uptime = uptime;
            break;
        }
        case Request_Kilowatthour: {                                                          //  [4] KilowattHour
            double kilowatthour = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);
            kilowatthour = kilowatthour / (double)1000;
            Current_Data_Record.field.kilowatthour = kilowatthour;
            break;
        }
        case Request_Power_Factor: {                                                          //  [5] Power Factor
            double powerfactor = (double)(value[3] << 8) + (double)value[4];
            powerfactor = powerfactor / (double)100;
            Current_Data_Record.field.powerfactor = powerfactor;
            break;
        }
        case Request_Frequency: {                                                                //  [7] Frequency
            double frequency = (double)(value[3] * (double)256) + (double)value[4];
            frequency = frequency / (double)10;
            Current_Data_Record.field.frequency = frequency;
            break;
        }
        case Request_Sensor_Temperature: {                                               // [8] Sensor Temperature
            double temperature = (double)(value[3] << 8) + (double)value[4];
            Current_Data_Record.field.sensor_temperature = temperature;
            break;
        }
        default: {
            console.print(millis(), DEC); console.println("\tRequested Fields != Received Fields");
            break;
        }
        }
        Update_Current_Timeinfo();
        for (int x = 0; x < 10; x++) {
            Current_Data_Record.field.ldate[x] = Current_Date_With[x];
        }
        for (int x = 0; x < 8; x++) {
            Current_Data_Record.field.ltime[x] = Current_Time_With[x];
        }
        return true;
    }
    console_print("RS485 FAIL: field=" + String(field) + " bytes=" + String(bytes_received) + " 1st=0x" + String(first_byte, HEX) + " status=" + String(value_status));
    return false;
}
void Check_Blue_Switch(bool Month_End_Auto) {
    if (!Month_End_Auto) {
        Blue_Switch.update();
        if (Blue_Switch.fell()) {
            console_print("Blue Button Pressed, executing Month End Procedure");
            Blue_Switch_Pressed = true;
            Update_Current_Timeinfo();
            console_print("Today's Actual Month = " + String(New_Date_Time_Data.field.Month));
            Month_End_Date_Time_Data.field.Month = New_Date_Time_Data.field.Month - 1;     // so set yesterday month
            if (Month_End_Date_Time_Data.field.Month < 1) {                                  // cope with previous Today Month = 1 (January)
                Month_End_Date_Time_Data.field.Month = 12;
                Month_End_Date_Time_Data.field.Year = New_Date_Time_Data.field.Year - 1;
            }
            else {
                Month_End_Date_Time_Data.field.Year = New_Date_Time_Data.field.Year;
            }
            console_print("Month End Month set to " + String(Month_End_Date_Time_Data.field.Month));
            console_print("Month End Year set to " + String(Month_End_Date_Time_Data.field.Year));
            Month_End_Process();
            Update_Current_Timeinfo();                                                  // reset the date
        }
        if (Blue_Switch.rose()) {
            console_print("Blue Button Released");
            Blue_Switch_Pressed = false;
        }
    }
    else {
        console_print("Month End Process (previous month) Requested");
        Update_Current_Timeinfo();
        console_print("Today's Actual Month = " + String(New_Date_Time_Data.field.Month));
        Month_End_Date_Time_Data.field.Month = New_Date_Time_Data.field.Month - 1;     // so set yesterday month
        if (Month_End_Date_Time_Data.field.Month < 1) {                                  // cope with previous Today Month = 1 (January)
            Month_End_Date_Time_Data.field.Month = 12;
            Month_End_Date_Time_Data.field.Year = New_Date_Time_Data.field.Year - 1;
        }
        else {
            Month_End_Date_Time_Data.field.Year = New_Date_Time_Data.field.Year;
        }
        console_print("Month End Month set to " + String(Month_End_Date_Time_Data.field.Month));
        console_print("Month End Year set to " + String(Month_End_Date_Time_Data.field.Year));
        Month_End_Process();
        Update_Current_Timeinfo();                                                  // reset the date
    }
}
void Check_Gas_Switch() {
    if (!config.gas_meter_enabled) {
        return;  // Gas meter monitoring disabled - skip to prevent false triggers from floating input
    }
    
    Gas_Switch.update();                                                                // read the gas switch
    if (Gas_Switch.fell()) {
        console_print("Gas Switch Fell");
        Current_Data_Record.field.gas_volume += Gas_Volume_Per_Sensor_Rise; // +gas when gas switch on rising edge
        Latest_Gas_Volume = Current_Data_Record.field.gas_volume;
        Latest_Gas_Date_With = Current_Date_With;
        Latest_Gas_Time_With = Current_Time_With;
        toggleLED(Green_led_pin); // flash the green light if Gas Switch changed
        
        // Save to EEPROM if changed by threshold amount (0.1m³)
        if (abs(Current_Data_Record.field.gas_volume - Last_Saved_Gas_Volume) >= GAS_VOLUME_SAVE_THRESHOLD) {
            Save_Gas_Volume_To_EEPROM(Current_Data_Record.field.gas_volume);
        }
    }
    if (Gas_Switch.rose()) {
        console_print("Gas Switch Rose");
    }
}
void Check_Red_Switch(String message) {
    // This function is deprecated but kept for compatibility
    // Now automatically reboots after error notification
    console_print("Fatal Error: " + message + " - Automatic reboot in 5 seconds");
    Send_WhatsApp_Message("Power Monitor - Fatal Error Detected - " + message + " - Automatic Reboot");
    smartDelay(5000);  // Allow time for message to be sent (keeps system responsive)
    ESP.restart();
}
void Check_Yellow_Switch() {
    Yellow_Switch.update();
    if (Yellow_Switch.fell()) {
        console_print("Yellow Button Pressed");
        Yellow_Switch_Pressed = true;
    }
    if (Yellow_Switch.rose()) {
        console_print("Yellow Button Released");
        Yellow_Switch_Pressed = false;
    }
}
void Clear_Arrays() {                                                       // clear the web arrays of old records
    for (int x = 0; x < Data_Table_Size; x++) {
        for (int y = 0; y < Data_Table_Record_Length; y++)
            Readings_Table[x].characters[y] = '0';
    }
    Data_Table_Pointer = 0;
    for (int x = 0; x < Data_Record_Length; x++) {
        Current_Data_Record.character[x] = 0;
    }
}
void SetTimeZone(String timezone) {
    console_print("Setting Timezone to " + String(timezone));
    setenv("TZ", timezone.c_str(), 1);                                    // ADJUST CLOCK SETiNGS TO LOCAL TiME
    tzset();
}
void Update_Current_Timeinfo() {
    static int connection_attempts = 0;
    static unsigned long lastAttemptTime = 0;
    
    // Non-blocking retry with 500ms interval (Issue #19 - Blocking Delays)
    if (!getLocalTime(&timeinfo)) {
        if (millis() - lastAttemptTime >= 500) {
            console_print("Attempting to Get Date " + String(connection_attempts));
            lastAttemptTime = millis();
            connection_attempts++;
            if (connection_attempts > 20) {
                console_print("Time Network Error, Restarting");
                Send_WhatsApp_Message("Power Monitor - Time Network Error, Restarting");
                ESP.restart();
            }
        }
        return;  // Exit early, try again next call
    }
    
    // Success - reset counter for next time
    connection_attempts = 0;
    
    New_Date_Time_Data.field.Year = timeinfo.tm_year + 1900;
    New_Date_Time_Data.field.Month = timeinfo.tm_mon + 1;
    New_Date_Time_Data.field.Day = timeinfo.tm_mday;
    New_Date_Time_Data.field.Hour = timeinfo.tm_hour;
    New_Date_Time_Data.field.Minute = timeinfo.tm_min;
    New_Date_Time_Data.field.Second = timeinfo.tm_sec;
    
    // Validate received time data (Issue #23 - Date/Time Validation)
    bool time_valid = true;
    String validation_error = "";
    
    if (New_Date_Time_Data.field.Year < 2025 || New_Date_Time_Data.field.Year > 2100) {
        validation_error = "Invalid year: " + String(New_Date_Time_Data.field.Year);
        time_valid = false;
    }
    if (New_Date_Time_Data.field.Month < 1 || New_Date_Time_Data.field.Month > 12) {
        validation_error = "Invalid month: " + String(New_Date_Time_Data.field.Month);
        time_valid = false;
    }
    if (New_Date_Time_Data.field.Day < 1 || New_Date_Time_Data.field.Day > 31) {
        validation_error = "Invalid day: " + String(New_Date_Time_Data.field.Day);
        time_valid = false;
    }
    if (New_Date_Time_Data.field.Hour > 23) {
        validation_error = "Invalid hour: " + String(New_Date_Time_Data.field.Hour);
        time_valid = false;
    }
    if (New_Date_Time_Data.field.Minute > 59) {
        validation_error = "Invalid minute: " + String(New_Date_Time_Data.field.Minute);
        time_valid = false;
    }
    if (New_Date_Time_Data.field.Second > 59) {
        validation_error = "Invalid second: " + String(New_Date_Time_Data.field.Second);
        time_valid = false;
    }
    
    if (!time_valid) {
        DEBUG_ERROR("NTP time validation failed: " + validation_error);
        Send_WhatsApp_Message("Power Monitor - Invalid NTP time received: " + validation_error + " - Restarting");
        smartDelay(2000);
        ESP.restart();
    }
    
    // DATE --
    Current_Date_With = String(New_Date_Time_Data.field.Year);        //  1951
    Current_Date_Without = String(New_Date_Time_Data.field.Year);        //  1951
    Current_Date_With += "/";                                         //  1951/
    if (New_Date_Time_Data.field.Month < 10) {
        Current_Date_With += "0";                                         //  1951/0
        Current_Date_Without += "0";
    }
    Current_Date_With += String(New_Date_Time_Data.field.Month);      //  1951/11
    Current_Date_Without += String(New_Date_Time_Data.field.Month);
    Current_Date_With += "/";                                         //  1951/11/

    if (New_Date_Time_Data.field.Day < 10) {
        Current_Date_With += "0";                                         //  1951/11/0
        Current_Date_Without += "0";
    }
    Current_Date_With += String(New_Date_Time_Data.field.Day);        //  1951/11/18
    Current_Date_Without += String(New_Date_Time_Data.field.Day);        //  1951/11/18
    // TiME --
    Current_Time_With = "";
    Current_Time_Without = "";
    if (New_Date_Time_Data.field.Hour < 10) {                           // if hours are less than 10 add a 0
        Current_Time_With = "0";
        Current_Time_Without = "0";
    }
    Current_Time_With += String(New_Date_Time_Data.field.Hour);       //  add hours
    Current_Time_Without += String(New_Date_Time_Data.field.Hour);       //  add hours
    Current_Time_With += ":";                                         //  23:
    if (New_Date_Time_Data.field.Minute < 10) {                         //  if minutes are less than 10 add a 0
        Current_Time_With += "0";                                         //  23:0
        Current_Time_Without += "0";
    }
    Current_Time_With += String(New_Date_Time_Data.field.Minute);     //  23:59
    Current_Time_Without += String(New_Date_Time_Data.field.Minute);     //  23:59
    Current_Time_With += ":";                                         //  23:59:
    if (New_Date_Time_Data.field.Second < 10) {
        Current_Time_With += "0";
        Current_Time_Without += "0";
    }
    Current_Time_With += String(New_Date_Time_Data.field.Second);     //  23:59:59
    Current_Time_Without += String(New_Date_Time_Data.field.Second);     //  23:59:59
    Standard_Format_Date_Time = "";
    if (New_Date_Time_Data.field.Day < 10) {
        Standard_Format_Date_Time += "0";                                         //  1951/11/0
    }
    Standard_Format_Date_Time += String(New_Date_Time_Data.field.Day) + "/";    //  18/
    if (New_Date_Time_Data.field.Month < 10) {
        Standard_Format_Date_Time += "0";                                         //  1951/0
    }
    Standard_Format_Date_Time += String(New_Date_Time_Data.field.Month) + "/";      //  1951/11
    Standard_Format_Date_Time += String(New_Date_Time_Data.field.Year);        //  1951
    Standard_Format_Date_Time += " ";
    if (New_Date_Time_Data.field.Hour < 10) {
        Standard_Format_Date_Time += "0";                                         //  1951/11/0
    }
    Standard_Format_Date_Time += String(New_Date_Time_Data.field.Hour);
    Standard_Format_Date_Time += ":";
    if (New_Date_Time_Data.field.Minute < 10) {
        Standard_Format_Date_Time += "0";                                         //  1951/11/0
    }
    Standard_Format_Date_Time += String(New_Date_Time_Data.field.Minute);
    Standard_Format_Date_Time += ":";
    if (New_Date_Time_Data.field.Second < 10) {
        Standard_Format_Date_Time += "0";                                         //  1951/11/0
    }
    Standard_Format_Date_Time += String(New_Date_Time_Data.field.Second);
    if (Last_Time_Update != timeinfo.tm_min) {
        Last_Time_Update = timeinfo.tm_min;
        console_print("Current Date and Time set to: " + Current_Date_With + " " + Current_Time_With);
    }
}

/**
 * @brief Parse JSON weather data from OpenWeatherMap API
 * 
 * Extracts temperature, humidity, pressure, and weather description from the
 * JSON response payload. Uses simple string parsing instead of JSON library to
 * reduce memory overhead. Looks for specific field names ("temp", "humidity",
 * etc.) and extracts the following numeric or string values.
 * 
 * JSON structure expected:
 * - {"main":{"temp":284.12,"humidity":72,"pressure":1018}}
 * - {"weather":[{"description":"overcast clouds"}]}
 * 
 * @param payload The JSON response string from OpenWeatherMap API
 * 
 * @return true if all required fields parsed successfully, false on parse error
 * 
 * @note Updates global variables: Weather_Temperature, Weather_Humidity, etc.
 * @note Temperature is in Kelvin from API, converted to Celsius
 * @note Parse errors return false but don't crash system
 * @see Get_Weather(), Parse()
 */
bool Parse_Weather_info(String payload) {
    /*
    {"coord":{"lon":-1.0871,"lat":51.2625},
    "weather":[{"id":804,"main":"Clouds",
    "description":"overcast clouds",
    "icon":"04n"}],"base":"stations","main":{
    "temp":284.12,
    "feels_like":283.47,
    "temp_min":282.75,
    "temp_max":284.89,
    "pressure":1018,
    "humidity":84},
    "visibility":10000,"wind":{
    "speed":8.23,
    "deg":260},
    "clouds":{"all":100},"dt":1672864004,"sys":{"type":2,"id":2016598,
    "country":"GB","sunrise":1672819699,"sunset":1672848573},
    "timezone":0,"id":2656192,"name":"Basingstoke","cod":200}
    */

    // Validate payload contains expected fields
    if (payload.indexOf("temp") < 0 || payload.indexOf("humidity") < 0) {
        console_print("Weather Parse - Invalid JSON payload (missing required fields)");
        return false;
    }

    int start = 0;
    int end = 0;
    // Temperature ----------
    start = payload.indexOf("temp\":");             // "temp":272.77,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    Parse(payload, start, end);
    Current_Data_Record.field.weather_temperature = (double)(atof(Parse_Output)) - (double)273.15;
    // Temperature Feels Like --------------
    start = payload.indexOf("feels_like");          // "feels_like":283.47,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    Parse(payload, start, end);
    Current_Data_Record.field.temperature_feels_like = (double)(atof(Parse_Output)) - (double)273.15;
    // Temperature Maximum --
    start = payload.indexOf("temp_max");            // "temp_max":284.89,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    Parse(payload, start, end);
    Current_Data_Record.field.temperature_maximum = (double)(atof(Parse_Output)) - (double)273.15;
    // Temperature Minimum --
    start = payload.indexOf("temp_min");            // "temp_min":282.75,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    Parse(payload, start, end);
    Current_Data_Record.field.temperature_minimum = (double)(atof(Parse_Output)) - (double)273.15;
    // Pressure -------------
    start = payload.indexOf("pressure");            // "pressure":1018,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    Parse(payload, start, end);
    Current_Data_Record.field.atmospheric_pressure = (double)atof(Parse_Output);
    // humidity -------------
    start = payload.indexOf("humidity\":");         // "humidity":95}
    start = payload.indexOf(":", start);
    end = payload.indexOf("}", start);
    Parse(payload, start, end);
    Current_Data_Record.field.relative_humidity = (double)atof(Parse_Output);
    // weather description --
    start = payload.indexOf("description");         // "description":"overcast clouds",
    start = (payload.indexOf(":", start) + 1);
    end = (payload.indexOf(",", start) - 1);
    Parse(payload, start, end);
    for (int x = 0; x < sizeof(Current_Data_Record.field.weather_description); x++) {
        Current_Data_Record.field.weather_description[x] = Parse_Output[x];
    }
    // wind speed -----------
    start = payload.indexOf("speed");                   // "speed":2.57,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    Parse(payload, start, end);
    Current_Data_Record.field.wind_speed = (double)(atof(Parse_Output));
    // wind direction -------
    start = payload.indexOf("deg");                     // "deg":20
    start = payload.indexOf(":", start);
    end = payload.indexOf("}", start);
    Parse(payload, start, end);
    Current_Data_Record.field.wind_direction = (double)atof(Parse_Output);
    return true;
}
void Parse(String payload, int start, int end) {
    int ptr = 0;
    for (int pos = start + 1; pos < end; pos++) {
        Parse_Output[ptr++] = payload[pos];
    }
    Parse_Output[ptr] = '\0';
}
