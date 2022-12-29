/*
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
16/12/2022  9.5 Add more statistics to statistics page
17/12/2022  9.6 Corrected issue with Delete Files, where operational files were displayed after file removed
20/12/2022  9.7 Corrected an issue where if the date changed new file creation would be continuously repeated
22/12/2022  9.8 Web data arrays cleared when date changed, SD flashes during Preloading
23/12/2022  9.9 Requirement to press start button removed
28/12/2022  9.10 Attempt to recover network if lost (up to 20 attempts = 10 seconds
*/
String version = "V9.10";                // software version number, shown on webpage
// compiler directives ------------------------------------------------------------------------------------------------
//#define ALLOW_WORKING_FILE_DELETION         // allows the user to chose to delete the day's working files
//#define DISPLAY_WEATHER_INFORMATION         // print the raw and parsed weather information
// definitions --------------------------------------------------------------------------------------------------------
#define console Serial
#define RS485_Port Serial2
// includes -----------------------------------------------------------------------------------------------------------
#include <vfs_api.h>
#include <FSImpl.h>
#include <sd_diskio.h>
#include <sd_defines.h>
#include <SD.h>
#include <FS.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include "time.h"
#include <Bounce2.h>
#include <Uri.h>
#include <HTTP_Method.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <stdio.h>
const char* ssid = "Woodleigh";
const char* password = "2008198399";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;                             // offset for the date and time function
const char city[] = { "Basingstoke\0" };
const char region[] = { "uk\0" };

String WiFi_Status_Message[7] = {
                    "WL_IDLE_STATUS",       // temporary status assigned when WiFi.begin() is called
                    "WL_NO_SSID_AVAIL",     // when no SSID are available
                    "WL_SCAN_COMPLETED",    // scan networks is completed
                    "WL_CONNECTED",         // when connected to a WiFi network
                    "WL_CONNECT_FAILED",    // when the connection fails for all the attempts
                    "WL_CONNECTION_LOST",   // when the connection is lost
                    "WL_DISCONNECTED",      // when disconnected from a network
};
const char  incomplete_weather_api_link[] = { "http://api.openweathermap.org/data/2.5/weather?q=#&APPID=917ddeff21dff2cfc5e57717f809d6ad\0" };
constexpr long console_Baudrate = 115200;
constexpr long RS485_Baudrate = 9600;                           // baud rate of RS485 Port
// ESP32 Pin Definitions ----------------------------------------------------------------------------------------------
constexpr int Blue_Switch_pin = 32;
constexpr int Red_Switch_pin = 33;
constexpr int Green_Switch_pin = 27;
constexpr int Running_led_pin = 26;
constexpr int Yellow_Switch_pin = 0;
constexpr int SD_Active_led_pin = 4;
constexpr int RS485_Enable_pin = 22;
constexpr int RS485_TX_pin = 17;
constexpr int RS485_RX_pin = 16;
constexpr int SS_pin = 5;
constexpr int SCK_pin = 18;
constexpr int MISO_pin = 19;
constexpr int MOSI_pin = 23;
constexpr int ONBOARDLED = 2;
struct tm timeinfo;                                             // spave for date and time fields
String This_Year = "";
String This_Month = "";
String This_Day = "";
String This_Hour = "";
String This_Minute = "";
String This_Second = "";
String This_Date = "";
// Instantiations -----------------------------------------------------------------------------------------------------
Bounce Red_Switch = Bounce();
Bounce Green_Switch = Bounce();
Bounce Blue_Switch = Bounce();
Bounce Yellow_Switch = Bounce();
WebServer server(80);                   // WebServer(HTTP port, 80 is defAult)
WiFiClient client;
HTTPClient http;
File Datafile;                         // Full data file, holds all readings from KWS-AC301L
File Consolefile;
// --------------------------------------------------------------------------------------------------------------------
constexpr int Voltage = 0;
constexpr int Amperage = 1;
constexpr int Wattage = 2;
constexpr int UpTime = 3;
constexpr int Kilowatthour = 4;
constexpr int PowerFactor = 5;
constexpr int Unknown = 6;
constexpr int Frequency = 7;
constexpr int Temperature = 8;
constexpr bool receive = 0;
constexpr bool transmit = 1;
/*
Requests                Hex                                 Decimal                                     Response Decimal No Wire in Sensor
 Voltage                02,03,00,0E,00,01,E5,FA             002,003,000,014,000,001,229,250             002,003,002,009,202,122,067
 Amperage               02,03,00,0F,00,02,F4,3B             002,003,000,015,000,002,244,059             002,003,004,000,000,000,000,201,051
 Watta                  02,03,00,11,00,02,94,3D             002,003,000,017,000,002,148,061             002,003,004,000,000,000,000,201,051
 minutes                02,03,00,19,00,01,55,FE             002,003,000,025,000,001,085,254             002,003,002,000,000,252,068
 kWh                    02,03,00,17,00,02,74,3C             002,003,000,023,000,002,116,060             002,003,004,000,000,000,000,201,051
 ?                      02,03,00,1F,00,01,B5,FF             002,003,000,031,000,101,181,255             002,003,002,146,192,145,116
 Freq                   02,03,00,1E,00,01,E4,3F             002,003,000,030,000,001,228,063             002,003,002,001,243,189,145         (49.8Hz)
 Temp                   02,03,00,1A,00,01,A5,FE             002,003,000,026,000,001,165,254             002,003,002,000,023,188,074         (23 degrees C)

 */
