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
16/12/2022  9.5 Add more information to information web page
17/12/2022  9.6 Corrected issue with Delete Files, where operational files were displayed after file removed
20/12/2022  9.7 Corrected an issue where if the date changed new file creation would be continuously repeated
22/12/2022  9.8 Web data arrays cleared when date changed, SD flashes during Preloading
23/12/2022  9.9 Requirement to press start button removed
28/12/2022  9.10 Attempt to recover network if lost (up to 20 attempts = 10 seconds
30/12/2022  9.11 Added autoscaling to web chart display
06/01/2023  10.0 Radical rewrite of Data handling
*/
String version = "V10.0";                       // software version number, shown on webpage
// compiler directives ------------------------------------------------------------------------------------------------
//#define ALLOW_WORKING_FILE_DELETION           // allows the user to chose to delete the day's working files
//#define DISPLAY_DATA_VALUES_COLLECTED         // print the data values as they are collected
//#define DISPLAY_DATA_VALUES_WRITTEN           // print the data values as they written to sd drive
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
constexpr int Voltage = 2;
constexpr int Amperage = 3;
constexpr int Wattage = 4;
constexpr int UpTime = 5;
constexpr int Kilowatthour = 6;
constexpr int PowerFactor = 7;
constexpr int Unknown = 8;
constexpr int Frequency = 9;
constexpr int Sensor_Temperature = 10;
constexpr int Weather_Temperature = 11;
constexpr int Temperature_Feels_Like = 12;
constexpr int Temperature_Maximum = 13;
constexpr int Temperature_Minimum = 14;
constexpr int Atmospheric_Pressure = 15;
constexpr int Relative_Humidity = 16;
constexpr int Wind_Speed = 17;
constexpr int Wind_Direction = 18;
constexpr int Weather_Description = 19;
// --------------------------------------------------------------------------------------------------------------------
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
String Data_File_Field_Names[20] = {
                        "Date",                     // [0]
                        "Time",                     // [1]
                        "Voltage",                  // [2]
                        "Amperage",                 // [3]
                        "Wattage",                  // [4]
                        "Up Time",                  // [5]
                        "kiloWattHours",            // [6]
                        "Power Factor",             // [7]
                        "Unknown",                  // [8]
                        "Frequency",                // [9]
                        "Sensor Temperature",       // [10]
                        "Weather Temperature",      // [11]
                        "Temperature_Feels_Like",   // [12]
                        "Temperature Maximum",      // [13]
                        "Temperature Minimum",      // [14]
                        "Atmospheric Pressure",     // [15]
                        "Relative Humidity",        // [16]
                        "Wind Speed",               // [17]
                        "Wind Direction",           // [18]
                        "Weather Description"       // [19]
};
String FileNames[50];
String Data_File_Values_0 = "1951/11/18";       // Date
String Data_File_Values_1 = "00:00:00";         // Time;
double Data_File_Values[20] = {
                        0.0,            // [0]  dummy date 
                        0.0,            // [1]  dummy time
                        0.0,            // [2]  voltage
                        0.0,            // [3]  amperage
                        0.0,            // [4]  wattage
                        0.0,            // [5]  up time
                        0.0,            // [6]  kilowatthours
                        0.0,            // [7]  power factor
                        0.0,            // [8]  unknown
                        0.0,            // [9]  frequency
                        0.0,            // [10] sensor temperature
                        0.0,            // [11] weather temperature
                        0.0,            // [12] temperature feels like
                        0.0,            // [13] temperature Maximum
                        0.0,            // [14] temperature Minimum
                        0.0,            // [15] atmospheric pressure
                        0.0,            // [16] relative humidity
                        0.0,            // [17] wind speed
                        0.0,            // [18] wind direction
                        0.0             // [19] weather description
};
String Data_File_Values_19 = "                                   ";
bool Yellow_Switch_Pressed = false;
String site_width = "1060"; // "1060";          // width of web page
String site_height = "600";                     // height of web page
constexpr int data_table_size = 59;             // number of data table rows (5 minutes worth)
constexpr int console_table_size = 19;          // number of lines to display on debug web page
int Global_Data_Table_Pointer = 0;              // points to the next index of Data_Table
int Global_Data_Record_Count = 0;               // running total of records written to Data Table
int Global_Console_Table_Pointer = 0;           // points to the next index of Console Table
int Global_Console_Record_Count = 0;            // running total of records writtem tp Console Table
String webpage;
String lastcall;
double temperature_calibration = (double)16.5 / (double)22.0;   // temperature reading = 22, actual temperature = 16.5
String Last_Boot_Time = "12:12:12";
String Last_Boot_Date = "2022/29/12";
typedef struct {
    char ldate[11];                 // [0]  date record was taken
    char ltime[9];                  // [1]  time record was taken
    double voltage;                 // [2]
    double amperage;                // [3]
    double wattage;                 // [4]
    double uptime;                  // [5]
    double kilowatthour;            // [6]
    double powerfactor;             // [7]
    double unknown;                 // [8]  always the same value
    double frequency;               // [9]
    double sensor_temperature;      // [10]
    double weather_temperature;     // [11]
    double temperature_feels_like;  // [12]
    double temperature_maximum;     // [13]
    double temperature_minimum;     // [14]
    double atmospheric_pressure;    // [15]
    double relative_humidity;       // [16]
    double wind_speed;              // [17]
    double wind_direction;          // [18]
    char weather_description[20];   // [19]
} record_type;
record_type readings_table[data_table_size + 1];
// Lowest Voltage -----------------------------------------------------------------------------------------------------
double lowest_voltage = 0;
String time_of_lowest_voltage = "00:00:00";
// Highest Voltage ----------------------------------------------------------------------------------------------------
double highest_voltage = 0;
String time_of_highest_voltage = "00:00:00";
// Highest amperage ---------------------------------------------------------------------------------------------------
double highest_amperage = 0;
String time_of_highest_amperage = "00:00:00";
// Lowest Temperature -------------------------------------------------------------------------------------------------
double lowest_temperature = 0;
String time_of_lowest_temperature = "00:00:00";
// Highest Temperature ------------------------------------------------------------------------------------------------
double highest_temperature = 0;
String time_of_highest_temperature = "00:00:00";
// Time of Latest Weather Reading -------------------------------------------------------------------------------------
String time_of_latest_reading = "00:00:00";
// Latest Weather Temperature -----------------------------------------------------------------------------------------
double latest_weather_temperature = 0;
// Latest Temperature Feels Like --------------------------------------------------------------------------------------
double latest_weather_temperature_feels_like = 0;
// Latest Temperature Maximum -----------------------------------------------------------------------------------------
double latest_weather_temperature_maximum = 0;
// Latest Temperature Minimum -----------------------------------------------------------------------------------------
double latest_weather_temperature_minimum = 0;
// Latest Atmospheric Pressure ----------------------------------------------------------------------------------------
double latest_atmospheric_pressure = 0;
// Latest Relative Humidity -------------------------------------------------------------------------------------------
double latest_relative_humidity = 0;
// Latest Wind Speed --------------------------------------------------------------------------------------------------
double latest_wind_speed = 0;
// Latest Wind Direction ----------------------------------------------------------------------------------------------
double latest_wind_direction = 0;
// Latest Weather Description -----------------------------------------------------------------------------------------
String latest_weather_description = "                                     ";
// --------------------------------------------------------------------------------------------------------------------
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
unsigned long last_cycle = 0;
unsigned long last_weather_read = 0;
uint64_t SD_freespace = 0;
uint64_t critical_SD_freespace = 0;
double SD_freespace_double = 0;
String temp_message;
//String console_message = "                                                                    ";
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
    Write_Console_Message("Booting - Commencing Setup");
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
    Write_Console_Message("Starting Server");
    server.begin();                                 // Start Webserver
    Write_Console_Message("Server Started");
    server.on("/", Display);                        // nothing specified so display main web page
    server.on("/Display", Display);                 // display the main web page
    server.on("/Information", Information);         // display information
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
        Write_Console_Message("SD Drive Begin Failed");
        while (true) {
            digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
            digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
            delay(500);
            Check_Red_Switch();
        }
    }
    else {
        Write_Console_Message("SD Drive Begin Succeeded");
        uint8_t cardType = SD.cardType();
        while (SD.cardType() == CARD_NONE) {
            Write_Console_Message("No SD Card Found");
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
        Write_Console_Message("SD Card Type: " + card);
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        critical_SD_freespace = cardSize * (uint64_t).9;
        Write_Console_Message("SD Card Size : " + String(cardSize) + "MBytes");
        Write_Console_Message("SD Total Bytes : " + String(SD.totalBytes()));
        Write_Console_Message("SD Used bytes : " + String(SD.usedBytes()));
        Write_Console_Message("SD Card Initialisation Complete");
        Write_Console_Message("Create Console Logging File");
    }
    Last_Boot_Time = GetTime(true);
    Last_Boot_Date = GetDate(true);
    This_Date = GetDate(false);
    Create_New_Data_File();
    Create_New_Console_File();
    Write_Console_Message("End of Setup");
    Write_Console_Message("Running in Full Function Mode");
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
                Write_Console_Message("Obtaining Weather Information Failed, Return code: " + String(httpCode));
            }
            http.end();
        }
        // weather end --------------------------------------------------------------------------------------------
        // sensor start -------------------------------------------------------------------------------------------
        for (int i = 0; i < 9; i++) {                           // transmit the requests, assembling the Values array
            Send_Request(i);                                    // send the RS485 Port the requests, one by one
            Receive(i);                                         // receive the sensor output
        }                                                       // all values should now be populated
        // sensor end ---------------------------------------------------------------------------------------------
        Data_File_Values_0 = GetDate(true);                     // get the date of the reading
        Data_File_Values_1 = GetTime(true);                     // get the time of the reading
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
}                                                                   // end of loop
void Write_New_Data_Record_to_Data_File() {
    digitalWrite(SD_Active_led_pin, HIGH);                          // turn the SD activity LED on
    Datafile = SD.open("/" + DataFileName, FILE_APPEND);            // open the SD file
    if (!Datafile) {                                                // oops - file not available!
        Write_Console_Message("Error re-opening Datafile:" + String(DataFileName));
        while (true) {
            digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
            digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
            delay(500);
            Check_Red_Switch();                               // Reset will restart the processor so no return
        }
    }
    SD_freespace = (SD.totalBytes() - SD.usedBytes());
    console.print(millis(), DEC); console.println("\tNew Data Record Written");
    Datafile.print(Data_File_Values_0); Datafile.print(",");                        // [0] dd/mm/yyyy,
    Datafile.print(Data_File_Values_1); Datafile.print(",");                        // [1] hh:mm:ss,
    SDprintDouble(Data_File_Values[Voltage], 1); Datafile.print(",");               // [2] voltage
    SDprintDouble(Data_File_Values[Amperage], 3); Datafile.print(",");              // [3] amperage
    SDprintDouble(Data_File_Values[Wattage], 2); Datafile.print(",");               // [4] wattage
    SDprintDouble(Data_File_Values[UpTime], 2); Datafile.print(",");                // [5] up time
    SDprintDouble(Data_File_Values[Kilowatthour], 3); Datafile.print(",");          // [6] kilowatt hour
    SDprintDouble(Data_File_Values[PowerFactor], 2); Datafile.print(",");           // [7] power factor
    SDprintDouble(Data_File_Values[Unknown], 1); Datafile.print(",");               // [8] unknown
    SDprintDouble(Data_File_Values[Frequency], 1); Datafile.print(",");             // [9] frequency
    SDprintDouble(Data_File_Values[Sensor_Temperature], 1); Datafile.print(",");    // [10] sensor temperature
    SDprintDouble(Data_File_Values[Weather_Temperature], 2); Datafile.print(",");   // [11] weather temperature
    SDprintDouble(Data_File_Values[Temperature_Feels_Like], 2); Datafile.print(",");// [12] temperature feels like
    SDprintDouble(Data_File_Values[Temperature_Maximum], 2); Datafile.print(",");   // [13] temperature maximum
    SDprintDouble(Data_File_Values[Temperature_Minimum], 2); Datafile.print(",");   // [14] temperature minimum
    SDprintDouble(Data_File_Values[Atmospheric_Pressure], 0); Datafile.print(",");  // [15] atmospheric pressure
    SDprintDouble(Data_File_Values[Relative_Humidity], 0); Datafile.print(",");     // [16] relative humidity
    SDprintDouble(Data_File_Values[Wind_Speed], 0); Datafile.print(",");            // [17] wind direction
    SDprintDouble(Data_File_Values[Wind_Direction], 0); Datafile.print(",");        // [18] wind speed
    Datafile.print(Data_File_Values_19);                                           // [19] weather description,
    Datafile.print("\n");                           // end of record
    Datafile.close();                               // close the sd file
    Datafile.flush();                               // make sure it has been written to SD
    Global_Data_Table_Pointer++;                    // increment the record count, the array pointer
    Global_Data_Record_Count++;                     // increment the current record count
    digitalWrite(SD_Active_led_pin, LOW);           // turn the SD activity LED on
    Update_Webpage_Variables_from_Data_File_Values(Data_File_Values);
#ifdef DISPLAY_DATA_VALUES_WRITTEN
    console.print(millis(), DEC); console.println("\tData Values Written:");
    console.print("\t\tDate: "); console.println(Data_File_Values_0);                                                                   // [0] date
    console.print("\t\tTime: "); console.println(Data_File_Values_1);                                                                   // [1] time
    console.print("\t\tVoltage: "); printConsoleDouble(Data_File_Values[Voltage], 6); console.println();                                // [2] voltage
    console.print("\t\tAmperage: "); printConsoleDouble(Data_File_Values[Amperage], 6); console.println();                              // [3] amperage
    console.print("\t\tWattage: "); printConsoleDouble(Data_File_Values[Wattage], 6); console.println();                                // [4] wattage
    console.print("\t\tUpTime: "); printConsoleDouble(Data_File_Values[UpTime], 6); console.println();                                  // [5] up time
    console.print("\t\tKilowatthour: "); printConsoleDouble(Data_File_Values[Kilowatthour], 6); console.println();                      // [6] kilowatt hour
    console.print("\t\tPower Factor: "); printConsoleDouble(Data_File_Values[PowerFactor], 6); console.println();                       // [7] power factor
    console.print("\t\tUnknown: "); printConsoleDouble(Data_File_Values[Unknown], 6); console.println();                                // [8] unknown
    console.print("\t\tFrequency: "); printConsoleDouble(Data_File_Values[Frequency], 6); console.println();                            // [9] frequency
    console.print("\t\tSensor Temperature: "); printConsoleDouble(Data_File_Values[Sensor_Temperature], 6); console.println();          // [10] sensor temperature
    console.print("\t\tWeather Temperature: "); printConsoleDouble(Data_File_Values[Weather_Temperature], 6); console.println();        // [11] weather temperature
    console.print("\t\tTemperature Feels Like: "); printConsoleDouble(Data_File_Values[Temperature_Feels_Like], 6); console.println();  // [12] temperature feels like
    console.print("\t\tTemperature Maximum: "); printConsoleDouble(Data_File_Values[Temperature_Maximum], 6); console.println();        // [13] temperature maximum
    console.print("\t\tTemperature Minimum: "); printConsoleDouble(Data_File_Values[Temperature_Minimum], 6); console.println();        // [14] temperature minimum
    console.print("\t\tAtmospheric Pressure: "); printConsoleDouble(Data_File_Values[Atmospheric_Pressure], 6); console.println();      // [15] atmospheric pressure
    console.print("\t\tRelative Humidity: "); printConsoleDouble(Data_File_Values[Relative_Humidity], 6); console.println();            // [16] relative humidity
    console.print("\t\tWind Speed: "); printConsoleDouble(Data_File_Values[Wind_Speed], 6); console.println();                          // [17] wind direction
    console.print("\t\tWind Direction: "); printConsoleDouble(Data_File_Values[Wind_Direction], 6); console.println();                  // [18] wind speed
    console.print("\t\tWeather Description"); console.println(Data_File_Values_19);                                                     // [19] weather description
#endif
}
void Add_New_Data_Record_to_Display_Table() {
    if (Global_Data_Table_Pointer > data_table_size) {                                                   // table full, shuffle fifo
        for (i = 0; i < data_table_size; i++) {                                                          // shuffle the rows up, losing row 0, make row [table_size] free
            strncpy(readings_table[i].ldate, readings_table[i + 1].ldate, sizeof(readings_table[i].ldate)); // [0]  date
            strncpy(readings_table[i].ltime, readings_table[i + 1].ltime, sizeof(readings_table[i].ltime)); // [1]  time
            readings_table[i].voltage = readings_table[i + 1].voltage;                                      // [2]  voltage
            readings_table[i].amperage = readings_table[i + 1].amperage;                                    // [3]  amperage
            readings_table[i].wattage = readings_table[i + 1].wattage;                                      // [4]  wattage
            readings_table[i].uptime = readings_table[i + 1].uptime;                                        // [5]  up time
            readings_table[i].kilowatthour = readings_table[i + 1].kilowatthour;                            // [6]  kilowatt hour
            readings_table[i].powerfactor = readings_table[i + 1].powerfactor;                              // [7]  power factor
            readings_table[i].unknown = readings_table[i + 1].unknown;                                      // [8]  unknown
            readings_table[i].frequency = readings_table[i + 1].frequency;                                  // [9]  frequency
            readings_table[i].sensor_temperature = readings_table[i + 1].sensor_temperature;                // [10] sensor temperature
            readings_table[i].weather_temperature = readings_table[i + 1].weather_temperature;              // [11] weather temperature
            readings_table[i].temperature_feels_like = readings_table[i + 1].temperature_feels_like;        // [12] temperature
            readings_table[i].temperature_maximum = readings_table[i + 1].temperature_maximum;              // [13] temperature maximum
            readings_table[i].temperature_minimum = readings_table[i + 1].temperature_minimum;              // [14] temperature minimum
            readings_table[i].atmospheric_pressure = readings_table[i + 1].atmospheric_pressure;            // [15] atmospheric pressure
            readings_table[i].relative_humidity = readings_table[i + 1].relative_humidity;                  // [16] relative humidity
            readings_table[i].wind_speed = readings_table[i + 1].wind_speed;                                // [17] wind speed
            readings_table[i].wind_direction = readings_table[i + 1].wind_direction;                        // [18] wind direction
            strncpy(readings_table[i].weather_description, readings_table[i + 1].weather_description, sizeof(readings_table[i].weather_description));// [19] weather description
        }
        Global_Data_Table_Pointer = data_table_size;                                                             // subsequent records will be added at the end of the table
        strncpy(readings_table[data_table_size].ldate, Data_File_Values_0.c_str(), sizeof(readings_table[data_table_size].ldate));                       // [0]  date
        strncpy(readings_table[data_table_size].ltime, Data_File_Values_1.c_str(), sizeof(readings_table[data_table_size].ltime));                       // [1]  time
        readings_table[data_table_size].voltage = Data_File_Values[Voltage];                             // [2]  voltage
        readings_table[data_table_size].amperage = Data_File_Values[Amperage];                           // [3]  amperage
        readings_table[data_table_size].wattage = Data_File_Values[Wattage];                             // [4]  wattage
        readings_table[data_table_size].uptime = Data_File_Values[UpTime];                               // [5]  uptime
        readings_table[data_table_size].kilowatthour = Data_File_Values[Kilowatthour];                   // [6]  kilowatthours
        readings_table[data_table_size].powerfactor = Data_File_Values[PowerFactor];                     // [7]  power factor
        readings_table[data_table_size].unknown = Data_File_Values[Unknown];                             // [8]  unknown
        readings_table[data_table_size].frequency = Data_File_Values[Frequency];                         // [9]  frequency
        readings_table[data_table_size].sensor_temperature = Data_File_Values[Sensor_Temperature];       // [10] sensor temperature
        readings_table[data_table_size].weather_temperature = Data_File_Values[Weather_Temperature];     // [11] weather temperature
        readings_table[data_table_size].temperature_feels_like = Data_File_Values[Temperature_Feels_Like]; // [12] temperature feels like
        readings_table[data_table_size].temperature_maximum = Data_File_Values[Temperature_Maximum];     // [13] temperature maximum
        readings_table[data_table_size].temperature_minimum = Data_File_Values[Temperature_Minimum];     // [14] temperature minimum
        readings_table[data_table_size].atmospheric_pressure = Data_File_Values[Atmospheric_Pressure];   // [15] atmospheric pressure
        readings_table[data_table_size].relative_humidity = Data_File_Values[Relative_Humidity];         // [16] relative humidity
        readings_table[data_table_size].wind_speed = Data_File_Values[Wind_Speed];                       // [17] wind speed
        readings_table[data_table_size].weather_temperature = Data_File_Values[Weather_Temperature];     // [18] wind direction
        strncpy(readings_table[data_table_size].weather_description, Data_File_Values_19.c_str(), sizeof(readings_table[data_table_size].weather_description));        // [19] weather description
    }
    else {                                                                          // add the record to the table
        strncpy(readings_table[Global_Data_Table_Pointer].ldate, Data_File_Values_0.c_str(), sizeof(readings_table[Global_Data_Table_Pointer].ldate));                       // [0]  date
        strncpy(readings_table[Global_Data_Table_Pointer].ltime, Data_File_Values_1.c_str(), sizeof(readings_table[Global_Data_Table_Pointer].ltime));                       // [1]  time
        readings_table[Global_Data_Table_Pointer].voltage = Data_File_Values[Voltage];                             // [2]  voltage
        readings_table[Global_Data_Table_Pointer].amperage = Data_File_Values[Amperage];                           // [3]  amperage
        readings_table[Global_Data_Table_Pointer].wattage = Data_File_Values[Wattage];                             // [4]  wattage
        readings_table[Global_Data_Table_Pointer].uptime = Data_File_Values[UpTime];                               // [5]  uptime
        readings_table[Global_Data_Table_Pointer].kilowatthour = Data_File_Values[Kilowatthour];                   // [6]  kilowatthours
        readings_table[Global_Data_Table_Pointer].powerfactor = Data_File_Values[PowerFactor];                     // [7]  power factor
        readings_table[Global_Data_Table_Pointer].unknown = Data_File_Values[Unknown];                             // [8]  unknown
        readings_table[Global_Data_Table_Pointer].frequency = Data_File_Values[Frequency];                         // [9]  frequency
        readings_table[Global_Data_Table_Pointer].sensor_temperature = Data_File_Values[Sensor_Temperature];       // [10] sensor temperature
        readings_table[Global_Data_Table_Pointer].weather_temperature = Data_File_Values[Weather_Temperature];     // [11] weather temperature
        readings_table[Global_Data_Table_Pointer].temperature_feels_like = Data_File_Values[Temperature_Feels_Like]; // [12] temperature feels like
        readings_table[Global_Data_Table_Pointer].temperature_maximum = Data_File_Values[Temperature_Maximum];     // [13] temperature maximum
        readings_table[Global_Data_Table_Pointer].temperature_minimum = Data_File_Values[Temperature_Minimum];     // [14] temperature minimum
        readings_table[Global_Data_Table_Pointer].atmospheric_pressure = Data_File_Values[Atmospheric_Pressure];   // [15] atmospheric pressure
        readings_table[Global_Data_Table_Pointer].relative_humidity = Data_File_Values[Relative_Humidity];         // [16] relative humidity
        readings_table[Global_Data_Table_Pointer].wind_speed = Data_File_Values[Wind_Speed];                       // [17] wind speed
        readings_table[Global_Data_Table_Pointer].weather_temperature = Data_File_Values[Weather_Temperature];     // [18] wind direction
        strncpy(readings_table[Global_Data_Table_Pointer].weather_description, Data_File_Values_19.c_str(), sizeof(readings_table[Global_Data_Table_Pointer].weather_description));        // [19] weather description
    }
}
void Create_New_Console_File() {
    digitalWrite(SD_Active_led_pin, HIGH);                          // turn the SD activity LED on
    ConsoleFileName = GetDate(false) + ".txt";                      // yes, so create a new file
    if (!SD.exists("/" + ConsoleFileName)) {
        Consolefile = SD.open("/" + ConsoleFileName, FILE_WRITE);
        if (!Consolefile) {                                         // log file not opened
            Write_Console_Message("Error opening Console file: [" + String(ConsoleFileName) + "]");
            while (true) {
                digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                delay(500);
                Check_Red_Switch();
            }
        }

        Consolefile.println(GetDate(true) + "," + GetTime(true) + "," + String(millis()) + ",Console File Started");
        Consolefile.close();
        Consolefile.flush();
        Global_Console_Table_Pointer = 1;
    }
    else {
        Write_Console_Message("Console File " + String(ConsoleFileName) + " already exists");
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
            Write_Console_Message("Error opening Data file @ line 324 [" + String(DataFileName) + "]");
            while (true) {
                digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                delay(500);
                Check_Red_Switch();
            }
        }
        for (int x = 0; x < 19; x++) {                               // write data column headings into the SD file
            Datafile.print(Data_File_Field_Names[x]);
            Datafile.print(",");
        }
        Datafile.println(Data_File_Field_Names[19]);
        Datafile.close();
        Datafile.flush();
        Global_Data_Table_Pointer = 0;
        digitalWrite(SD_Active_led_pin, LOW);
        This_Date = GetDate(false);                                 // update the current date
        Write_Console_Message("Data File Created " + DataFileName);
    }
    else {
        Write_Console_Message("Data File " + String(DataFileName) + " already exists");
        Prefill_Array();
    }
}
void Write_Console_Message(String console_message) {
    String saved_console_message = console_message;
    String Date = "1951/18/11";
    String Time = "00:00:00";
    unsigned long milliseconds = millis();
    String pre_date = "1951/18/11";
    String pre_time = "00:00:00";
    if (Post_Setup_Status) {                                                // only write the console message to disk once setup is complete
        if (pre_loop_message_count > 0) {                                   // are there any pre loop console messages stored
            for (int x = 0; x < pre_loop_message_count; x++) {
                pre_date = GetDate(true);
                pre_time = GetTime(true);
                Write_New_Console_Message_to_Console_File(pre_date, pre_time, pre_loop_millis_values[x], pre_loop_messages[x]);
                Add_New_Console_Message_to_Console_Table(pre_date, pre_time, pre_loop_millis_values[x], pre_loop_messages[x]);
            }
            pre_loop_message_count = 0;
        }
        Date = GetDate(true);
        Time = GetTime(true);
        Write_New_Console_Message_to_Console_File(Date, Time, milliseconds, saved_console_message);
        Add_New_Console_Message_to_Console_Table(Date, Time, milliseconds, saved_console_message);
    }
    else {
        pre_loop_millis_values[pre_loop_message_count] = millis();
        pre_loop_messages[pre_loop_message_count] = console_message;
        pre_loop_message_count++;
    }
    console_message = saved_console_message;
    console.print(millis(), DEC); console.print("\t"); console.println(console_message);
}
void Write_New_Console_Message_to_Console_File(String date, String time, unsigned long milliseconds, String console_message) {
#ifdef DISPLAY_PROGRAMME_PROGRESS
    console.print(millis(), DEC); console.println("\tWrite_New_Console_Message_to_Console_File Entered");
#endif
    digitalWrite(SD_Active_led_pin, HIGH);                          // turn the SD activity LED on
    Consolefile = SD.open("/" + ConsoleFileName, FILE_APPEND);      // open the SD file
    if (!Consolefile) {                                             // oops - file not available!
        Write_Console_Message("Error re-opening Console file: " + String(ConsoleFileName));
        while (true) {
            digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
            digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
            delay(500);
            Check_Red_Switch();                                   // Reset will restart the processor so no return
        }
    }
    Consolefile.println(date + "," + time + "," + milliseconds + "," + console_message);
    Consolefile.close();                                            // close the sd file
    Consolefile.flush();                                            // make sure it has been written to SD
    SD_freespace_double = (double)SD_freespace / 1000000;
    if (SD_freespace < critical_SD_freespace) {
        for (int x = 0; x < console_message.length(); x++) {        // reserve the current console_message
            temp_message[x] = console_message[x];
        }
        Write_Console_Message("WARNING - SD Free Space critical " + String(SD_freespace) + "MBytes");
        for (int x = 0; x < temp_message.length(); x++) {            // restore the console_message
            console_message[x] = temp_message[x];
        }
    }
    digitalWrite(SD_Active_led_pin, LOW);                           // turn the SD activity LED off
}
void Add_New_Console_Message_to_Console_Table(String date, String time, unsigned long milliseconds, String console_message) {
    if (Global_Data_Table_Pointer > console_table_size) {                           // table full, shuffle fifo
        for (int i = 0; i < console_table_size; i++) {                              // shuffle the rows up, losing row 0, make row [table_size] free
            strncpy(console_table[i].ldate, console_table[i + 1].ldate, sizeof(console_table[i].ldate));             // date
            strncpy(console_table[i].ltime, console_table[i + 1].ltime, sizeof(console_table[i].ltime));             // time
            console_table[i].milliseconds = console_table[i + 1].milliseconds;
            strncpy(console_table[i].message, console_table[i + 1].message, sizeof(console_table[i].message));
        }
        Global_Console_Table_Pointer = console_table_size;               // subsequent records will be added at the end of the table
        strncpy(console_table[console_table_size].ldate, date.c_str(), sizeof(console_table[console_table_size].ldate));
        strncpy(console_table[console_table_size].ltime, time.c_str(), sizeof(console_table[console_table_size].ltime));
        console_table[console_table_size].milliseconds = milliseconds;
        strncpy(console_table[console_table_size].message, console_message.c_str(), sizeof(console_table[console_table_size].message));   // write the new message onto the end of the table
    }
    else {                                                                      // add the record to the table
        strncpy(console_table[Global_Console_Table_Pointer].ldate, date.c_str(), sizeof(console_table[Global_Console_Table_Pointer].ldate));
        strncpy(console_table[Global_Console_Table_Pointer].ltime, time.c_str(), sizeof(console_table[Global_Console_Table_Pointer].ltime));
        console_table[Global_Console_Table_Pointer].milliseconds = milliseconds;
        strncpy(console_table[Global_Console_Table_Pointer].message, console_message.c_str(), sizeof(console_table[Global_Console_Table_Pointer].message));      // write the new message onto the end of the table
    }
    Global_Console_Table_Pointer++;                                             // increment the console table pointer
    Global_Console_Record_Count++;                                              // increment the console record count
}
void Check_WiFi() {
    if (WiFi.status() != WL_CONNECTED) {                     // whilst it is not connected keep trying
        Write_Console_Message("WiFi Connection Failed, Attempting to Reconnect");
        delay(500);
        StartWiFi(ssid, password);
    }
}
int StartWiFi(const char* ssid, const char* password) {
    Write_Console_Message("WiFi Connecting to " + String(ssid));
    WiFi.disconnect(true);                                      // disconnect to set new wifi connection
    WiFi.begin(ssid, password);                                 // connect to the wifi network
    int WiFi_Status = WiFi.status();
    Write_Console_Message("WiFi Status: " + String(WiFi_Status) + " " + WiFi_Status_Message[WiFi_Status]);
    int wifi_connection_attempts = 0;                           // zero the attempt counter
    while (WiFi.status() != WL_CONNECTED) {                     // whilst it is not connected keep trying
        delay(500);
        Write_Console_Message("Connection attempt " + String(wifi_connection_attempts));
        if (wifi_connection_attempts++ > 20) {
            Write_Console_Message("Network Error, Restarting");
            ESP.restart();
        }
    }
    WiFi_Status = WiFi.status();
    Write_Console_Message("WiFi Status: " + String(WiFi_Status) + " " + WiFi_Status_Message[WiFi_Status]);
    WiFi_Signal_Strength = (int)WiFi.RSSI();
    Write_Console_Message("WiFi Signal Strength:" + String(WiFi_Signal_Strength));
    Write_Console_Message("WiFi IP Address: " + String(WiFi.localIP().toString().c_str()));
    return true;
}
void StartTime() {
    Write_Console_Message("Starting Time Server");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Write_Console_Message("Time Server Started");
}
void Prefill_Array() {
    int character_count = 0;
    char Field[25];
    int datafieldNo = 1;
    char datatemp;
    int prefill_Data_Table_Pointer = 0;
    Global_Data_Table_Pointer = 0;
    SD_Led_Flash_Start_Stop(true);                                              // start the sd led flashing
    Write_Console_Message("Loading datafile from " + String(DataFileName));
    File dataFile = SD.open("/" + DataFileName, FILE_READ);
    if (dataFile) {
        while (dataFile.available()) {                                           // throw the first row, column headers, away
            datatemp = dataFile.read();
            if (datatemp == '\n') break;
        }
        while (dataFile.available()) {                                           // do while there are data available
            Flash_SD_LED();                                                     // flash the sd led
            datatemp = dataFile.read();
            Field[character_count++] = datatemp;                            // add it to the csvfield string
            if (datatemp == ',' || datatemp == '\n') {                                  // look for end of field
                Field[character_count - 1] = '\0';                           // insert termination character where the ',' or '\n' was
                switch (datafieldNo) {
                case 1:
                    strncpy(readings_table[prefill_Data_Table_Pointer].ldate, Field, sizeof(readings_table[prefill_Data_Table_Pointer].ldate));       // Date
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array ldate from file: ");
                    //                    console.println(readings_table[Data_Table_Pointer].ldate);
                    break;
                case 2:
                    strncpy(readings_table[prefill_Data_Table_Pointer].ltime, Field, sizeof(readings_table[prefill_Data_Table_Pointer].ltime));       // Time
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array ltime from file: ");
                    //                    console.println(readings_table[Data_Table_Pointer].ltime);
                    break;
                case 3:
                    readings_table[prefill_Data_Table_Pointer].voltage = atof(Field);      // Voltage
                    //                   console.print(millis(), DEC); console.print("\tPrefill_Array voltage: ");
                    //                   console.println(readings_table[Data_Table_Pointer].voltage);
                    break;
                case 4:
                    readings_table[prefill_Data_Table_Pointer].amperage = atof(Field);     // Amperage
                    //                   console.print(millis(), DEC); console.print("\tPrefill_Array amperage: ");
                    //                   console.println(readings_table[Data_Table_Pointer].amperage);
                    break;
                case 5:
                    readings_table[prefill_Data_Table_Pointer].wattage = atof(Field);      // Wattage
                    //                   console.print(millis(), DEC); console.print("\tPrefill_Array wattage: ");
                    //                   console.println(readings_table[Data_Table_Pointer].wattage);
                    break;
                case 6:
                    readings_table[prefill_Data_Table_Pointer].uptime = atof(Field);       // Up Time
                    //                   console.print(millis(), DEC); console.print("\tPrefill_Array uptime: ");
                    //                   console.println(readings_table[Data_Table_Pointer].uptime);
                    break;
                case 7:
                    readings_table[prefill_Data_Table_Pointer].kilowatthour = atof(Field); // KiloWatt Hour
                    //                   console.print(millis(), DEC); console.print("\tPrefill_Array kilowatthour: ");
                    //                   console.println(readings_table[Data_Table_Pointer].kilowatthour);
                    break;
                case 8:
                    readings_table[prefill_Data_Table_Pointer].powerfactor = atof(Field);  // Power Factor
                    //                   console.print(millis(), DEC); console.print("\tPrefill_Array power factor: ");
                    //                   console.println(readings_table[Data_Table_Pointer].powerfactor);
                    break;
                case 9:
                    readings_table[prefill_Data_Table_Pointer].unknown = atof(Field);      // Unknown
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array unknown: ");
                    //                    console.println(readings_table[Data_Table_Pointer].unknown);
                    break;
                case 10:
                    readings_table[prefill_Data_Table_Pointer].frequency = atof(Field);    // Frequency
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array frequency: ");
                    //                    console.println(readings_table[Data_Table_Pointer].frequency);
                    break;
                case 11:
                    readings_table[prefill_Data_Table_Pointer].sensor_temperature = atof(Field);  // Temperature
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array sensor temperature: ");
                    //                    console.println(readings_table[Data_Table_Pointer].sensor_temperature);
                    break;
                case 12:
                    readings_table[prefill_Data_Table_Pointer].weather_temperature = atof(Field);
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array weather temperature: ");
                    //                    console.println(readings_table[Data_Table_Pointer].weather_temperature);
                    break;
                case 13:
                    readings_table[prefill_Data_Table_Pointer].temperature_feels_like = atof(Field);
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array temperature feels_like: ");
                    //                    console.println(readings_table[Data_Table_Pointer].temperature_feels_like);
                    break;
                case 14:
                    readings_table[prefill_Data_Table_Pointer].temperature_maximum = atof(Field);
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array temperature maximum: ");
                    //                    console.println(readings_table[Data_Table_Pointer].temperature_maximum);
                    break;
                case 15:
                    readings_table[prefill_Data_Table_Pointer].temperature_minimum = atof(Field);
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array temperature minimum: ");
                    //                    console.println(readings_table[Data_Table_Pointer].temperature_minimum);
                    break;
                case 16:
                    readings_table[prefill_Data_Table_Pointer].atmospheric_pressure = atof(Field);
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array atmospheric pressure: ");
                    //                    console.println(readings_table[Data_Table_Pointer].atmospheric_pressure);
                    break;
                case 17:
                    readings_table[prefill_Data_Table_Pointer].relative_humidity = atof(Field);
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array relative humidity: ");
                    //                    console.println(readings_table[Data_Table_Pointer].relative_humidity);
                    break;
                case 18:
                    readings_table[prefill_Data_Table_Pointer].wind_speed = atof(Field);
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array wind speed: ");
                    //                    console.println(readings_table[Data_Table_Pointer].wind_speed);
                    break;
                case 19:
                    readings_table[prefill_Data_Table_Pointer].wind_direction = atof(Field);
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array wind direction: ");
                    //                    console.println(readings_table[Data_Table_Pointer].wind_direction);
                    break;
                case 20:
                    strncpy(readings_table[prefill_Data_Table_Pointer].weather_description, Field, sizeof(readings_table[prefill_Data_Table_Pointer].weather_description));
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array weather description: ");
                    //                    console.println(readings_table[Data_Table_Pointer].weather_description);
                    break;
                default:
                    break;
                }
                datafieldNo++;
                Field[0] = '\0';
                character_count = 0;
            }
            if (datatemp == '\n') {                                         // at this point the obtained record has been saved in the table
                // end of sd data row
                Update_Webpage_Variables_from_Table(readings_table, prefill_Data_Table_Pointer);             // update the web variables with the record just saved
                prefill_Data_Table_Pointer++;                               // increment the table array pointer
                Global_Data_Record_Count++;                                 // increment the running record toral 
                if (prefill_Data_Table_Pointer > data_table_size) {         // if pointer is greater than table size
                    Shuffle_Data_Table();
                    prefill_Data_Table_Pointer = data_table_size;
                }
                datafieldNo = 1;
            } // end of end of line detected
        } // end of while
    }
    dataFile.close();
    Write_Console_Message("Loaded Data Records: " + String(Global_Data_Record_Count));
    SD_Led_Flash_Start_Stop(false);
}
void Shuffle_Data_Table() {
    for (int i = 0; i < data_table_size - 1; i++) {           // shuffle the rows up, losing row 0, make row [table_size] free
        strncpy(readings_table[i].ldate, readings_table[i + 1].ldate, sizeof(readings_table[i].ldate));                           // [0]  date
        strncpy(readings_table[i].ltime, readings_table[i + 1].ltime, sizeof(readings_table[i].ltime));                           // [1]  time
        readings_table[i].voltage = readings_table[i + 1].voltage;                              // [2]  voltage
        readings_table[i].amperage = readings_table[i + 1].amperage;                            // [3]  amperage
        readings_table[i].wattage = readings_table[i + 1].wattage;                              // [4]  wattage
        readings_table[i].uptime = readings_table[i + 1].uptime;                                // [5]  uptime
        readings_table[i].kilowatthour = readings_table[i + 1].kilowatthour;                    // [6]  kilowatthours
        readings_table[i].powerfactor = readings_table[i + 1].powerfactor;                      // [7]  power factor
        readings_table[i].unknown = readings_table[i + 1].unknown;                              // [8]  unknown
        readings_table[i].frequency = readings_table[i + 1].frequency;                          // [9]  frequency
        readings_table[i].sensor_temperature = readings_table[i + 1].sensor_temperature;        // [10] sensor temperature
        readings_table[i].weather_temperature = readings_table[i + 1].weather_temperature;      // [11] weather temperature
        readings_table[i].temperature_feels_like = readings_table[i + 1].temperature_feels_like;// [12] temperature feels like
        readings_table[i].temperature_maximum = readings_table[i + 1].temperature_maximum;      // [13] temperature maximum
        readings_table[i].temperature_minimum = readings_table[i + 1].temperature_minimum;      // [14] temperature minimum
        readings_table[i].atmospheric_pressure = readings_table[i + 1].atmospheric_pressure;    // [15] atmospheric pressure
        readings_table[i].relative_humidity = readings_table[i + 1].relative_humidity;          // [16] relative humidity
        readings_table[i].wind_speed = readings_table[i + 1].wind_speed;                        // [17] wind speed
        readings_table[i].wind_direction = readings_table[i + 1].wind_direction;                // [18] wind direction
        strncpy(readings_table[i].weather_description, readings_table[i + 1].weather_description, sizeof(readings_table[i].weather_description));// [19] weather description
    }
}
void Update_Webpage_Variables_from_Data_File_Values(double Data_File_Values[20]) {
    // highest voltage ------------------------------------------------------------------------------------------------
    if (Data_File_Values[Voltage] >= highest_voltage) {
        time_of_highest_voltage = String(Data_File_Values_1);
        highest_voltage = Data_File_Values[Voltage];                       // update the largest current value
    }
    // lowest voltage -------------------------------------------------------------------------------------------------
    if (lowest_voltage >= Data_File_Values[Voltage]) {
        time_of_lowest_voltage = String(Data_File_Values_1);
        //       console.print(millis(), DEC); console.print("\tWrite new data record Data_File_Values_1: "); console.println(time_of_lowest_voltage);
        lowest_voltage = Data_File_Values[Voltage];                        // update the largest current value
    }
    // largest amperage -----------------------------------------------------------------------------------------------    if (Data_Values[1] >= largest_amperage) {                  // load the maximum amperage value
    if (Data_File_Values[Amperage] >= highest_amperage) {
        time_of_highest_amperage = String(Data_File_Values_1);
        highest_amperage = Data_File_Values[Amperage];                      // update the largest current value
    }
    // highest weather temperature ------------------------------------------------------------------------------------
    if (Data_File_Values[Weather_Temperature] >= highest_temperature) {           // update the highest weather temperature
        time_of_highest_temperature = String(Data_File_Values_1);
        highest_temperature = Data_File_Values[Weather_Temperature];              // update the highest weather temperature
    }
    // lowest temperature ---------------------------------------------------------------------------------------------
    if (Data_File_Values[Weather_Temperature] <= lowest_temperature) {            // update the lowest weather temperature
        time_of_lowest_temperature = String(Data_File_Values_1);
        lowest_temperature = Data_File_Values[Weather_Temperature];               // update the lowest weather temperature
    }
    time_of_latest_reading = String(Data_File_Values_1);
    // latest weather temperature -------------------------------------------------------------------------------------
    latest_weather_temperature = Data_File_Values[Weather_Temperature];           // update the highest weather temperature
    // latest weather temperature feels like --------------------------------------------------------------------------
    latest_weather_temperature_feels_like = Data_File_Values[Temperature_Feels_Like];
    // latest weather temperature maximum -----------------------------------------------------------------------------
    latest_weather_temperature_maximum = Data_File_Values[Temperature_Maximum];
    // latest weather temperature minimum -----------------------------------------------------------------------------
    latest_weather_temperature_minimum = Data_File_Values[Temperature_Minimum];
    // latest atmospheric pressure ------------------------------------------------------------------------------------
    latest_atmospheric_pressure = Data_File_Values[Atmospheric_Pressure];
    // latest relative humidity ---------------------------------------------------------------------------------------
    latest_relative_humidity = Data_File_Values[Relative_Humidity];
    // latest wind speed ----------------------------------------------------------------------------------------------
    latest_wind_speed = Data_File_Values[Wind_Speed];
    // latest wind direction ------------------------------------------------------------------------------------------
    latest_wind_direction = Data_File_Values[Wind_Direction];
    // latest weather description -------------------------------------------------------------------------------------
    latest_weather_description = String(Data_File_Values_19);
    // ----------------------------------------------------------------------------------------------------------------
    SD_freespace_double = (double)SD_freespace / 1000000;
    if (SD_freespace < critical_SD_freespace) {
        Write_Console_Message("\tWARNING - SD Free Space critical " + String(SD_freespace) + "MBytes");
    }
}
void Update_Webpage_Variables_from_Table(record_type readings_table[data_table_size], int Data_Table_Pointer) {
    if (readings_table[Data_Table_Pointer].voltage >= highest_voltage) {
        time_of_highest_voltage = String(readings_table[Data_Table_Pointer].ltime);
        highest_voltage = readings_table[Data_Table_Pointer].voltage;
    }
    if ((readings_table[Data_Table_Pointer].voltage <= lowest_voltage) || !lowest_voltage) {
        time_of_lowest_voltage = String(readings_table[Data_Table_Pointer].ltime);
        //       console.print(millis(), DEC); console.print("\tUpdate Webpage Variables ltime : "); console.println(time_of_lowest_voltage);
        lowest_voltage = readings_table[Data_Table_Pointer].voltage;                     // update the largest current value
    }
    if (readings_table[Data_Table_Pointer].amperage >= highest_amperage) {               // load the maximum amperage value
        time_of_highest_amperage = String(readings_table[Data_Table_Pointer].ltime);
        highest_amperage = readings_table[Data_Table_Pointer].amperage;                  // update the largest current value
    }
    if (readings_table[Data_Table_Pointer].weather_temperature >= highest_temperature) { // update the highest weather temperature
        time_of_highest_temperature = String(readings_table[Data_Table_Pointer].ltime);
        highest_temperature = readings_table[Data_Table_Pointer].weather_temperature;    // update the highest weather temperature
    }
    if (readings_table[Data_Table_Pointer].weather_temperature <= lowest_temperature) {  // update the lowest weather temperature
        time_of_lowest_temperature = String(readings_table[Data_Table_Pointer].ltime);
        lowest_temperature = readings_table[Data_Table_Pointer].weather_temperature;     // update the highest weather temperature
    }
    latest_weather_temperature_feels_like = readings_table[Data_Table_Pointer].temperature_feels_like;
    latest_weather_temperature_maximum = readings_table[Data_Table_Pointer].temperature_maximum;
    latest_weather_temperature_minimum = readings_table[Data_Table_Pointer].temperature_minimum;
    latest_relative_humidity = readings_table[Data_Table_Pointer].relative_humidity;
    latest_atmospheric_pressure = readings_table[Data_Table_Pointer].atmospheric_pressure;
    latest_wind_speed = readings_table[Data_Table_Pointer].wind_speed;
    latest_wind_direction = readings_table[Data_Table_Pointer].wind_direction;
    latest_weather_description = readings_table[Data_Table_Pointer].weather_description;
}
void Prefill_Console_Array() {
    int console_character_count = 0;
    char console_txtField[120];
    int console_fieldNo = 1;
    char console_temp;
    Global_Data_Table_Pointer = 0;
    SD_Led_Flash_Start_Stop(true);
    File consoleFile = SD.open("/" + ConsoleFileName, FILE_READ);
    if (consoleFile) {
        Write_Console_Message("Loading console file from " + String(ConsoleFileName));
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
                    strncpy(console_table[Global_Console_Table_Pointer].ldate, console_txtField, sizeof(console_table[Global_Console_Table_Pointer].ldate));        // Date
                    break;
                case 2:
                    strncpy(console_table[Global_Console_Table_Pointer].ltime, console_txtField, sizeof(console_table[Global_Console_Table_Pointer].ltime));        // Time
                    break;
                case 3:
                    console_table[Global_Console_Table_Pointer].milliseconds = atof(console_txtField); // milliseconds
                    break;
                case 4:
                    strncpy(console_table[Global_Console_Table_Pointer].message, console_txtField, sizeof(console_table[Global_Console_Table_Pointer].message));      // message
                    break;
                }
                console_fieldNo++;
                console_txtField[0] = '\0';
                console_character_count = 0;
            }
            if (console_temp == '\n') {                                                         // end of sd data row
                Global_Console_Table_Pointer++;                                                         // increment array pointer
                Global_Console_Record_Count++;                                                 // increment the current_record count
                if (Global_Console_Table_Pointer > console_table_size) {                                // if pointer is greater than table size
                    for (int i = 0; i < console_table_size; i++) {                              // shuffle the rows up, losing row 0, make row [table_size] free
                        strncpy(console_table[i].ldate, console_table[i + 1].ldate, sizeof(console_table[i].ldate));             // date
                        strncpy(console_table[i].ltime, console_table[i + 1].ltime, sizeof(console_table[i].ltime));             // time
                        console_table[i].milliseconds = console_table[i + 1].milliseconds;
                        strncpy(console_table[i].message, console_table[i + 1].message, sizeof(console_table[i].message));
                    }
                    Global_Console_Table_Pointer = console_table_size;                                                  // subsequent records will be added at the end of the table
                }
                console_fieldNo = 1;
            }
        } // end of while
    }
    consoleFile.close();
    Write_Console_Message("Loaded Console Records: " + String(Global_Console_Record_Count));
    SD_Led_Flash_Start_Stop(false);
}
void Display() {
    webpage = "";
    double maximum_amperage = 0;
    double minimum_amperage = 0;
    double this_amperage = 0;
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
    for (int i = 0; i < (Global_Data_Table_Pointer); i++) {
        if (String(readings_table[i].ltime) != "") {                  // if the ltime field contains data
            for (int y = 0; y < 8; y++) {                               // replace the ":"s in ltime with ","
                if (readings_table[i].ltime[y] == ':') {
                    readings_table[i].ltime[y] = ',';
                }
            }
            webpage += "[[";
            webpage += String(readings_table[i].ltime) + "],";
            webpage += String(readings_table[i].amperage, 1) + "]";
            this_amperage = readings_table[i].amperage;
            if (this_amperage > maximum_amperage) maximum_amperage = this_amperage;
            if (this_amperage < minimum_amperage) minimum_amperage = this_amperage;
            if (i != Global_Data_Table_Pointer) webpage += ",";    // do not add a "," to the last record
        }
    }
    webpage += "]);\n";
    webpage += F("var options = {");
    webpage += F("title:'Electrical Power Consumption");
    webpage += " (logarithmic scale)";
    webpage += F("',titleTextStyle:{fontName:'Arial', fontSize:20, color: 'DodgerBlue'},");
    webpage += F("legend:{position:'bottom'},colors:['red'],backgroundColor:'#F3F3F3',chartArea: {width:'90%', height:'80%'},");
    webpage += F("hAxis:{slantedText:true,slantedTextAngle:90,titleTextStyle:{width:'100%',color:'Purple',bold:true,fontSize:16},");
    webpage += F("gridlines:{color:'#333'},showTextEvery:1");
    webpage += F("},");
    webpage += F("vAxes:");
    webpage += F("{0:{viewWindowMode:'explicit',gridlines:{color:'black'}, viewWindow:{");
    webpage += F("min:");
    webpage += String(minimum_amperage, 1);
    webpage += F(",max:");
    webpage += String(maximum_amperage, 1);
    webpage += F("}, ");
    webpage += F("scaleType: '");
    webpage += "log";
    webpage += F("', title : 'Amperage(A)', format : '##.####'}, ");
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
    Write_Console_Message("Processor Reset via Web Command");
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
    webpage += F("<li><a href='/Information'>Display Information</a></li>");
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
void Information() {                                                 // Display file size of the datalog file
    int file_count = Count_Files_on_SD_Drive();
    webpage = ""; // don't delete this command, it ensures the server works reliably!
    Page_Header(true, "Energy Monitor Information");
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
    // Data Record Count ----------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Number of Data Readings: ");
    webpage += String(Global_Data_Record_Count);
    webpage += F(", Number of Console Entries: ");
    webpage += String(Global_Console_Record_Count);
    webpage += "</span></strong></p>";
    // Highest Voltage ------------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Highest Voltage was recorded at ");
    webpage += time_of_highest_voltage + " : " + String(highest_voltage) + " volts";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Lowest Voltage -------------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Lowest Voltage was recorded at ");
    webpage += time_of_lowest_voltage + " : " + String(lowest_voltage) + " volts";
    //   console.print(millis(), DEC); console.print("\tInformation time_of_lowest_voltage: "); console.println(time_of_lowest_voltage);
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Highest Amperage ----------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Greatest Amperage was recorded at ");
    webpage += time_of_highest_amperage + " : " + String(highest_amperage) + " amps";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Latest Weather Temperature Feels Like, Maximum & Minimum -----------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Temperature: ");
    webpage += String(latest_weather_temperature, 2);
    webpage += F("&deg;C,");
    webpage += F("  Feels Like: ");
    webpage += String(latest_weather_temperature_feels_like, 2);
    webpage += F("&deg;C,");
    webpage += F("  Maximum: ");
    webpage += String(latest_weather_temperature_maximum, 2);
    webpage += F("&deg;C,");
    webpage += F("  Minimum: ");
    webpage += String(latest_weather_temperature_minimum, 2);
    webpage += F("&deg;C");
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Latest Weather Relative Humidity -----------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Relative Humidity: ");
    webpage += String(latest_relative_humidity) + "%";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Latest Atmospheric Pressure ----------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Atmospheric Pressure: ");
    webpage += String(latest_atmospheric_pressure) + " millibars";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Latest Wind Speed and Direction ------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Wind Speed: ");
    webpage += String(latest_wind_speed) + "m/s,";
    webpage += F("Direction: ");
    webpage += String(latest_wind_direction);
    webpage += F("&deg;");
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Latest Weather Description -----------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Weather: ");
    webpage += String(latest_weather_description);
    webpage += "</span></strong></p>";
    // ----------------------------------------------------------------------------------------------------------------
    datafile.close();
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
}
void Download_Files() {
    int file_count = Count_Files_on_SD_Drive();                                 // this counts and creates an array of file names on SD
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
    Write_Console_Message("Download of File " + fileName + " Requested via Webpage");
    File datafile = SD.open("/" + fileName, FILE_READ);    // Now read data from FS
    if (datafile) {                                             // if there is a file
        if (datafile.available()) {                             // If data is available and present
            String contentType = "application/octet-stream";
            server.sendHeader("Content-Disposition", "attachment; filename=" + fileName);
            if (server.streamFile(datafile, contentType) != datafile.size()) {
                Write_Console_Message("Sent less data (" + String(server.streamFile(datafile, contentType)) + ") from " + fileName + " than expected (" + String(datafile.size()) + ")");
            }
        }
    }
    datafile.close(); // close the file:
    webpage = "";
}
void Delete_Files() {                                                           // allow the cliet to select a file for deleti
    int file_count = Count_Files_on_SD_Drive();                                 // this counts and creates an array of file names on SD
    Write_Console_Message("Delete Files Requested via Webpage");
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
        Write_Console_Message(DataFileName + " Removed");
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
    Write_Console_Message("Web Display of Console Messages Requested via Webpage");
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
                //                Write_Console_Message(console_message);
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
    Write_Console_Message("Start of Wipe Files Request by Switch");
    String filename;
    File root = SD.open("/");                                       //  Open the root directory
    while (true) {
        File entry = root.openNextFile();                           //  get the next file
        if (entry) {
            filename = entry.name();
            Write_Console_Message("Removing " + filename);
            SD.remove(entry.name());                                //  delete the file
        }
        else {
            root.close();
            Write_Console_Message("All files removed from root directory, rebooting");
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
void Receive(int field) {
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
                Write_Console_Message("No Reply from RS485 within 500 ms");
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
        case 0: {                                                                               // Voltage
            double voltage = (value[3] << 8) + value[4];                                          // Received is number of tenths of volt
            voltage = voltage / (double)10;                                                         // convert to volts
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print(millis(), DEC); console.println("\tReceived Data: ");
            console.print("\t\tReceived Voltage: "); printConsoleDouble(voltage, 6); console.println();
#endif
            Data_File_Values[Voltage] = voltage;                               // Voltage output format double
            break;
        }
        case 1: {                                                                                // Amperage
            double amperage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);       // Received is number of milli amps 
            amperage = amperage / (double)1000;                                                         // convert to amps 0.00n
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t\tReceived Ampergae: "); printConsoleDouble(amperage, 6); console.println();
#endif
            Data_File_Values[Amperage] = amperage;                                                                            // Amperage output format double nn.n
            break;
        }
        case 2: {                                                                                 // Wattage
            double wattage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);        // Recieved is number of tenths of watts
            wattage = wattage / (double)10;                                                             // convert to watts
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t\tReceived Wattage: "); printConsoleDouble(wattage, 6); console.println();
#endif
            Data_File_Values[Wattage] = wattage;
            break;
        }
        case 3: {
            double uptime = (double)(value[3] << 8) + (double)value[4];
            uptime = uptime / (double)60;                                                              // convert to hours
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t\tReceived Uptime: "); printConsoleDouble(uptime, 6); console.println();
#endif
            Data_File_Values[UpTime] = uptime;
            break;
        }
        case 4: {
            double kilowatthour = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);
            kilowatthour = kilowatthour / (double)1000;
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t\tReceived Kilowatthour: "); printConsoleDouble(kilowatthour, 6); console.println();
#endif
            Data_File_Values[Kilowatthour] = kilowatthour;
            break;
        }
        case 5: {
            double powerfactor = (double)(value[3] << 8) + (double)value[4];
            powerfactor = powerfactor / (double)100;
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t\tReceived Power Factor: "); printConsoleDouble(powerfactor, 6); console.println();
#endif
            Data_File_Values[PowerFactor] = powerfactor;
            break;
        }
        case 6: {
            double unknown = (value[3] << 8) + value[4];
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t\tReceived Unknown: "); printConsoleDouble(unknown, 6); console.println();
#endif
            Data_File_Values[Unknown] = unknown;
            break;
        }
        case 7: {
            double frequency = (double)(value[3] * (double)256) + (double)value[4];
            frequency = frequency / (double)10;
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t\tReceived frequency: "); printConsoleDouble(frequency, 6); console.println();
#endif
            Data_File_Values[Frequency] = frequency;
            break;
        }
        case 8: {
            double temperature = (double)(value[3] << 8) + (double)value[4];
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t\tReceived Temperature: "); printConsoleDouble(temperature, 6); console.println();
#endif
            Data_File_Values[Sensor_Temperature] = temperature;
            break;
        }
        }
    }
}
void Check_Green_Switch() {
    Green_Switch.update();
    if (Green_Switch.fell()) {
        Write_Console_Message("Green Button Pressed - No Action Assigned");
    }
}
void Check_Blue_Switch() {
    char Running_led_state = 0;
    char SD_led_state = 0;
    Blue_Switch.update();                                                   // update wipe switch
    if (Blue_Switch.fell()) {
        Write_Console_Message("Blue Button Pressed");
        Running_led_state = digitalRead(Running_led_pin);                   // save the current state of the leds
        SD_led_state = digitalRead(SD_Active_led_pin);
        do {
            digitalWrite(Running_led_pin, HIGH);                            // turn the run led on
            digitalWrite(SD_Active_led_pin, LOW);                           // turn the sd led off
            Yellow_Switch.update();
            if (Yellow_Switch.fell()) Yellow_Switch_Pressed = true;         // Blue switch + Yellow switch = wipe directory
            Red_Switch.update();                                            // reset will cancel the Wipe
            if (Red_Switch.fell()) {
                Write_Console_Message("Red Button Pressed");
                ESP.restart();
            }
            delay(150);
            digitalWrite(Running_led_pin, LOW);                             // turn the run led off
            digitalWrite(SD_Active_led_pin, HIGH);                          // turn the sd led on
            delay(150);
        } while (!Yellow_Switch_Pressed);
        if (Yellow_Switch_Pressed) {
            Write_Console_Message("Yellow Button Pressed");
            digitalWrite(SD_Active_led_pin, HIGH);
            Write_Console_Message("Wiping Files");
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
        Write_Console_Message("Red Button Pressed");
        ESP.restart();
    }
}
void Check_Yellow_Switch() {
    Yellow_Switch.update();
    if (Yellow_Switch.fell()) {
        Write_Console_Message("Yellow Button Pressed");
        Yellow_Switch_Pressed = true;
    }
    if (Yellow_Switch.rose()) {
        Write_Console_Message("Yellow Button Released");
        Yellow_Switch_Pressed = false;
    }
    return;
}
void Clear_Arrays() {                                           // clear the web arrays of old records
    for (int x = 0; x <= console_table_size; x++) {
        console_table[x].ldate[0] = '0';
        console_table[x].ltime[0] = '0';
        console_table[x].message[0] = '0';
        console_table[x].milliseconds = 0;
    }
    for (int x = 0; x <= data_table_size; x++) {
        readings_table[x].ldate[0] = '0';
        readings_table[x].ltime[0] = '0';
        readings_table[x].amperage = 0;
        readings_table[x].frequency = 0;
        readings_table[x].kilowatthour = 0;
        readings_table[x].powerfactor = 0;
        readings_table[x].sensor_temperature = 0;
        readings_table[x].unknown = 0;
        readings_table[x].uptime = 0;
        readings_table[x].voltage = 0;
        readings_table[x].wattage = 0;
        readings_table[x].sensor_temperature = 0;
        readings_table[x].weather_temperature = 0;
        readings_table[x].temperature_feels_like = 0;
        readings_table[x].temperature_maximum = 0;
        readings_table[x].temperature_minimum = 0;
        readings_table[x].atmospheric_pressure = 0;
        readings_table[x].relative_humidity = 0;
        readings_table[x].wind_speed = 0;
        readings_table[x].wind_direction = 0;
        readings_table[x].weather_description[0] = '0';
    }
    Global_Data_Table_Pointer = 0;
    Global_Console_Table_Pointer = 0;
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
        Write_Console_Message("Attempting to Get Date " + String(connection_attempts));
        delay(500);
        Check_Red_Switch();
        connection_attempts++;
        if (connection_attempts > 20) {
            Write_Console_Message("Time Network Error, Restarting");
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
        Write_Console_Message("Attempting to Get Time " + String(connection_attempts));
        delay(500);
        Check_Red_Switch();                         // see if user wants to abort getting the time before 20 connectio attempts
        connection_attempts++;
        if (connection_attempts > 20) {
            Write_Console_Message("Time Network Error, Restarting");
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

    String console_message = String(val);  //prints the int part
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
    Write_Console_Message(console_message);
}
void printConsoleDouble(double val, byte precision) {
    // prints val with number of decimal places determine by precision
    // precision is a number from 0 to 6 indicating the desired decimial places
    // example: printDouble( 3.1415, 2); // prints 3.14 (two decimal places)
    console.print(val);  //prints the int part
    if (precision > 0) {
        console.print("."); // print the decimal point
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
            console.print("0");
        console.print(frac);
    }
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
    int start = 0;
    int end = 0;
#ifdef DISPLAY_DATA_VALUES_COLLECTED
    console.print(millis(), DEC); console.println("\tRaw Weather Record:");
    console.println(payload);
#endif
    // Temperature ----------------------------------------------------------------------------------------------------
    start = payload.indexOf("temp\":");                 // "temp":272.77,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Data_File_Values[Weather_Temperature] = (double)(atof(Parse_Output)) - (double)273.15;
    // Temperature Feels Like -----------------------------------------------------------------------------------------
    start = payload.indexOf("feels_like");              // "feels_like":283.47,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Data_File_Values[Temperature_Feels_Like] = (double)(atof(Parse_Output)) - (double)273.15;
    // Temperature Maximum --------------------------------------------------------------------------------------------
    start = payload.indexOf("temp_max");                // "temp_max":284.89,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Data_File_Values[Temperature_Maximum] = (double)(atof(Parse_Output)) - (double)273.15;
    // Temperature Minimum --------------------------------------------------------------------------------------------
    start = payload.indexOf("temp_min");                // "temp_min":282.75,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Data_File_Values[Temperature_Minimum] = (double)(atof(Parse_Output)) - (double)273.15;
    // Pressure -------------------------------------------------------------------------------------------------------
    start = payload.indexOf("pressure");                // "pressure":1018,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Data_File_Values[Atmospheric_Pressure] = (double)atof(Parse_Output);
    // humidity -------------------------------------------------------------------------------------------------------
    start = payload.indexOf("humidity\":");             // "humidity":95}
    start = payload.indexOf(":", start);
    end = payload.indexOf("}", start);
    parse(payload, start, end);
    Data_File_Values[Relative_Humidity] = (double)atof(Parse_Output);
    // weather description --------------------------------------------------------------------------------------------
    start = payload.indexOf("description");             // "description":"overcast clouds",
    start = (payload.indexOf(":", start) + 1);
    end = (payload.indexOf(",", start) - 1);
    parse(payload, start, end);
    Data_File_Values_19 = Parse_Output;
    // wind speed -----------------------------------------------------------------------------------------------------
    start = payload.indexOf("speed");                       // "speed":2.57,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Data_File_Values[Wind_Speed] = (double)(atof(Parse_Output));
    // wind direction -------------------------------------------------------------------------------------------------
    start = payload.indexOf("deg");                         // "deg":20
    start = payload.indexOf(":", start);
    end = payload.indexOf("}", start);
    parse(payload, start, end);
    Data_File_Values[Wind_Direction] = (double)atof(Parse_Output);
#ifdef DISPLAY_WEATHER_INFORMATION
    console.print("Parsed Weather Temperature: "); console.println(Data_File_Values[Weather_Temperature], DEC);
    console.print("Parsed Temperature Feels Like: "); console.println(Data_File_Values[Temperature_Feels_Like], DEC);
    console.print("Parsed Temperature Maximum: "); console.println(Data_File_Values[Temperature_Maximum], DEC);
    console.print("Parsed Temperature Minimum: "); console.println(Data_File_Values[Temperature_Minimum], DEC);
    console.print("Parsed Atmospheric Pressure: "); console.println(Data_File_Values[Atmospheric_Pressure], DEC);
    console.print("Parsed Relative Humidity: "); console.println(Data_File_Values[Relative_Humidity], DEC);
    console.print("Parsed Wind Direction: "); console.println(Data_File_Values[Wind_Direction], DEC);
    console.print("Parsed Wind Speed: "); console.println(Data_File_Values[Wind_Speed], DEC);
    console.print("Parsed Weather Description: "); console.println(Data_File_Values_19);
    while (1);
#endif
}
void parse(String payload, int start, int end) {
    int ptr = 0;
    for (int pos = start + 1; pos < end; pos++) {
        Parse_Output[ptr++] = payload[pos];
    }
    Parse_Output[ptr] = '\0';
}
/* Stack Size ---------------------------------------------------------------------------------------------------------
    The value returned is the high water mark in words.
    for example,
    on a 32 bit machine a return value of 1 would indicate that 4 bytes of stack were unused.
    If the return value is zero then the task has likely overflowed its stack.
    If the return value is close to zero then the task has come close to overflowing its stack.

unsigned int Stack_Size(void* pvParameters) {
    console.print(millis(), DEC); console.print("\tStack highwater mark: ");
    console.print(uxTaskGetStackHighWaterMark(NULL));
}
// -----------------------------------------------------------------------------------------------------------------------
  void* SpStart = NULL;
  StackPtrAtStart = (void *)&SpStart;
  watermarkStart =  uxTaskGetStackHighWaterMark(NULL);
  StackPtrEnd = StackPtrAtStart - watermarkStart;

  Serial.begin(115200);
  delay(2000);

  Serial.printf("\r\n\r\nAddress of Stackpointer near start is:  %p \r\n",  (void *)StackPtrAtStart);
  Serial.printf("End of Stack is near: %p \r\n",  (void *)StackPtrEnd);
  Serial.printf("Free Stack near start is:  %d \r\n",  (uint32_t)StackPtrAtStart - (uint32_t)StackPtrEnd);






*/