byte RS485_Requests[9][8] = {
                        {0x02,0x03,0x00,0x0E,0x00,0x01,0xE5,0xFA},			// Request Voltage, in tenths of a volt, divided by 10 for volts
                        {0x02,0x03,0x00,0x0F,0x00,0x02,0xF4,0x3B},			// Request Amperage, in milli-amps, divided by 1000 for amps
                        {0x02,0x03,0x00,0x11,0x00,0x02,0x94,0x3D},			// Request Watts, in tenths of a watt, divided by 10 for watts
                        {0x02,0x03,0x00,0x19,0x00,0x01,0x55,0xFE},			// Request uptime, in minutes
                        {0x02,0x03,0x00,0x17,0x00,0x02,0x74,0x3C},          // Request kilo_watt_hour, in milli-watts, divided by 1000 for kWh
                        {0x02,0x03,0x00,0x1D,0x00,0x01,0x14,0x3F},          // Request power factor, in one hundredths, divided by 100 for units
                        {0x02,0x03,0x00,0x1F,0x00,0x01,0xB5,0xFF},          // ?
                        {0x02,0x03,0x00,0x1E,0x00,0x01,0xE4,0x3F},          // Request Hertz, in tenths of a hertz, divided by 10 for Hz
                        {0x02,0x03,0x00,0x1A,0x00,0x01,0xA5,0xFE}           // Request temperature, in degrees centigrade
};
char print_buffer[80];
String DataFileName = "20220101";
String ConsoleFileName = "20220101";
String RS485_FieldNames[15] = {
                        "Volts",                    // [0]
                        "Amps",                     // [1]
                        "Watts",                    // [2]
                        "Up Time",                  // [3]
                        "kWh",                      // [4]
                        "Power Factor",             // [5]
                        "Unknown",                  // [6]
                        "Hz",                       // [7]
                        "degC",                     // [8]
                        "Temperature",              // [9]
                        "Pressure",                 // [10]
                        "Humidity",                 // [11]
                        "Wind Direction",           // [12]
                        "Wind Speed",               // [13]
                        "Weather"                   // [14]
};
String FileNames[50];
String Data_Date;
String Data_Time;
double Data_Values[9] = {
                        0.0,            // volts
                        0.0,            // amps
                        0.0,            // watts
                        0.0,            // up time
                        0.0,            // kWh
                        0.0,            // power factor
                        0.0,            // ?
                        0.0,            // Hz
                        0.0,            // degC
};
bool Yellow_Switch_Pressed = false;
String site_width = "1060"; // "1060";                             // width of web page
String site_height = "600";                             // height of web page
int const table_size = 72;
constexpr int console_table_size = 20;                  // number of lines to display on debug web page
int       record_count, current_data_record_count, console_record_count, current_console_record_count;
String    webpage, lastcall;
double temperature_calibration = (double)16.5 / (double)22.0;   // temperature reading = 22, actual temperature = 16.5
String Last_Boot_Time = "12:12:12";
String Last_Boot_Date = "2022/29/12";
typedef struct {
    char ldate[11];     // date record was taken
    char ltime[9];      // time record was taken
    double voltage;
    double amperage;
    double wattage;
    double uptime;
    double kilowatthour;
    double powerfactor;
    double unknown;                                 // always the same value
    double frequency;
    double temperature;
} record_type;
record_type readings_table[table_size + 1];
// Lowest Voltage -----------------------------------------------------------------------------------------------------
double lowest_voltage = 0;
String time_of_lowest_voltage = "00:00:00";
// Highest Voltage ----------------------------------------------------------------------------------------------------
double highest_voltage = 0;
String time_of_highest_voltage = "00:00:00";
// Highest amperage -----------------------------------------------------------------------------------------------------
double highest_amperage = 0;
String time_of_highest_amperage = "00:00:00";
// Highest frequency --------------------------------------------------------------------------------------------------
typedef struct {
    char ldate[11];         // date the message was taken
    char ltime[9];          // the time the message was taken
    unsigned long milliseconds;  // the millis() value of the message    
    char message[120];
} console_record_type;
console_record_type console_table[console_table_size + 1];
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
weather_record_type weather_record;
double lowest_temperature = 0;
String time_of_lowest_temperature = "00:00:00";
double highest_temperature = 0;
String time_of_highest_temperature = "00:00:00";
double latest_temperature = 0;
String time_of_latest_temperature = "00:00:00";
int latest_pressure = 0;
String time_of_latest_pressure = "00:00:00";
int latest_humidity = 0;
String time_of_latest_humidity = "00:00:00";
String latest_weather = "                                     ";
String time_of_latest_weather = "00:00:00";
double largest_amperage = 0;
String time_of_largest_amperage = "00:00:00";
double latest_wind_speed = 0;
String time_of_latest_wind_speed = "00:00:00";
int latest_wind_direction = 0;
String time_of_latest_wind_direction = "00:00:00";
unsigned long last_cycle = 0;
unsigned long last_weather_read = 0;
uint64_t SD_freespace = 0;
uint64_t critical_SD_freespace = 0;
double SD_freespace_double = 0;
String temp_message;
String console_message = "                                                                    ";
bool Post_Setup_Status = false;
char complete_weather_api_link[120];
int i = 0;
char Parse_Output[25];
String pre_loop_messages[100];
unsigned long pre_loop_millis_values[100];
int pre_loop_message_count = 0;
bool New_Day_File_Required = true;
unsigned long sd_off_time = 0;
unsigned long sd_on_time = 0;
int WiFi_Signal_Strength = 0;
// setup --------------------------------------------------------------------------------------------------------------
void setup() {
    console.begin(console_Baudrate);                                            // enable the console
    while (!console);                                                           // wait for port to settle
    delay(4000);
    console_message = "Booting - Commencing Setup";
    Write_Console_Message();
    pinMode(SD_Active_led_pin, OUTPUT);
    pinMode(Green_Switch_pin, INPUT_PULLUP);
    pinMode(Red_Switch_pin, INPUT_PULLUP);
    pinMode(Blue_Switch_pin, INPUT_PULLUP);
    pinMode(Yellow_Switch_pin, INPUT_PULLUP);
    pinMode(Running_led_pin, OUTPUT);
    Red_Switch.attach(Red_Switch_pin);     // setup defaults for debouncing switches
    Green_Switch.attach(Green_Switch_pin);
    Blue_Switch.attach(Blue_Switch_pin);
    Yellow_Switch.attach(Yellow_Switch_pin);
    Red_Switch.interval(5);                  // sets debounce time
    Green_Switch.interval(5);
    Blue_Switch.interval(5);
    Red_Switch.update();
    Green_Switch.update();
    Blue_Switch.update();
    digitalWrite(Running_led_pin, LOW);
    digitalWrite(SD_Active_led_pin, LOW);
    // WiFi and Web Setup -------------------------------------------------------------------------
    StartWiFi(ssid, password);                      // Start WiFi
    StartTime();                                    // Start Time
    console_message = "Starting Server";
    Write_Console_Message();
    server.begin();                                 // Start Webserver
    console_message = "Server Started";
    Write_Console_Message();
    server.on("/", Display);                        // nothing specified so display main web page
    server.on("/Display", Display);                 // display the main web page
    server.on("/Statistics", Statistics);           // display statistics
    server.on("/DownloadFiles", Download_Files);    // select a file to download
    server.on("/GetFile", Download_File);           // download the selectedfile
    server.on("/DeleteFiles", Delete_Files);        // select a file to delete
    server.on("/DelFile", Del_File);                // delete the selected file
    server.on("/Reset", Web_Reset);                 // reset the orocessor from the webpage
    server.on("/ConsoleShow", Console_Show);        // display the last 30 console messages on a webpage
    RS485_Port.begin(RS485_Baudrate, SERIAL_8N1, RS485_RX_pin, RS485_TX_pin);
    pinMode(RS485_Enable_pin, OUTPUT);
    delay(10);
    if (!SD.begin(SS_pin)) {
        console_message = "SD Drive Begin Failed @ line 232";
        Write_Console_Message();
        while (true) {
            digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
            digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
            delay(500);
            Check_Red_Switch();
        }
    }
    else {
        console_message = "SD Drive Begin Succeeded";
        Write_Console_Message();
        uint8_t cardType = SD.cardType();
        while (SD.cardType() == CARD_NONE) {
            console_message = "No SD Card Found @ line 246";
            Write_Console_Message();
            while (true) {
                digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                delay(500);
                Check_Red_Switch();
            }
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
        console_message = "SD Card Type: " + card;
        Write_Console_Message();
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        critical_SD_freespace = cardSize * (uint64_t).9;
        console_message = "SD Card Size : " + String(cardSize) + "MBytes";
        Write_Console_Message();
        console_message = "SD Total Bytes : " + String(SD.totalBytes());
        Write_Console_Message();
        console_message = "SD Used bytes : " + String(SD.usedBytes());
        Write_Console_Message();
        console_message = "SD Card Initialisation Complete";
        Write_Console_Message();
        console_message = "Create Console Logging File";
        Write_Console_Message();
    }
    Last_Boot_Time = GetTime(true);
    Last_Boot_Date = GetDate(true);
    This_Date = GetDate(false);
    Create_New_Data_File();
    Create_New_Console_File();
    console_message = "End of Setup";
    Write_Console_Message();
    console_message = "Running in Full Function Mode";
    Write_Console_Message();
    int count = 0;
    for (int x = 0; x <= 120; x++) {
        if (incomplete_weather_api_link[x] == '\0') break;
        if (incomplete_weather_api_link[x] != '#') {
            complete_weather_api_link[count] = incomplete_weather_api_link[x];
            count++;
        }
        else {
            for (int y = 0; y < strlen(city); y++) {
                complete_weather_api_link[count++] = city[y];
            }
            complete_weather_api_link[count++] = ',';
            for (int y = 0; y < strlen(region); y++) {
                complete_weather_api_link[count++] = region[y];
            }
        }
    }
    digitalWrite(SD_Active_led_pin, LOW);
    Post_Setup_Status = true;
}   // end of Setup
void loop() {
    Check_WiFi();
    Check_Red_Switch();                                     // check if reset switch has been pressed
    Check_Green_Switch();                                   // check if start switch has been pressed
    Check_Blue_Switch();                                    // check if wipesd switch has been pressed
    Drive_Running_Led();                                    // on when started, flashing when not, flashing with SD led if waiting for reset
    server.handleClient();                                  // handle any messages from the website
    // if (started) {                                       // user has started the readings
        // Energy Information - every 5 seconds -------------------------------------------------------------------------------
    if (millis() > last_cycle + (unsigned long)5000) {    // send requests every 5 seconds (5000 millisecods)
        last_cycle = millis();                            // update the last read milli second reading
        // weather start ------------------------------------------------------------------------------------------
        HTTPClient http;
        http.begin(complete_weather_api_link);                           // start the weather connectio
        int httpCode = http.GET();                              // send the request
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                Parse_Weather_Info(payload);
            }
            else {
                console_message = "Obtaining Weather Information Failed, Return code: " + String(httpCode);
                Write_Console_Message();
                weather_record.temp = 0;
                weather_record.temp_min = 0;
                weather_record.temp_max = 0;
                weather_record.temp_feel = 0;
                weather_record.pressure = 0;
                weather_record.humidity = 0;
                weather_record.wind_direction = 0;
                weather_record.wind_speed = 0;
                weather_record.weather = "";
            }
            http.end();
        }
        // weather end --------------------------------------------------------------------------------------------
        // sensor start -------------------------------------------------------------------------------------------
        for (int i = 0; i < 9; i++) {                           // transmit the requests, assembling the Values array
            Send_Request(i);                                    // send the RS485 Port the requests, one by one
            Data_Values[i] = Receive(i);                        // get the reply
        }                                                       // all values should now be populated
        // sensor end ---------------------------------------------------------------------------------------------
        Data_Date = GetDate(true);                              // get the date of the reading
        Data_Time = GetTime(true);                              // get the time of the reading
        if (This_Date != GetDate(false) && New_Day_File_Required == true) {
            Create_New_Data_File();                             // so create a new Data File with new file name
            Create_New_Console_File();
            Clear_Arrays();
            New_Day_File_Required = false;
        }
        else {
            New_Day_File_Required = true;                       // reset the flag
        }
        Write_New_Data_Record_to_Data_File();                   // write the new record to SD Drive
        Add_New_Data_Record_to_Display_Table();                 // add the record to the display table
    }                                                           // end of if millis >5000
//}                                                               // end of if started
}                                                                   // end of loop
void Write_New_Data_Record_to_Data_File() {
    digitalWrite(SD_Active_led_pin, HIGH);                          // turn the SD activity LED on
    Datafile = SD.open("/" + DataFileName, FILE_APPEND);            // open the SD file
    if (!Datafile) {                                                // oops - file not available!
        console_message = "Error re-opening Datafile:" + String(DataFileName);
        Write_Console_Message();
        while (true) {
            digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
            digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
            delay(500);
            Check_Red_Switch();                               // Reset will restart the processor so no return
        }
    }
    SD_freespace = (SD.totalBytes() - SD.usedBytes());
    console.print(millis(), DEC); console.println("\tNew Data Record Written");
    Datafile.print(Data_Date); Datafile.print(",");                 // 1. dd/mm/yyyy,
    Datafile.print(Data_Time); Datafile.print(",");                 // 2. dd/mm/yyyy,hh:mm:ss,
    SDprintDouble(Data_Values[0], 1); Datafile.print(",");          // 3. voltage
    SDprintDouble(Data_Values[1], 3); Datafile.print(",");          // 4. amperage
    SDprintDouble(Data_Values[2], 2); Datafile.print(",");          // 5. wattage
    SDprintDouble(Data_Values[3], 1); Datafile.print(",");          // 6. up time
    SDprintDouble(Data_Values[4], 3); Datafile.print(",");          // 7. kilowatt hour
    SDprintDouble(Data_Values[5], 2); Datafile.print(",");          // 8. power factor
    SDprintDouble(Data_Values[6], 1); Datafile.print(",");          // 9. unknown
    SDprintDouble(Data_Values[7], 1); Datafile.print(",");          // 10. frequency
    SDprintDouble(Data_Values[8], 1); Datafile.print(",");      // 11. temperature
    //                                                                      weather values
    SDprintDouble(weather_record.temp, 2); Datafile.print(",");             // 12. temperature
    SDprintDouble(weather_record.pressure, 0); Datafile.print(",");         // 13. pressure
    SDprintDouble(weather_record.humidity, 0); Datafile.print(",");         // 14. humidity
    SDprintDouble(weather_record.wind_direction, 0); Datafile.print(",");   // 15. wind direction
    SDprintDouble(weather_record.wind_speed, 0); Datafile.print(",");       // 16. wind speed
    Datafile.print(weather_record.weather);                                 // 17. description
    Datafile.print("\n");                                      // end of record
    Datafile.close();                                          // close the sd file
    Datafile.flush();                                          // make sure it has been written to SD
    digitalWrite(SD_Active_led_pin, LOW);
    // highest voltage ------------------------------------------------------------------------------------------------
    if (Data_Values[0] >= highest_voltage) {
        for (i = 0; i <= Data_Time.length() + 1; i++) {         // load the time
            time_of_highest_voltage[i] = Data_Time[i];
        }
        highest_voltage = Data_Values[0];                       // update the largest current value
    }
    // lowest voltage -------------------------------------------------------------------------------------------------
    if (Data_Values[0] <= lowest_voltage) {
        for (i = 0; i <= Data_Time.length() + 1; i++) {         // load the time
            time_of_lowest_voltage[i] = Data_Time[i];
        }
        lowest_voltage = Data_Values[0];                        // update the largest current value
    }
    // largest amperage -----------------------------------------------------------------------------------------------    if (Data_Values[1] >= largest_amperage) {                  // load the maximum amperage value
    for (i = 0; i <= Data_Time.length() + 1; i++) {                               // load the time
        time_of_largest_amperage[i] = Data_Time[i];
    }
    largest_amperage = Data_Values[1];                      // update the largest current value
    // weather information --------------------------------------------------------------------------------------------
    if (weather_record.temp >= highest_temperature) {           // update the highest weather temperature
        for (i = 0; i <= Data_Time.length() + 1; i++) {         // load the time
            time_of_highest_temperature[i] = Data_Time[i];
        }
        highest_temperature = weather_record.temp;              // update the highest weather temperature
    }
    // latest temperature ---------------------------------------------------------------------------------------------
    for (i = 0; i <= Data_Time.length() + 1; i++) {             // load the time
        time_of_latest_temperature[i] = Data_Time[i];
    }
    latest_temperature = weather_record.temp;                   // update the highest weather temperature
    // lowest temperature ---------------------------------------------------------------------------------------------
    if (weather_record.temp <= lowest_temperature) {            // update the lowest weather temperature
        for (i = 0; i <= Data_Time.length() + 1; i++) {         // load the time
            time_of_lowest_temperature[i] = Data_Time[i];
        }
        lowest_temperature = weather_record.temp;               // update the lowest weather temperature
    }
    // pressure -------------------------------------------------------------------------------------------------------
    for (i = 0; i <= Data_Time.length() + 1; i++) {             // load the time
        time_of_latest_pressure[i] = Data_Time[i];
    }
    latest_pressure = weather_record.pressure;
    // humidity -------------------------------------------------------------------------------------------------------
    for (i = 0; i <= Data_Time.length() + 1; i++) {             // load the time
        time_of_latest_humidity[i] = Data_Time[i];
    }
    latest_humidity = weather_record.humidity;
    // wind speed -----------------------------------------------------------------------------------------------------
    for (i = 0; i <= Data_Time.length() + 1; i++) {             // load the time
        time_of_latest_wind_speed[i] = Data_Time[i];
    }
    latest_wind_speed = weather_record.wind_speed;
    // wind direction -------------------------------------------------------------------------------------------------
    for (i = 0; i <= Data_Time.length() + 1; i++) {             // load the time
        time_of_latest_wind_direction[i] = Data_Time[i];
    }
    latest_wind_direction = weather_record.wind_direction;
    // weather description --------------------------------------------------------------------------------------------
    for (i = 0; i <= Data_Time.length() + 1; i++) {             // load the time
        time_of_latest_weather[i] = Data_Time[i];
    }
    latest_weather = weather_record.weather;
    // ----------------------------------------------------------------------------------------------------------------
    SD_freespace_double = (double)SD_freespace / 1000000;
    if (SD_freespace < critical_SD_freespace) {
        console_message = "\tWARNING - SD Free Space critical " + String(SD_freespace) + "MBytes";
        Write_Console_Message();
    }
    record_count++;                                             // increment the record count, the array pointer
    current_data_record_count++;                                     // increment the current record count
    digitalWrite(SD_Active_led_pin, LOW);                       // turn the SD activity LED on
}
void Add_New_Data_Record_to_Display_Table() {
    if (record_count > table_size) {                            // table full, shuffle fifo
        record_count = table_size;
        for (i = 0; i < table_size; i++) {                                      // shuffle the rows up, losing row 0, make row [table_size] free
            strcpy(readings_table[i].ldate, readings_table[i + 1].ldate);           // date
            strcpy(readings_table[i].ltime, readings_table[i + 1].ltime);           // time
            readings_table[i].voltage = readings_table[i + 1].voltage;              // voltage
            readings_table[i].amperage = readings_table[i + 1].amperage;            // amperage
            readings_table[i].wattage = readings_table[i + 1].wattage;              // wattage
            readings_table[i].uptime = readings_table[i + 1].uptime;                // up time
            readings_table[i].kilowatthour = readings_table[i + 1].kilowatthour;    // kilowatt hour
            readings_table[i].powerfactor = readings_table[i + 1].powerfactor;      // power factor
            readings_table[i].unknown = readings_table[i + 1].unknown;              // unknown
            readings_table[i].frequency = readings_table[i + 1].frequency;          // frequency
            readings_table[i].temperature = readings_table[i + 1].temperature;      // temperature
        }
        record_count = table_size;                                                  // subsequent records will be added at the end of the table
        for (i = 0; i <= Data_Date.length() + 1; i++) {
            readings_table[table_size].ldate[i] = Data_Date[i];
        }                                                                           // write the new reading to the end of the table
        for (i = 0; i <= Data_Time.length() + 1; i++) {
            readings_table[table_size].ltime[i] = Data_Time[i];
        }
        readings_table[table_size].voltage = Data_Values[Voltage];
        readings_table[table_size].amperage = Data_Values[Amperage];
        readings_table[table_size].wattage = Data_Values[Wattage];                 // write the watts value into the table
        readings_table[table_size].uptime = Data_Values[UpTime];
        readings_table[table_size].kilowatthour = Data_Values[Kilowatthour];
        readings_table[table_size].powerfactor = Data_Values[PowerFactor];
        readings_table[table_size].unknown = Data_Values[Unknown];
        readings_table[table_size].frequency = Data_Values[Frequency];
        readings_table[table_size].temperature = Data_Values[Temperature];
    }
    else {                                                                          // add the record to the table
        for (i = 0; i <= Data_Date.length() + 1; i++) {
            readings_table[record_count].ldate[i] = Data_Date[i];
        }                                                                         // write the new reading to the end of the table
        for (i = 0; i <= Data_Time.length() + 1; i++) {
            readings_table[record_count].ltime[i] = Data_Time[i];
        }
        readings_table[record_count].voltage = Data_Values[Voltage];
        readings_table[record_count].amperage = Data_Values[Amperage];
        readings_table[record_count].wattage = Data_Values[Wattage];
        readings_table[record_count].uptime = Data_Values[UpTime];
        readings_table[record_count].kilowatthour = Data_Values[Kilowatthour];
        readings_table[record_count].powerfactor = Data_Values[PowerFactor];
        readings_table[record_count].unknown = Data_Values[Unknown];
        readings_table[record_count].frequency = Data_Values[Frequency];
        readings_table[record_count].temperature = Data_Values[Temperature];
    }                                                                           // end of if record_count > table_size
}
void Create_New_Console_File() {
    char milliseconds[10];
    digitalWrite(SD_Active_led_pin, HIGH);                          // turn the SD activity LED on
    ConsoleFileName = GetDate(false) + ".txt";                      // yes, so create a new file
    if (!SD.exists("/" + ConsoleFileName)) {
        Consolefile = SD.open("/" + ConsoleFileName, FILE_WRITE);
        if (!Consolefile) {                                         // log file not opened
            console_message = "Error opening Console file: [" + String(ConsoleFileName) + "]";
            Write_Console_Message();
            while (true) {
                digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                delay(500);
                Check_Red_Switch();
            }
        }
        Consolefile.println(GetDate(true) + "," + GetTime(true) + "," + milliseconds + ",Console File Started");
        Consolefile.close();
        Consolefile.flush();
        console_record_count = 1;
    }
    else {
        console_message = "Console File " + String(ConsoleFileName) + " already exists";
        Write_Console_Message();
        Prefill_Console_Array();
    }
    digitalWrite(SD_Active_led_pin, LOW);                           // turn the SD activity LED off
}
void Create_New_Data_File() {
    DataFileName = GetDate(false) + ".csv";
    digitalWrite(SD_Active_led_pin, HIGH);
    if (!SD.exists("/" + DataFileName)) {
        Datafile = SD.open("/" + DataFileName, FILE_WRITE);
        if (!Datafile) {                                            // log file not opened
            console_message = "Error opening Data file @ line 324 [" + String(DataFileName) + "]";
            Write_Console_Message();
            while (true) {
                digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                delay(500);
                Check_Red_Switch();
            }
        }
        Datafile.print("Date,");                                    // write first column header
        Datafile.print("Time,");                                    // write second column header
        for (int x = 0; x < 14; x++) {                               // write data column headings into the SD file
            Datafile.print(RS485_FieldNames[x]);
            Datafile.print(",");
        }
        Datafile.println(RS485_FieldNames[14]);
        Datafile.close();
        Datafile.flush();
        current_data_record_count = 0;
        digitalWrite(SD_Active_led_pin, LOW);
        This_Date = GetDate(false);                                 // update the current date
        console_message = "Data File Created " + DataFileName;
        Write_Console_Message();
    }
    else {
        console_message = "Data File " + String(DataFileName) + " already exists";
        Write_Console_Message();
        Prefill_Array();
    }
}
void Write_Console_Message() {
    String saved_console_message = console_message;
    String Date;
    String Time;
    unsigned long milliseconds = millis();
    String pre_date = "1951/18/11";
    String pre_time = "00:00:00";
    if (Post_Setup_Status) {                                                // only write the console message to disk once setup is complete
        if (pre_loop_message_count > 0) {                                   // are there any pre loop console messages stored
            for (int x = 0; x < pre_loop_message_count; x++) {
                pre_date = GetDate(true);
                pre_time = GetTime(true);
                console_message = pre_loop_messages[x];
                Write_New_Console_Message_to_Console_File(pre_date, pre_time, pre_loop_millis_values[x]);
                Add_New_Console_Message_to_Console_Table(pre_date, pre_time, pre_loop_millis_values[x]);
            }
            pre_loop_message_count = 0;
        }
        Date = GetDate(true);
        Time = GetTime(true);
        console_message = saved_console_message;                            // restore the current console_message
        Write_New_Console_Message_to_Console_File(Date, Time, milliseconds);
        Add_New_Console_Message_to_Console_Table(Date, Time, milliseconds);
    }
    else {
        pre_loop_millis_values[pre_loop_message_count] = millis();
        pre_loop_messages[pre_loop_message_count] = console_message;
        pre_loop_message_count++;
    }
    console_message = saved_console_message;
    console.print(millis(), DEC); console.print("\t"); console.println(console_message);
}
void Write_New_Console_Message_to_Console_File(String date, String time, unsigned long milliseconds) {
    digitalWrite(SD_Active_led_pin, HIGH);                          // turn the SD activity LED on
    Consolefile = SD.open("/" + ConsoleFileName, FILE_APPEND);      // open the SD file
    if (!Consolefile) {                                             // oops - file not available!
        console_message = "Error re-opening Console file: " + String(ConsoleFileName);
        Write_Console_Message();
        while (true) {
            digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
            digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
            delay(500);
            Check_Red_Switch();                                   // Reset will restart the processor so no return
        }
    }
    Consolefile.print(date);
    Consolefile.print(",");
    Consolefile.print(time);
    Consolefile.print(",");
    Consolefile.print(milliseconds);
    Consolefile.print(",");
    Consolefile.println(console_message);
    Consolefile.close();                                            // close the sd file
    Consolefile.flush();                                            // make sure it has been written to SD
    SD_freespace_double = (double)SD_freespace / 1000000;
    if (SD_freespace < critical_SD_freespace) {
        for (int x = 0; x < console_message.length(); x++) {        // reserve the current console_message
            temp_message[x] = console_message[x];
        }
        console_message = "WARNING - SD Free Space critical " + String(SD_freespace) + "MBytes";
        Write_Console_Message();
        for (int x = 0; x < temp_message.length(); x++) {            // restore the console_message
            console_message[x] = temp_message[x];
        }
    }
    digitalWrite(SD_Active_led_pin, LOW);                           // turn the SD activity LED off
}
void Add_New_Console_Message_to_Console_Table(String date, String time, unsigned long milliseconds) {
    if (console_record_count > console_table_size) {                                // table full, shuffle fifo
        for (int i = 0; i < console_table_size; i++) {                              // shuffle the rows up, losing row 0, make row [table_size] free
            strcpy(console_table[i].ldate, console_table[i + 1].ldate);             // date
            strcpy(console_table[i].ltime, console_table[i + 1].ltime);             // time
            console_table[i].milliseconds = console_table[i + 1].milliseconds;
            strcpy(console_table[i].message, console_table[i + 1].message);
        }
        console_record_count = console_table_size;               // subsequent records will be added at the end of the table
        for (i = 0; i < 10; i++) {                              // write the new message date (string) onto the end of the table Char array
            console_table[console_table_size].ldate[i] = date[i];
        }
        for (i = 0; i < 8; i++) {                              // write the new message time onto the end of the table
            console_table[console_table_size].ltime[i] = time[i];
        }
        console_table[console_table_size].milliseconds = milliseconds;
        for (i = 0; i <= 119; i++) {
            console_table[console_table_size].message[i] = console_message[i];   // write the new message onto the end of the table
            if (console_table[console_table_size].message[i] == '\0') break;
        }
    }
    else {                                                                      // add the record to the table
        for (i = 0; i < 10; i++) {
            console_table[console_record_count].ldate[i] = date[i];
        }                                                                       // write the new reading to the end of the table
        for (i = 0; i < 8; i++) {
            console_table[console_record_count].ltime[i] = time[i];
        }
        console_table[console_record_count].milliseconds = milliseconds;
        for (i = 0; i < 119; i++) {
            console_table[console_record_count].message[i] = console_message[i];      // write the new message onto the end of the table
            if (console_table[console_record_count].message[i] == '\0') break;
        }
    }
    console_record_count++;                                                     // increment the console record count
}
void Check_WiFi() {
    while (WiFi.status() != WL_CONNECTED) {                     // whilst it is not connected keep trying
        int wifi_connection_attempts = 0;
        delay(500);
        console_message = "Connection attempt " + String(wifi_connection_attempts);
        Write_Console_Message();
        if (wifi_connection_attempts++ > 20) {
            int WiFi_Status = WiFi.status();
            console_message = "WiFi Status: " + String(WiFi_Status) + " " + WiFi_Status_Message[WiFi_Status];  // display current status of WiFi
            Write_Console_Message();
            console_message = "Network Error, Restarting";
            Write_Console_Message();
            ESP.restart();
        }
    }
}
int StartWiFi(const char* ssid, const char* password) {
    console_message = "WiFi Connecting to " + String(ssid);
    Write_Console_Message();
    WiFi.disconnect(true);                                      // disconnect to set new wifi connection
    WiFi.begin(ssid, password);                                 // connect to the wifi network
    int WiFi_Status = WiFi.status();
    console_message = "WiFi Status: " + String(WiFi_Status) + " " + WiFi_Status_Message[WiFi_Status];  // display current status of WiFi
    Write_Console_Message();
    int wifi_connection_attempts = 0;                           // zero the attempt counter
    while (WiFi.status() != WL_CONNECTED) {                     // whilst it is not connected keep trying
        delay(500);
        console_message = "Connection attempt " + String(wifi_connection_attempts);
        Write_Console_Message();
        if (wifi_connection_attempts++ > 20) {
            console_message = "Network Error, Restarting";
            Write_Console_Message();
            ESP.restart();
        }
    }
    WiFi_Status = WiFi.status();
    console_message = "WiFi Status: " + String(WiFi_Status) + " " + WiFi_Status_Message[WiFi_Status];  // display current status of WiFi
    Write_Console_Message();
    WiFi_Signal_Strength = (int)WiFi.RSSI();
    console_message = "WiFi Signal Strength:" + String(WiFi_Signal_Strength);
    Write_Console_Message();
    console_message = "WiFi IP Address: " + String(WiFi.localIP().toString().c_str());
    Write_Console_Message();
    return true;
}
void StartTime() {
    console_message = "Starting Time Server";
    Write_Console_Message();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    console_message = "Time Server Started";
    Write_Console_Message();
}
void Prefill_Array() {
    int datacharacter_count = 0;
    char dataField[20];
    int datafieldNo = 1;
    char datatemp;
    double this_temperature = 0;
    current_data_record_count = 0;
    SD_Led_Flash_Start_Stop(true);                                              // start the sd led flashing
    File dataFile = SD.open("/" + DataFileName, FILE_READ);
    if (dataFile) {
        while (dataFile.available()) {                                           // throw the first row, column headers, away
            datatemp = dataFile.read();
            if (datatemp == '\n') break;
        }
        console_message = "Loading datafile from " + String(DataFileName);
        Write_Console_Message();
        while (dataFile.available()) {                                           // do while there are data available
            Flash_SD_LED();                                                     // flash the sd led
            datatemp = dataFile.read();
            dataField[datacharacter_count++] = datatemp;                            // add it to the csvfield string
            if (datatemp == ',' || datatemp == '\n') {                                  // look for end of field
                dataField[datacharacter_count - 1] = '\0';                           // insert termination character where the ',' or '\n' was
                switch (datafieldNo) {
                case 1:
                    strcpy(readings_table[record_count].ldate, dataField);       // Date
                    break;
                case 2:
                    strcpy(readings_table[record_count].ltime, dataField);       // Time
                    break;
                case 3:
                    readings_table[record_count].voltage = atof(dataField);      // Voltage
                    break;
                case 4:
                    readings_table[record_count].amperage = atof(dataField);     // Amperage
                    break;
                case 5:
                    readings_table[record_count].wattage = atof(dataField);      // Wattage
                    break;
                case 6:
                    readings_table[record_count].uptime = atof(dataField);       // Up Time
                    break;
                case 7:
                    readings_table[record_count].kilowatthour = atof(dataField); // KiloWatt Hour
                    break;
                case 8:
                    readings_table[record_count].powerfactor = atof(dataField);  // Power Factor
                    break;
                case 9:
                    readings_table[record_count].unknown = atof(dataField);      // Unknown
                    break;
                case 10:
                    readings_table[record_count].frequency = atof(dataField);    // Frequency
                    break;
                case 11:
                    readings_table[record_count].temperature = atof(dataField);  // Temperature
                    break;
                case 12:
                    // weather temperature
                    this_temperature = atof(dataField);                  // weather temperature
                    break;
                case 13:
                    // weather pressure
                    break;
                case 14:
                    // weather humidity
                    break;
                case 15:
                    // weather wind direction
                    break;
                case 16:
                    // weather wind speed
                    break;
                case 17:
                    // weather description
                    break;
                }
                datafieldNo++;
                dataField[0] = '\0';
                datacharacter_count = 0;
            }
            if (datatemp == '\n') {                                                             // end of sd data row
                if (readings_table[record_count].voltage >= highest_voltage) {
                    for (i = 0; i <= Data_Time.length() + 1; i++) {                               // load the time
                        time_of_lowest_voltage[i] = readings_table[record_count].ltime[i];
                    }
                    highest_voltage = readings_table[record_count].voltage;
                }
                if (readings_table[record_count].voltage <= lowest_voltage) {
                    for (i = 0; i <= Data_Time.length() + 1; i++) {                               // load the time
                        time_of_lowest_voltage[i] = readings_table[record_count].ltime[i];
                    }
                    lowest_voltage = readings_table[record_count].voltage;                     // update the largest current value
                }
                if (readings_table[record_count].amperage >= largest_amperage) {                  // load the maximum amperage value
                    for (i = 0; i <= Data_Time.length() + 1; i++) {                               // load the time
                        time_of_largest_amperage[i] = readings_table[record_count].ltime[i];
                    }
                    largest_amperage = readings_table[record_count].amperage;                     // update the largest current value
                }
                //                                                          weather information
                if (this_temperature >= highest_temperature) {       // update the highest weather temperature
                    for (i = 0; i <= Data_Time.length() + 1; i++) {             // load the time
                        time_of_highest_temperature[i] = readings_table[record_count].ltime[i];
                    }
                    highest_temperature = this_temperature;          // update the highest weather temperature
                }
                if (this_temperature <= lowest_temperature) {        // update the lowest weather temperature
                    for (i = 0; i <= Data_Time.length() + 1; i++) {             // load the time
                        time_of_lowest_temperature[i] = readings_table[record_count].ltime[i];
                    }
                    lowest_temperature = this_temperature;           // update the lowest weather temperature
                }
                record_count++;                                                                 // increment array pointer
                current_data_record_count++;                                                    // increment the current_record count
                if (record_count > table_size) {                                                // if pointer is greater than table size
                    for (int i = 0; i < table_size; i++) {                                      // shuffle the rows up, losing row 0, make row [table_size] free
                        strcpy(readings_table[i].ldate, readings_table[i + 1].ldate);           // date
                        strcpy(readings_table[i].ltime, readings_table[i + 1].ltime);           // time
                        readings_table[i].voltage = readings_table[i + 1].voltage;              // voltage
                        readings_table[i].amperage = readings_table[i + 1].amperage;            // amperage
                        readings_table[i].wattage = readings_table[i + 1].wattage;              // wattage
                        readings_table[i].uptime = readings_table[i + 1].uptime;                // up time
                        readings_table[i].kilowatthour = readings_table[i + 1].kilowatthour;    // kilowatt hour
                        readings_table[i].powerfactor = readings_table[i + 1].powerfactor;      // power factor
                        readings_table[i].unknown = readings_table[i + 1].unknown;              // unknown
                        readings_table[i].frequency = readings_table[i + 1].frequency;          // frequency
                        readings_table[i].temperature = readings_table[i + 1].temperature;      // temperature
                    }
                    record_count = table_size;                                                  // subsequent records will be added at the end of the table
                }
                datafieldNo = 1;
            }
        } // end of while
    }
    dataFile.close();
    console_message = "Loaded Data Records: " + String(current_data_record_count);
    Write_Console_Message();
    SD_Led_Flash_Start_Stop(false);
}
void Prefill_Console_Array() {
    int console_character_count = 0;
    char console_txtField[120];
    int console_fieldNo = 1;
    char console_temp;
    current_console_record_count = 0;
    SD_Led_Flash_Start_Stop(true);
    File consoleFile = SD.open("/" + ConsoleFileName, FILE_READ);
    if (consoleFile) {
        console_message = "Loading console file from " + String(ConsoleFileName);
        Write_Console_Message();
        while (consoleFile.available()) {                                       // do while there are data available
            Flash_SD_LED();
            console_temp = consoleFile.read();                                  // read a character from the file
            console_txtField[console_character_count++] = console_temp;         // add it to the consolefield string
            if (console_temp == ',' || console_temp == '\n') {                  // look for end of field
                if (console_fieldNo != 3) {                                     // field 3 is not a string, it is a long
                    console_txtField[console_character_count - 1] = '\0';       // so do not terminate it with a /0
                }                                                               // insert termination character where the ',' or '\n' was
                switch (console_fieldNo) {
                case 1:
                    strcpy(console_table[console_record_count].ldate, console_txtField);        // Date
                    break;
                case 2:
                    strcpy(console_table[console_record_count].ltime, console_txtField);        // Time
                    break;
                case 3:
                    console_table[console_record_count].milliseconds = atof(console_txtField); // milliseconds
                    break;
                case 4:
                    strcpy(console_table[console_record_count].message, console_txtField);      // message
                    break;
                }
                console_fieldNo++;
                console_txtField[0] = '\0';
                console_character_count = 0;
            }
            if (console_temp == '\n') {                                                         // end of sd data row
                console_record_count++;                                                         // increment array pointer
                current_console_record_count++;                                                 // increment the current_record count
                if (console_record_count > console_table_size) {                                // if pointer is greater than table size
                    for (int i = 0; i < console_table_size; i++) {                              // shuffle the rows up, losing row 0, make row [table_size] free
                        strcpy(console_table[i].ldate, console_table[i + 1].ldate);             // date
                        strcpy(console_table[i].ltime, console_table[i + 1].ltime);             // time
                        console_table[i].milliseconds = console_table[i + 1].milliseconds;
                        strcpy(console_table[i].message, console_table[i + 1].message);
                    }
                    console_record_count = console_table_size;                                                  // subsequent records will be added at the end of the table
                }
                console_fieldNo = 1;
            }
        } // end of while
    }
    consoleFile.close();
    console_message = "Loaded Console Records: " + String(current_console_record_count);
    Write_Console_Message();
    SD_Led_Flash_Start_Stop(false);
}
void Display() {
    webpage = "";                           // don't delete this command, it ensures the server works reliably!
    Page_Header(true, "Energy Usage Monitor");
    // <script> -------------------------------------------------------------------------------------------------------
    webpage += F("<script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>");
    webpage += F("<script type=\"text/javascript\">");
    webpage += F("google.charts.load('current', {packages: ['corechart', 'line']});");
    webpage += F("google.setOnLoadCallback(drawChart);");
    webpage += F("google.charts.load('current', {'packages': ['bar'] });");
    webpage += F("google.charts.setOnLoadCallback(drawChart);");
    webpage += F("function drawChart() {");
    webpage += F("var data=new google.visualization.DataTable();");
    webpage += F("data.addColumn('timeofday', 'Time');");
    webpage += F("data.addColumn('number', 'Amperage');");
    webpage += F("data.addRows([");
    for (int i = 0; i < (record_count); i++) {
        if (String(readings_table[i].ltime) != "") {                  // if the ltime field contains data
            for (int y = 0; y < 8; y++) {                               // replace the ":"s in ltime with ","
                if (readings_table[i].ltime[y] == ':') {
                    readings_table[i].ltime[y] = ',';
                }
            }
            webpage += "[[";
            webpage += String(readings_table[i].ltime) + "],";
            webpage += String(readings_table[i].amperage * 1000, 1) + "]";
            if (i != record_count) webpage += ",";    // do not add a "," to the last record
        }
    }
    webpage += "]);\n";
    webpage += F("var options = {");
    webpage += F("title:'Electrical Power Consumption',titleTextStyle:{fontName:'Arial', fontSize:20, color: 'Maroon'},");
    webpage += F("legend:{position:'bottom'},colors:['red'],backgroundColor:'#F3F3F3',chartArea: {width:'90%', height:'60%'},");
    webpage += F("hAxis:{slantedText:true,slantedTextAngle:90,titleTextStyle:{width:'100%',color:'Purple',bold:true,fontSize:16},");
    webpage += F("gridlines:{color:'#333'},showTextEvery:1");
    //   webpage += F(",title:'Time'");
    webpage += F("},");
    webpage += F("vAxes:");
    webpage += F("{0:{viewWindowMode:'explicit',gridlines:{color:'black'}, viewWindow:{min:0,max:10000},scaleType: 'log',title:'Amperage (mA)',format:'#####'},");
    webpage += F("}, ");
    webpage += F("series:{0:{targetAxisIndex:0},curveType:'none'},};");
    webpage += F("var chart = new google.visualization.LineChart(document.getElementById('line_chart'));chart.draw(data, options);");
    webpage += F("}");
    webpage += F("</script>");
    // </script> ------------------------------------------------------------------------------------------------------
    webpage += F("<div id='line_chart' style='width:960px; height:600px'></div>");
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
    lastcall = "display";
}
void Web_Reset() {
    console_message = "Processor Reset via Web Command";
    Write_Console_Message();
    ESP.restart();
}
void Page_Header(bool refresh, String Header) {
    webpage = "<!DOCTYPE html><head>";
    // <html start -----------------------------------------------------------------------------------------------------
    webpage += F("<html");                                                                  // start of HTML section
    webpage += F("lang='en'>");
    if (refresh) webpage += F("<meta http-equiv='refresh' content='20'>");                  // 20-sec refresh time
    webpage += F("<meta name='viewport' content='width=");
    webpage += site_width;
    webpage += F(", initial-scale=1'>");
    webpage += F("<meta http-equiv='Cache-control' content='public'>");
    webpage += F("<meta http-equiv='X-Content-Type-Options:nosniff';");
    webpage += F("<title>");
    webpage += F("</title>");
    // <h1 start ------------------------------------------------------------------------------------------------------
    webpage += F("<h1 ");                                                                   // start of h1
    webpage += F("style='text-align:center;'>");
    // <span start ----------------------------------------------------------------------------------------------------
    // colour of header tile letters
    webpage += F("<span style='color:DodgerBlue; font-size:36pt;'>");                              // start of span
    webpage += Header;
    // </span> end ----------------------------------------------------------------------------------------------------
    webpage += F("</span>");                                                                // end of span
    webpage += F("<span style='font-size: medium;'><align center=''></align></span>");
    // <h1> end -------------------------------------------------------------------------------------------------------
    webpage += F("</h1>");                                                                  // end of h1
    // </style> -------------------------------------------------------------------------------------------------------
    //                                                                                               #31c1f9
    // background colour for footer row
    webpage += F("<style>ul{list-style-type:none;margin:0;padding:0;overflow:hidden;background-color:#31c1f9;font-size:14px;}");
    webpage += F("li{float:left;}");
    webpage += F("li a{display:block;text-align:center;padding:5px 25px;text-decoration:none;}");
    webpage += F("li a:hover{background-color:#FFFFFF;}");
    //                                #31c1f9
    webpage += F("h1{background-color:White;}");
    webpage += F("body{width:");
    webpage += site_width;
    webpage += F("px;margin:0 auto;font-family:arial;font-size:14px;text-align:center;");
    webpage += F("color:#ed6495;background-color:#F7F2Fd;}");
    // </style> end ---------------------------------------------------------------------------------------------------
    webpage += F("</style>");                                                               // end of style section
    // </head> end ----------------------------------------------------------------------------------------------------
    webpage += F("</head>");                                                                // end of head section
    // <body> start ---------------------------------------------------------------------------------------------------
    webpage += F("<body>");                                                                 // start of body
}
void Page_Footer() {
    char signature[20] = { 0xA9,0x53,0x74,0x65,0x70,0x68,0x65,0x6E,0x20,0x47,0x6F,0x75,0x6C,0x64,0x20,0x32,0x30,0x32,0x32,0x00 };
    // <ul> start -----------------------------------------------------------------------------------------------------
    webpage += F("<ul>");
    webpage += F("<li><a href='/Display'>Webpage</a> </li>");
    webpage += F("<li><a href='/Statistics'>Display Statistics</a></li>");
    webpage += F("<li><a href='/DownloadFiles'>Download Files</a></li>");
    webpage += F("<li><a href='/DeleteFiles'>Delete Files</a></li>");
    webpage += F("<li><a href='/Reset'>Reset Processor</a></li>");
    webpage += F("<li><a href='/ConsoleShow'>Show Console</a></li>");
    // </ul> end ------------------------------------------------------------------------------------------------------
    webpage += F("</ul>");
    // <footer> start -------------------------------------------------------------------------------------------------
    webpage += F("<footer>");
    // <p -------------------------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style = 'text-align: center;'>");
    // <span start ----------------------------------------------------------------------------------------------------
    webpage += F("<span ");
    webpage += F("style = 'color: red;'");
    webpage += F(">");
    webpage += String(signature);
    webpage += F(" (");
    webpage += String(version);
    webpage += F(") Last Page Update - ");
    webpage += GetTime(true);
    webpage += F(" SD Free Space = ");
    webpage += String(SD_freespace_double, 2) + " MB";
    // </span> end ----------------------------------------------------------------------------------------------------
    webpage += F("</span>");
    // </p> end -------------------------------------------------------------------------------------------------------
    webpage += F("</p>");
    // </footer> end --------------------------------------------------------------------------------------------------
    webpage += F("</footer>");
    // </body> end ----------------------------------------------------------------------------------------------------
    webpage += F("</body>");
    // </html> end ----------------------------------------------------------------------------------------------------
    webpage += F("</html>");
}
void Statistics() {                                                 // Display file size of the datalog file
    int file_count = Count_Files_on_SD_Drive();
    webpage = ""; // don't delete this command, it ensures the server works reliably!
    Page_Header(true, "Energy Monitor Statistics");
    File datafile = SD.open("/" + DataFileName, FILE_READ);  // Now read data from FS
    // Wifi Signal Strength -------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">WiFi Signal Strength = ");
    webpage += String(WiFi_Signal_Strength) + " Dbm";
    webpage += "</span></strong></p>";
    // Data File Size -------------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Current Data file size = ");
    webpage += String(datafile.size());
    webpage += F(" Bytes");
    webpage += "</span></strong></p>";
    // Console File Size ----------------------------------------------------------------------------------------------
    File consolefile = SD.open("/" + ConsoleFileName, FILE_READ);  // Now read data from FS
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Current Console file size = ");
    webpage += String(consolefile.size());
    webpage += F(" Bytes");
    webpage += "</span></strong></p>";
    // Freespace ------------------------------------------------------------------------------------------------------
    if (SD_freespace < critical_SD_freespace) {
        webpage += F("<p ");
        webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
        webpage += F("'>SD Free Space = ");
        webpage += String(SD_freespace / 1000000);
        webpage += F(" MB");
        webpage += "</span></strong></p>";
    }
    else {
        webpage += F("<p ");
        webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
        webpage += F("'>SD Free Space = ");
        webpage += String(SD_freespace / 1000000);
        webpage += F(" MB");
        webpage += "</span></strong></p>";
    }
    // File Count -----------------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'> Number of Files on SD : ");
    webpage += String(file_count - 1);
    webpage += "</span></strong></p>";
    // Last Boot Time -------------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Date the System was Booted : ");
    webpage += Last_Boot_Date + " at " + Last_Boot_Time;
    webpage += "</span></strong></p>";
    // ----------------------------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Number of readings = ");
    webpage += String(current_data_record_count);
    webpage += "</span></strong></p>";
    // Highest Voltage ------------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Highest Voltage was Recorded at ");
    webpage += String(time_of_highest_voltage) + " : ";
    webpage += String(highest_voltage) + " volts";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Lowest Voltage -------------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Lowest Voltage was Recorded at ");
    webpage += String(time_of_lowest_voltage) + " : ";
    webpage += String(lowest_voltage) + " volts";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Greatest Amperage ----------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Greatest Amperage was Recorded at ");
    webpage += String(time_of_largest_amperage) + " : ";
    webpage += String(largest_amperage) + " ma";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Latest Temperature -------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Latest Outside Temperature was Recorded at ");
    webpage += String(time_of_latest_temperature) + " : ";
    webpage += String(latest_temperature, 2);
    webpage += F("&deg;C");
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Lowest Temperature -------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Lowest Outside Temperature was Recorded at ");
    webpage += String(time_of_lowest_temperature) + " : ";
    webpage += String(lowest_temperature, 2);
    webpage += F("&deg;C");
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Highest Temperature -------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Highest Outside Temperature was Recorded at ");
    webpage += String(time_of_highest_temperature) + " : ";
    webpage += String(highest_temperature, 2);
    webpage += F("&deg;C");
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Humidity -------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Latest Relative Humidity Recorded at ");
    webpage += String(time_of_latest_humidity) + " : ";
    webpage += String(latest_humidity) + "%";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Pressure -------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Latest Atmospheric Pressure Recorded at ");
    webpage += String(time_of_latest_pressure) + " : ";
    webpage += String(latest_pressure) + " millibar";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather -------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Latest Weather Description Recorded at ");
    webpage += String(time_of_latest_weather) + " : ";
    webpage += String(latest_weather);
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Wind Speed -------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Latest Wind Speed Recorded at ");
    webpage += String(time_of_latest_wind_speed) + " : ";
    webpage += String(latest_wind_speed) + "m/s";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Direction -------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Latest Wind Direction Recorded at ");
    webpage += String(time_of_latest_wind_direction) + " : ";
    webpage += String(latest_wind_direction);
    webpage += F("&deg;");
    webpage += "</span></strong></p>";
    //   webpage += F("<p ");
       // ----------------------------------------------------------------------------------------------------------------
    datafile.close();
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
}
void Download_Files() {
    int file_count = Count_Files_on_SD_Drive();                                 // this counts and creates an array of file names on SD
    //    console_message = "Download of Files Requested via Webpage";
    //    Write_Console_Message();
    webpage = "";
    Page_Header(false, "Energy Monitor Download Files");
    for (i = 1; i < file_count; i++) {
        webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(i) + " " + String(FileNames[i]) + " ";
        webpage += "&nbsp;<a href=\"/GetFile?file=" + String(FileNames[i]) + " " + "\">Download</a>";
        webpage += "</h3>";
    }
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
}
void Download_File() {                                                          // download the selected file
    String fileName = server.arg("file");
    console_message = "Download of File " + fileName + " Requested via Webpage";
    Write_Console_Message();
    File datafile = SD.open("/" + fileName, FILE_READ);    // Now read data from FS
    if (datafile) {                                             // if there is a file
        if (datafile.available()) {                             // If data is available and present
            String contentType = "application/octet-stream";
            server.sendHeader("Content-Disposition", "attachment; filename=" + fileName);
            if (server.streamFile(datafile, contentType) != datafile.size()) {
                console_message = "Sent less data (" + String(server.streamFile(datafile, contentType)) + ")";
                console_message += " from " + fileName + " than expected (";
                console_message += String(datafile.size()) + ")";
                Write_Console_Message();
            }
        }
    }
    datafile.close(); // close the file:
    webpage = "";
}
void Delete_Files() {                                                           // allow the cliet to select a file for deletion
    int file_count = Count_Files_on_SD_Drive();                                 // this counts and creates an array of file names on SD
    console_message = "Delete Files Requested via Webpage";
    Write_Console_Message();
    webpage = "";
    Page_Header(false, "Energy Monitor Delete Files");
#ifndef ALLOW_WORKING_FILE_DELETION
    if (file_count > 3) {
#endif
        for (i = 1; i < file_count; i++) {
#ifndef ALLOW_WORKING_FILE_DELETION
            if (FileNames[i] != DataFileName && FileNames[i] != ConsoleFileName) {   // do not list the current file
#endif
                webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(i) + " " + String(FileNames[i]) + " ";
                webpage += "&nbsp;<a href=\"/DelFile?file=" + String(FileNames[i]) + " " + "\">Delete</a>";
                webpage += "</h3>";
#ifndef ALLOW_WORKING_FILE_DELETION
            }
#endif
        }
#ifndef ALLOW_WORKING_FILE_DELETION
    }
    else {
        webpage += F("<h3 ");
        webpage += F("style='text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:24px;'");
        webpage += F(">No Deletable Files");
        webpage += F("</span></strong></h3>");
    }
#endif
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
}
void Del_File() {                                                       // web request to delete a file
    String fileName = "\20221111.csv";                                  // dummy load to get the string space reserved
    fileName = "/" + server.arg("file");
#ifndef ALLOW_WORKING_FILE_DELETION
    if (fileName != ("/" + DataFileName)) {                            // do not delete the current file
#endif
        SD.remove(fileName);
        console_message = DataFileName + " Removed";
        Write_Console_Message();
#ifndef ALLOW_WORKING_FILE_DELETION
    }
#endif
    int file_count = Count_Files_on_SD_Drive();                         // this counts and creates an array of file names on SD
    webpage = "";                                                       // don't delete this command, it ensures the server works reliably!
    Page_Header(false, "Energy Monitor Delete Files");
#ifndef ALLOW_WORKING_FILE_DELETION
    if (file_count > 3) {
#endif
        for (i = 1; i < file_count; i++) {
#ifndef ALLOW_WORKING_FILE_DELETION
            if (FileNames[i] != DataFileName && FileNames[i] != ConsoleFileName) {   // do not list the current file
#endif
                webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(i) + " " + String(FileNames[i]) + " ";
                webpage += "&nbsp;<a href=\"/DelFile?file=" + String(FileNames[i]) + " " + "\">Delete</a>";
                webpage += "</h3>";
#ifndef ALLOW_WORKING_FILE_DELETION
            }
#endif
        }
#ifndef ALLOW_WORKING_FILE_DELETION
    }
    else {
        webpage += F("<h3 ");
        webpage += F("style='text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:24px;'");
        webpage += F(">No Deletable Files");
        webpage += F("</span></strong></h3>");
    }
#endif
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
}
void Console_Show() {
    console_message = "Web Display of Console Messages Requested via Webpage";
    Write_Console_Message();
    webpage = "";
    Page_Header(true, "Console Messages");
    for (int x = 0; x < console_table_size; x++) {
        webpage += F("<p ");
        webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
        webpage += F("'>");
        webpage += String(console_table[x].ldate);
        webpage += F(" ");
        webpage += String(console_table[x].ltime);
        webpage += F(".");
        webpage += String(console_table[x].milliseconds);
        webpage += F(": ");
        webpage += String(console_table[x].message);
        webpage += "</span></strong></p>";
    }
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
}
int Count_Files_on_SD_Drive() {
    int file_count = 0;
    bool files_present = false;
    String filename;
    File root = SD.open("/");                                       //  Open the root directory
    do {
        File entry = root.openNextFile();                           //  get the next file
        if (entry) {
            filename = entry.name();
            files_present = true;
            File datafile = SD.open("/" + filename, FILE_READ);     // Now read data from FS
            if (datafile) {                                         // if there is a file
                FileNames[file_count] = filename;
                //                console_message = "File " + String(file_count) + " filename " + String(filename);
                //                Write_Console_Message();
                file_count++;                                       // increment the file count
            }
            datafile.close(); // close the file:
            webpage = "";
        }
        else {
            root.close();
            files_present = false;
        }
    } while (files_present);
    return (file_count);
}
void Wipe_Files() {                            // selected by pressing combonation of buttons
    console_message = "Start of Wipe Files Request by Switch";
    Write_Console_Message();
    String filename;
    File root = SD.open("/");                                       //  Open the root directory
    while (true) {
        File entry = root.openNextFile();                           //  get the next file
        if (entry) {
            filename = entry.name();
            console_message = "Removing " + filename;
            Write_Console_Message();
            SD.remove(entry.name());                                //  delete the file
        }
        else {
            root.close();
            console_message = "All files removed from root directory, rebooting";
            Write_Console_Message();
            ESP.restart();
        }
    }
}
void Send_Request(int field) {
    digitalWrite(RS485_Enable_pin, transmit);                                               // set RS485_Enable HIGH to transmit values to RS485
    for (int x = 0; x < 7; x++) {
        RS485_Port.write(RS485_Requests[field][x]);                                     // write the Request string to the RS485 Port
        delay(2);
    }
    RS485_Port.write(RS485_Requests[field][7]);
    delay(2);
}
double Receive(int field) {
    byte pointer = 0;
    byte required_bytes = 0;
    int received_character = 0;
    bool value_status = false;
    unsigned long start_time;                                                   // take the start time, used to check for time out
    long value[7] = { 0,0,0,0,0,0,0 };
    digitalWrite(RS485_Enable_pin, receive);                                    // set Enable pin low to receive values from RS485
    start_time = millis();                                                      // take the start time, used to check for time out
    do {
        while (!RS485_Port.available()) {                                       // wait for some data to arrive
            if (millis() > start_time + (unsigned long)500) {                   // no data received within 500 ms so timeout
                console_message = "No Reply from RS485 within 500 ms";
                Write_Console_Message();
                while (true) {                                                          // wait for reset to be pressed
                    digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                    digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                    delay(500);
                    Check_Red_Switch();                                               // Reset will restart the processor so no return
                }
            }
        }
        value_status = true;                                                    // set the value status to true, which is true at this point
        while (RS485_Port.available()) {                                        // Data is available so add them to Result
            received_character = RS485_Port.read();                             // take the character
            switch (pointer) {                                                  // check the received character is correct at this point
            case 0: {
                if (received_character != 2) {
                    value_status = false;                                       // first byte should be a 2, used as sync character
                }
                value[0] = received_character;
                break;
            }
            case 1: {
                if (received_character != 3) {
                    value_status = false;                                       // second byte should be a 3
                }
                value[1] = received_character;
                break;
            }
            case 2: {
                if (received_character == 2) {                                  // third byte should be 2 or 4, which indicates the length of valueters
                    required_bytes = 7;                                         // total of 7 bytes required       
                }
                else if (received_character == 4) {
                    required_bytes = 9;                                         // total of 9 bytes required
                }
                else {
                    value_status = false;                                       // if not a 2 or 4 value is bad
                }
                value[2] = received_character;
                break;
            }
            case 3: {
                value[3] = received_character;                             // received characters 3 to (position == required_bytes) are part of the value
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
            default: {                                                          // throw other characters away
            }
            }
            if (value_status == true) pointer++;                               // increment the inpointer
        }
    } while (pointer < required_bytes);                                        // loop until all the required characters have been received, or timeout
    if (value_status == true) {                                                 // received value is good
        switch (field) {
        case Voltage: {                                                                                 // Voltage
            double volts = (value[3] << 8) + value[4];                                                  // Received is number of tenths of volt
            volts = volts / (double)10;                                                                 // convert to v           
            return volts;                                                                               // Voltage output format double
            break;
        }
        case Amperage: {                                                                                // Amperage
            double amperage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);       // Received is number of milli amps 
            amperage = amperage / (double)1000;                                                         // convert to amps 0.00n
            return amperage;                                                                            // Amperage output format double nn.n
            break;
        }
        case Wattage: {                                                                                 // Wattage
            double wattage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);        // Recieved is number of tenths of watts
            wattage = wattage / (double)10;                                                             // convert to watts
            return wattage;
            break;
        }
        case UpTime: {
            double uptime = (value[3] << 8) + value[4];
            uptime = uptime / (double)60;                                                              // convert to hours
            return uptime;
            break;
        }
        case Kilowatthour: {
            double kilowatthour = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);
            kilowatthour = kilowatthour / (double)1000;
            return kilowatthour;
            break;
        }
        case PowerFactor: {
            double powerfactor = (value[3] << 8) + value[4];
            powerfactor = powerfactor / (double)100;
            return powerfactor;
            break;
        }
        case Unknown: {
            double unknown = (value[3] << 8) + value[4];
            return unknown;
            break;
        }
        case Frequency: {
            double frequency = (value[3] * 256) + value[4];
            frequency = frequency / (double)10;
            return frequency;
            break;
        }
        case Temperature: {
            double temperature = (value[3] << 8) + value[4];
            return temperature;
            break;
        }
        default: {
        }
        }
    }
    return (double)0;
}
void Check_Green_Switch() {
    Green_Switch.update();
    if (Green_Switch.fell()) {
        console_message = "Green Button Pressed - No Action Assigned";
        Write_Console_Message();
    }
}
void Check_Blue_Switch() {
    char Running_led_state = 0;
    char SD_led_state = 0;
    Blue_Switch.update();                                                   // update wipe switch
    if (Blue_Switch.fell()) {
        console_message = "Blue Button Pressed";
        Write_Console_Message();
        Running_led_state = digitalRead(Running_led_pin);                   // save the current state of the leds
        SD_led_state = digitalRead(SD_Active_led_pin);
        do {
            digitalWrite(Running_led_pin, HIGH);                            // turn the run led on
            digitalWrite(SD_Active_led_pin, LOW);                           // turn the sd led off
            Yellow_Switch.update();
            if (Yellow_Switch.fell()) Yellow_Switch_Pressed = true;         // Blue switch + Yellow switch = wipe directory
            Red_Switch.update();                                            // reset will cancel the Wipe
            if (Red_Switch.fell()) {
                console_message = "Red Button Pressed";
                Write_Console_Message();
                ESP.restart();
            }
            delay(150);
            digitalWrite(Running_led_pin, LOW);                             // turn the run led off
            digitalWrite(SD_Active_led_pin, HIGH);                          // turn the sd led on
            delay(150);
        } while (!Yellow_Switch_Pressed);
        if (Yellow_Switch_Pressed) {
            console_message = "Yellow Button Pressed";
            Write_Console_Message();
            digitalWrite(SD_Active_led_pin, HIGH);
            console_message = "Wiping Files";
            Write_Console_Message();
            Wipe_Files();                                                   // delete all files on the SD, ReYellow_Switch_Pressed when compete
        }
        digitalWrite(Running_led_pin, Running_led_state);                   // restore the previous state of the leds
        digitalWrite(SD_Active_led_pin, SD_led_state);
    }
}
void Drive_Running_Led() {
    digitalWrite(Running_led_pin, HIGH);
}
void Check_Red_Switch() {
    Red_Switch.update();
    if (Red_Switch.fell()) {
        console_message = "Red Button Pressed";
        Write_Console_Message();
        ESP.restart();
    }
}
void Check_Yellow_Switch() {
    Yellow_Switch.update();
    if (Yellow_Switch.fell()) {
        console_message = "Yellow Button Pressed";
        Write_Console_Message();
        Yellow_Switch_Pressed = true;
    }
    if (Yellow_Switch.rose()) {
        console_message = "Yellow Button Released";
        Write_Console_Message();
        Yellow_Switch_Pressed = false;
    }
    return;
}
void Clear_Arrays() {                                           // clear the web arrays of old records
    for (int x = 0; x < console_table_size; x++) {
        console_table[x].ldate[0] = '0';
        console_table[x].ltime[0] = '0';
        console_table[x].message[0] = '0';
        console_table[x].milliseconds = 0;
    }
    for (int x = 0; x < table_size; x++) {
        readings_table[x].ldate[0] = '0';
        readings_table[x].ltime[0] = '0';
        readings_table[x].amperage = 0;
        readings_table[x].frequency = 0;
        readings_table[x].kilowatthour = 0;
        readings_table[x].powerfactor = 0;
        readings_table[x].temperature = 0;
        readings_table[x].unknown = 0;
        readings_table[x].uptime = 0;
        readings_table[x].voltage = 0;
        readings_table[x].wattage = 0;
    }
}
void SD_Led_Flash_Start_Stop(bool state) {
    if (state) {
        digitalWrite(SD_Active_led_pin, HIGH);                  // turn the led on (flash)
        sd_on_time = millis();                                  // set the start time (immediate)
        sd_off_time = millis() + (unsigned long)300;            // set the stoptime (start + period)
    }
    else {
        digitalWrite(SD_Active_led_pin, LOW);                   // turn the led off
    }
}
void Flash_SD_LED() {
    if (millis() > sd_on_time && millis() < sd_off_time) {      // turn the led on (flash)
        digitalWrite(SD_Active_led_pin, HIGH);
    }
    else {
        digitalWrite(SD_Active_led_pin, LOW);
    }
    if (millis() >= (sd_off_time)) {                             // turn the led on (flash)
        sd_on_time = millis() + (unsigned long)300;                // set the next on time (flash)
        sd_off_time = millis() + (unsigned long)600;
    }
}
String GetDate(bool format) {
    int connection_attempts = 0;
    while (!getLocalTime(&timeinfo)) {
        console_message = "Attempting to Get Date " + String(connection_attempts);
        Write_Console_Message();
        delay(500);
        connection_attempts++;
        if (connection_attempts > 20) {
            console_message = "Time Network Error, Restarting";
            Write_Console_Message();
            ESP.restart();
        }
    }
    This_Year = (String)(timeinfo.tm_year + 1900);
    This_Month = (String)(timeinfo.tm_mon + 1);
    if (This_Month.length() < 2) This_Month = "0" + This_Month;
    This_Day = (String)timeinfo.tm_mday;
    if (This_Day.length() < 2) This_Day = "0" + This_Day;
    String date_str;
    if (!format) {                                               // if format = 0 then output raw date ddmmyyyy
        date_str = This_Year + This_Month + This_Day;
    }
    else {                                                      // if format = 1 then output formatted date dd/mm/yyyy
        date_str = This_Year + "/" + This_Month + "/" + This_Day;
    }
    return date_str;
}
String GetTime(bool format) {
    int connection_attempts = 0;
    while (!getLocalTime(&timeinfo)) {
        console_message = "Attempting to Get Time " + String(connection_attempts);
        Write_Console_Message();
        delay(500);
        connection_attempts++;
        if (connection_attempts > 20) {
            console_message = "Time Network Error, Restarting";
            Write_Console_Message();
            ESP.restart();
        }
    }
    This_Hour = (String)timeinfo.tm_hour;
    if (This_Hour.length() < 2) This_Hour = "0" + This_Hour;
    This_Minute = (String)timeinfo.tm_min;
    if (This_Minute.length() < 2) This_Minute = "0" + This_Minute;
    This_Second = (String)timeinfo.tm_sec;
    if (This_Second.length() < 2) This_Second = "0" + This_Second;
    String time_str;
    if (format) {
        time_str = This_Hour + ":" + This_Minute + ":" + This_Second;
    }
    else {
        time_str = This_Hour + This_Minute + This_Second;
    }
    return time_str;
}
void printDouble(double val, byte precision) {
    // prints val with number of decimal places determine by precision
    // precision is a number from 0 to 6 indicating the desired decimial places
    // example: printDouble( 3.1415, 2); // prints 3.14 (two decimal places)

    console_message = String(val);  //prints the int part
    if (precision > 0) {
        console_message += ("."); // print the decimal point
        unsigned long frac;
        unsigned long mult = 1;
        byte padding = precision - 1;
        while (precision--)
            mult *= 10;

        if (val >= 0)
            frac = (val - int(val)) * mult;
        else
            frac = (int(val) - val) * mult;
        unsigned long frac1 = frac;
        while (frac1 /= 10)
            padding--;
        while (padding--)
            console_message += "0";
        console_message += String(frac);
    }
    Write_Console_Message();
}
void SDprintDouble(double val, byte precision) {
    Datafile.print(int(val));  //prints the int part
    if (precision > 0) {
        Datafile.print("."); // print the decimal point
        unsigned long frac;
        unsigned long mult = 1;
        byte padding = precision - 1;
        while (precision--)
            mult *= 10;

        if (val >= 0)
            frac = (val - int(val)) * mult;
        else
            frac = (int(val) - val) * mult;
        unsigned long frac1 = frac;
        while (frac1 /= 10)
            padding--;
        while (padding--)
            Datafile.print("0");
        Datafile.print(frac, DEC);
    }
}
void Parse_Weather_Info(String payload) {
    //   console.println(payload);
       // Temperature ----------------------------------------------------------------------------------------------------
    int temp_start = payload.indexOf("temp\":");                        // "temp":272.77,
    temp_start = payload.indexOf(":", temp_start);
    int temp_end = payload.indexOf(",", temp_start);
    parse(payload, temp_start, temp_end);
    weather_record.temp = (double)(atof(Parse_Output)) - (double)273.15;
    // Pressure -------------------------------------------------------------------------------------------------------
    int pressure_start = payload.indexOf("pressure\":");        // "pressure":1007,
    pressure_start = payload.indexOf(":", pressure_start);
    int pressure_end = payload.indexOf(",", pressure_start);
    parse(payload, pressure_start, pressure_end);
    weather_record.pressure = (int)atof(Parse_Output);
    // humidity -------------------------------------------------------------------------------------------------------
    int humidity_start = payload.indexOf("humidity\":");        // "humidity":95}
    humidity_start = payload.indexOf(":", humidity_start);
    int humidity_end = payload.indexOf("}", humidity_start);
    parse(payload, humidity_start, humidity_end);
    weather_record.humidity = (int)atof(Parse_Output);
    // weather description --------------------------------------------------------------------------------------------
    int weather_start = payload.indexOf("main");            // "weather" : [{"id":701, "main" : "Mist", "description":"mist", "icon" : "50n"}] ,
    weather_start = payload.indexOf(":", weather_start);            // "description":"clear sky","icon"
    weather_start = weather_start + 2;
    int weather_end = payload.indexOf("\"", weather_start);
    parse(payload, weather_start - 1, weather_end);
    weather_record.weather = String(Parse_Output);
    // wind speed -----------------------------------------------------------------------------------------------------
    int wins_start = payload.indexOf("speed");                       // "speed":2.57,
    wins_start = payload.indexOf(":", wins_start);
    int wins_end = payload.indexOf(",", wins_start);
    parse(payload, wins_start, wins_end);
    weather_record.wind_speed = (double)(atof(Parse_Output));
    // wind direction -------------------------------------------------------------------------------------------------
    int wind_start = payload.indexOf("deg");                         // "deg":20
    wind_start = payload.indexOf(":", wind_start);
    int wind_end = payload.indexOf("}", wind_start);
    parse(payload, wind_start, wind_end);
    weather_record.wind_direction = (int)atof(Parse_Output);
#ifdef DISPLAY_WEATHER_INFORMATION
    console.println(payload);
    console.print("Parsed Temperature: "); console.println(weather_record.temp, DEC);
    console.print("Parsed Pressure: "); console.println(weather_record.pressure, DEC);
    console.print("Parsed Humidity: "); console.println(weather_record.humidity, DEC);
    console.print("Parsed Wind Direction: "); console.println(weather_record.wind_direction, DEC);
    console.print("Parsed Wind Speed: "); console.println(weather_record.wind_speed, DEC);
    console.print("Parsed Description: "); console.println(weather_record.weather);
#endif
}
void parse(String payload, int start, int end) {
    int ptr = 0;
    for (int pos = start + 1; pos < end; pos++) {
        Parse_Output[ptr++] = payload[pos];
    }
    Parse_Output[ptr] = '\0';
}