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
09/01/2023  10.1 Bug Fix
20/01/2023  11.0 Radical rewrite of Data handling.
21/01/2023  11.1 Coverted receive milli amperage to amperage in amps.
25/01/2023  11.2 Removed milliseconds from console records
*/
String version = "V11.1";                       // software version number, shown on webpage
// compiler directives ------------------------------------------------------------------------------------------------
//#define ALLOW_WORKING_FILE_DELETION           // allows the user to chose to delete the day's working files
//#define DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED  //
//#define DISPLAY_DATA_VALUES_COLLECTED           // print the data values as they are collected
//#define DISPLAY_DATA_VALUES_WRITTEN             // print the data values as they written to sd drive
//#define DISPLAY_WEATHER_INFORMATION             // print the weather data as it is received
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
#include <time.h>
#include <Bounce2.h>
#include <Uri.h>
#include <HTTP_Method.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <stdio.h>
// --------------------------------------------------------------------------------------------------------------------
const char* ssid = "Woodleigh";
const char* password = "2008198399";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;                             // offset for the date and time function
const char city[] = { "Basingstoke\0" };
const char region[] = { "uk\0" };
const String WiFi_Status_Message[7] = {
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
constexpr int Number_of_Column_Field_Names = 23;
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
// Date and Time Fields -----------------------------------------------------------------------------------------------0
typedef struct {
    int Second;
    int Minute;
    int Hour;
    int Day;
    int Month;
    int Year;
    String Date = "18/11/1951";
    String Time = "00:00:00";
}sensor_data;
sensor_data Sensor_Data;
// Instantiations -----------------------------------------------------------------------------------------------------
struct tm timeinfo;
Bounce Red_Switch = Bounce();
Bounce Green_Switch = Bounce();
Bounce Blue_Switch = Bounce();
Bounce Yellow_Switch = Bounce();
WebServer server(80);                   // WebServer(HTTP port, 80 is defAult)
WiFiClient client;
HTTPClient http;
File Datafile;                         // Full data file, holds all readings from KWS-AC301L
File Consolefile;
// KWS Request Field Numbers ------------------------------------------------------------------------------------------
constexpr int Request_Voltage = 0;
constexpr int Request_Amperage = 1;
constexpr int Request_Wattage = 2;
constexpr int Request_UpTime = 3;
constexpr int Request_Kilowatthour = 4;
constexpr int Request_Power_Factor = 5;
constexpr int Request_Frequency = 6;
constexpr int Request_Sensor_Temperature = 7;
// --------------------------------------------------------------------------------------------------------------------
constexpr bool receive = 0;
constexpr bool transmit = 1;
constexpr int Number_of_RS485_Requests = 7;
constexpr byte RS485_Requests[Number_of_RS485_Requests + 1][8] = {
                        {0x02,0x03,0x00,0x0E,0x00,0x01,0xE5,0xFA},			// [0]  Request Voltage, in tenths of a volt, divided by 10 for volts
                        {0x02,0x03,0x00,0x0F,0x00,0x02,0xF4,0x3B},			// [1]  Request Amperage, in milli-amps, divided by 1000 for amps
                        {0x02,0x03,0x00,0x11,0x00,0x02,0x94,0x3D},			// [2]  Request Watts, in tenths of a watt, divided by 10 for watts
                        {0x02,0x03,0x00,0x19,0x00,0x01,0x55,0xFE},			// [3]  Request uptime, in minutes
                        {0x02,0x03,0x00,0x17,0x00,0x02,0x74,0x3C},          // [4]  Request kilo_watt_hour, in milli-watts, divided by 1000 for kWh
                        {0x02,0x03,0x00,0x1D,0x00,0x01,0x14,0x3F},          // [5]  Request power factor, in one hundredths, divided by 100 for units
                        {0x02,0x03,0x00,0x1E,0x00,0x01,0xE4,0x3F},          // [6]  Request Hertz, in tenths of a hertz, divided by 10 for Hz
                        {0x02,0x03,0x00,0x1A,0x00,0x01,0xA5,0xFE},          // [7]  Request temperature, in degrees centigrade
};
char print_buffer[80];
String DataFileName = "20220101";
String ConsoleFileName = "20220101";
constexpr int Number_of_Field_Names = 9;
const String Request_Field_Names[Number_of_Field_Names + 1] = {
                        "Date",                     // [0]
                        "Time",                     // [1]
                        "Voltage",                  // [2]
                        "Amperage",                 // [3]
                        "Wattage",                  // [4]
                        "Up Time",                  // [5]
                        "kiloWattHours",            // [6]
                        "Power Factor",             // [7]
                        "Frequency",                // [8]
                        "Sensor Temperature",       // [9]
};
String Column_Field_Names[Number_of_Column_Field_Names + 1] = {
                        "Date",                     // [0]
                        "Time",                     // [1]
                        "Voltage",                  // [2]
                        "Amperage",                 // [3]
                        "Wattage",                  // [4]
                        "Up Time",                  // [5]
                        "kiloWattHours",            // [6]
                        "Power Factor",             // [7]
                        "Frequency",                // [8]
                        "Sensor Temperature",       // [9]
                        "Weather Temperature",      // [10]
                        "Temperature_Feels_Like",   // [11]
                        "Temperature Maximum",      // [12]
                        "Temperature Minimum",      // [13]
                        "Atmospheric Pressure",     // [14]
                        "Relative Humidity",        // [15]
                        "Wind Speed",               // [16]
                        "Wind Direction",           // [17]
                        "Weather Description"       // [18]
};
String FileNames[50];
struct Data_Record_Values {
    char ldate[11];                 //  [0 - 10]  [00]  date record was taken
    char ltime[9];                  //  [11 - 19] [01]  time record was taken
    double voltage;                 //  [20 - 23] [02]
    double amperage;                //  [24 - 27] [03]
    double wattage;                 //  [28 - 31] [04]
    double uptime;                  //  [32 - 35] [05]
    double kilowatthour;            //  [36 - 39] [06]
    double powerfactor;             //  [40 - 43] [07]
    double frequency;               //  [44 - 47] [08]
    double sensor_temperature;      //  [48 - 51] [09]
    double weather_temperature;     //  [52 - 55] [10]  Weather temperature
    double temperature_feels_like;  //  [56 - 59] [11]  temperature feels like
    double temperature_maximum;     //  [60 - 63] [12]  temperature maximum
    double temperature_minimum;     //  [64 - 67] [13]  temperature minimum
    double atmospheric_pressure;    //  [68 - 71] [14]  atmospheric pressure
    double relative_humidity;       //  [72 - 75] [15]  relative humidity
    double wind_speed;              //  [76 - 79] [16]  wind speed
    double wind_direction;          //  [80 - 83] [17]  wind direction
    char weather_description[20];   //  [84 - 103] [18] weather description
}__attribute__((packed));
constexpr int current_data_record_length = 103;
union Data_Record_Union {
    Data_Record_Values field;
    unsigned char character[current_data_record_length + 1];
};
Data_Record_Union Current_Data_Record;
String Data_File_Values_19 = "                                   ";     // [24] space for the read weather description
bool Yellow_Switch_Pressed = false;
String site_width = "1060";                     // width of web page
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
String This_Date = "2022/10/10";
struct Data_Table_Record {
    char ldate[11];                 //  [0 - 10]  date record was taken
    char ltime[9];                  //  [11 - 19]  time record was taken
    double voltage;                 //  [20 - 23]
    double amperage;                //  [24 - 27]
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
}__attribute__((packed));
constexpr int packet_length = 103;
union Data_Table_Union {
    Data_Table_Record field;
    unsigned char characters[packet_length + 1];
};
Data_Table_Union readings_table[data_table_size + 1];
// Lowest Voltage -----------------------------------------------------------------------------------------------------
double lowest_voltage = 0;
String time_of_lowest_voltage = "00:00:00";
String date_of_lowest_voltage = "0000/00/00";
// Highest Voltage ----------------------------------------------------------------------------------------------------
double highest_voltage = 0;
String time_of_highest_voltage = "00:00:00";
String date_of_highest_voltage = "0000/00/00";
// Highest amperage ---------------------------------------------------------------------------------------------------
double highest_amperage = 0;
String time_of_highest_amperage = "00:00:00";
String date_of_highest_amperage = "0000/00/00";
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
extern volatile unsigned long timer0_millis;
void setup() {
    console.begin(console_Baudrate);                                            // enable the console
    while (!console);                                                           // wait for port to settle
    delay(4000);
    Post_Setup_Status = false;
    Write_Console_Message("Booting - Commencing Setup");
    Write_Console_Message("Configuring IO");
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
    Write_Console_Message("IO Configuration Complete");
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
    Update_TimeInfo(false);                                                  // update This date time info, no /s
    This_Date = Sensor_Data.Date;
    Update_TimeInfo(true);                                                  // update date and time with / and : 
    Last_Boot_Time = Sensor_Data.Time;
    Last_Boot_Date = Sensor_Data.Date;
    Create_New_Data_File();
    Create_New_Console_File();
    Write_Console_Message("Preparing Customised Weather Request");
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
    Write_Console_Message("Weather Request Created");
    digitalWrite(SD_Active_led_pin, LOW);
    Write_Console_Message("End of Setup");
    Write_Console_Message("Running in Full Function Mode");
    Post_Setup_Status = true;
}   // end of Setup
void loop() {
    digitalWrite(Running_led_pin, HIGH);                    // turn green led on
    Check_WiFi();                                           // check that the WiFi is still conected
    Check_Red_Switch();                                     // check if reset switch has been pressed
    Check_Green_Switch();                                   // check if start switch has been pressed
    Check_Blue_Switch();                                    // check if wipesd switch has been pressed
    Drive_Running_Led();                                    // on when started, flashing when not, flashing with SD led if waiting for reset
    Update_TimeInfo(false);                                 // update the Date and Time
    if (This_Date != Sensor_Data.Date && New_Day_File_Required == true) {   // check we are in same day as the setup
        Write_Console_Message("This Date:" + (This_Date)+" Now Date: " + Sensor_Data.Date + "New_Day_File_Required:" + String(New_Day_File_Required));
        Write_Console_Message("New Day Process Commenced");
        Create_New_Data_File();                             // no, so create a new Data File with new file name
        Create_New_Console_File();
        Clear_Arrays();                                     // clear memory
        New_Day_File_Required = false;
    }
    else {
        New_Day_File_Required = true;                       // reset the flag
    }
    server.handleClient();                                  // handle any messages from the website
    if (millis() > last_cycle + (unsigned long)5000) {      // send requests every 5 seconds (5000 millisecods)
        last_cycle = millis();                              // update the last read milli second reading
        // weather information request start --------------------------------------------------------------------------
        HTTPClient http;
        http.begin(complete_weather_api_link);              // start the weather connectio
        int httpCode = http.GET();                          // send the request
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
        // weather information request end ----------------------------------------------------------------------------
        // sensor information request start ---------------------------------------------------------------------------
        for (int i = 0; i <= Number_of_RS485_Requests; i++) {    // transmit the requests, assembling the Values array
            Send_Request(i);                                    // send the RS485 Port the requests, one by one
            Receive(i);                                         // receive the sensor output
        }                                                       // all values should now be populated
        // sensor information request end -----------------------------------------------------------------------------
        Update_TimeInfo(true);
        for (int x = 0; x < 10; x++) {
            Current_Data_Record.field.ldate[x] = Sensor_Data.Date[x];
        }
        for (int x = 0; x < 9; x++) {
            Current_Data_Record.field.ltime[x] = Sensor_Data.Time[x];
        }
        Write_New_Data_Record_to_Data_File();                   // write the new record to SD Drive
        Add_New_Data_Record_to_Display_Table();                 // add the record to the display table
    }                                                           // end of if millis >5000
}                                                               // end of loop
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
    Datafile.print(Current_Data_Record.field.ldate); Datafile.print(",");                   // [00] Date
    Datafile.print(Current_Data_Record.field.ltime); Datafile.print(",");                   // [01] Time
    Datafile.print(Current_Data_Record.field.voltage); Datafile.print(",");                 // [02] Voltage
    Datafile.print(Current_Data_Record.field.amperage); Datafile.print(",");                // [03] Amperage
    Datafile.print(Current_Data_Record.field.wattage); Datafile.print(",");                 // [04] Wattage
    Datafile.print(Current_Data_Record.field.uptime); Datafile.print(",");                  // [05] UpTime
    Datafile.print(Current_Data_Record.field.kilowatthour); Datafile.print(",");            // [06] Kiliowatthour
    Datafile.print(Current_Data_Record.field.powerfactor); Datafile.print(",");             // [07] Power Factor
    Datafile.print(Current_Data_Record.field.frequency); Datafile.print(",");               // [08] Frequency
    Datafile.print(Current_Data_Record.field.sensor_temperature); Datafile.print(",");      // [09] Sensor Temperature
    Datafile.print(Current_Data_Record.field.weather_temperature); Datafile.print(",");     // [10] Weaather Temperature
    Datafile.print(Current_Data_Record.field.temperature_feels_like); Datafile.print(",");  // [11] Temperature Feels Like
    Datafile.print(Current_Data_Record.field.temperature_maximum); Datafile.print(",");     // [12] Temperature Maximum
    Datafile.print(Current_Data_Record.field.temperature_minimum); Datafile.print(",");     // [13] Temperature Minimum
    Datafile.print(Current_Data_Record.field.atmospheric_pressure); Datafile.print(",");    // [14] Atmospheric Pressure
    Datafile.print(Current_Data_Record.field.relative_humidity); Datafile.print(",");       // [15] Relative Humidity
    Datafile.print(Current_Data_Record.field.wind_speed); Datafile.print(",");              // [16] Wind Speed
    Datafile.print(Current_Data_Record.field.wind_direction); Datafile.print(",");          // [17] Wind Direction
    Datafile.print(Current_Data_Record.field.weather_description);                          // [18] Weather Description
    Datafile.print("\n");                           // end of record
    Datafile.close();                               // close the sd file
    Datafile.flush();                               // make sure it has been written to SD
    Global_Data_Table_Pointer++;                    // increment the record count, the array pointer
    Global_Data_Record_Count++;                     // increment the current record count
    digitalWrite(SD_Active_led_pin, LOW);           // turn the SD activity LED on
    Update_Webpage_Variables_from_Current_Data_Record();
#ifdef DISPLAY_DATA_VALUES_WRITTEN
    console.print(millis(), DEC); console.println("\tData Values Written:");
    console.print("\t\tDate: "); console.println(Current_Data_Record.field.ldate);                                      // [0] date
    console.print("\t\tTime: "); console.println(Current_Data_Record.field.ltime);                                      // [1] time
    console.print("\t\tVoltage: "); console.println(Current_Data_Record.field.voltage, 2);                              // [2] voltage
    console.print("\t\tAmperage: "); console.println(Current_Data_Record.field.amperage, 3);                            // [3] amperage
    console.print("\t\tWattage: "); console.println(Current_Data_Record.field.wattage, 2);                              // [4] wattage
    console.print("\t\tUpTime: "); console.println(Current_Data_Record.field.uptime, 2);                                // [5] up time
    console.print("\t\tKilowatthour: "); console.println(Current_Data_Record.field.kilowatthour, 2);                    // [6] kilowatt hour
    console.print("\t\tPower Factor: "); console.println(Current_Data_Record.field.powerfactor, 2);                     // [7] power factor
    console.print("\t\tFrequency: "); console.println(Current_Data_Record.field.frequency, 2);                          // [8] frequency
    console.print("\t\tSensor Temperature: "); console.println(Current_Data_Record.field.sensor_temperature, 2);        // [9] sensor temperature
    console.print("\t\tWeather Temperature: "); console.println(Current_Data_Record.field.weather_temperature, 2);      // [10] weather temperature
    console.print("\t\tTemperature Feels Like: "); console.print(Current_Data_Record.field.temperature_feels_like, 2);  // [11] temperature feels like
    console.print("\t\tTemperature Maximum: "); console.println(Current_Data_Record.field.temperature_maximum, 2);      // [12] temperature maximum
    console.print("\t\tTemperature Minimum: "); console.println(Current_Data_Record.field.temperature_minimum, 2);      // [13] temperature minimum
    console.print("\t\tAtmospheric Pressure: "); console.println(Current_Data_Record.field.atmospheric_pressure, 2);    // [14] atmospheric pressure
    console.print("\t\tRelative Humidity: "); console.println(Current_Data_Record.field.relative_humidity, 2);          // [15] relative humidity
    console.print("\t\tWind Speed: "); console.println(Current_Data_Record.field.wind_speed, 2);                        // [16] wind direction
    console.print("\t\tWind Direction: "); console.println(Current_Data_Record.field.wind_direction, 2);                // [17] wind speed
    console.print("\t\tWeather Description"); console.println(Current_Data_Record.field.weather_description);           // [18] weather description
#endif
}
void Add_New_Data_Record_to_Display_Table() {
    if (Global_Data_Table_Pointer > data_table_size) {                                                   // table full, shuffle fifo
        for (i = 0; i < data_table_size; i++) {                                                          // shuffle the rows up, losing row 0, make row [table_size] free
            strncpy(readings_table[i].field.ldate, readings_table[i + 1].field.ldate, sizeof(readings_table[i].field.ldate)); // [0]  date
            strncpy(readings_table[i].field.ltime, readings_table[i + 1].field.ltime, sizeof(readings_table[i].field.ltime)); // [1]  time
            readings_table[i].field.voltage = readings_table[i + 1].field.voltage;                                      // [2]  voltage
            readings_table[i].field.amperage = readings_table[i + 1].field.amperage;                                    // [3]  amperage
            readings_table[i].field.wattage = readings_table[i + 1].field.wattage;                                      // [4]  wattage
            readings_table[i].field.uptime = readings_table[i + 1].field.uptime;                                        // [5]  up time
            readings_table[i].field.kilowatthour = readings_table[i + 1].field.kilowatthour;                            // [6]  kilowatt hour
            readings_table[i].field.powerfactor = readings_table[i + 1].field.powerfactor;                              // [7]  power factor
            readings_table[i].field.frequency = readings_table[i + 1].field.frequency;                                  // [8]  frequency
            readings_table[i].field.sensor_temperature = readings_table[i + 1].field.sensor_temperature;                // [9] sensor temperature
            readings_table[i].field.weather_temperature = readings_table[i + 1].field.weather_temperature;              // [10] weather temperature
            readings_table[i].field.temperature_feels_like = readings_table[i + 1].field.temperature_feels_like;        // [11] temperature
            readings_table[i].field.temperature_maximum = readings_table[i + 1].field.temperature_maximum;              // [12] temperature maximum
            readings_table[i].field.temperature_minimum = readings_table[i + 1].field.temperature_minimum;              // [13] temperature minimum
            readings_table[i].field.atmospheric_pressure = readings_table[i + 1].field.atmospheric_pressure;            // [14] atmospheric pressure
            readings_table[i].field.relative_humidity = readings_table[i + 1].field.relative_humidity;                  // [15] relative humidity
            readings_table[i].field.wind_speed = readings_table[i + 1].field.wind_speed;                                // [16] wind speed
            readings_table[i].field.wind_direction = readings_table[i + 1].field.wind_direction;                        // [17] wind direction
            strncpy(readings_table[i].field.weather_description, readings_table[i + 1].field.weather_description, sizeof(readings_table[i].field.weather_description));// [18] weather description
        }
        Global_Data_Table_Pointer = data_table_size;                                                             // subsequent records will be added at the end of the table
        strncpy(readings_table[data_table_size].field.ldate, Current_Data_Record.field.ldate, sizeof(readings_table[data_table_size].field.ldate));                       // [0]  date
        strncpy(readings_table[data_table_size].field.ltime, Current_Data_Record.field.ltime, sizeof(readings_table[data_table_size].field.ltime));                       // [1]  time
        readings_table[data_table_size].field.voltage = Current_Data_Record.field.voltage;                             // [2]  voltage
        readings_table[data_table_size].field.amperage = Current_Data_Record.field.amperage;                           // [3]  amperage
        readings_table[data_table_size].field.wattage = Current_Data_Record.field.wattage;                             // [4]  wattage
        readings_table[data_table_size].field.uptime = Current_Data_Record.field.uptime;                               // [5]  uptime
        readings_table[data_table_size].field.kilowatthour = Current_Data_Record.field.kilowatthour;                   // [6]  kilowatthours
        readings_table[data_table_size].field.powerfactor = Current_Data_Record.field.powerfactor;                     // [7]  power factor
        readings_table[data_table_size].field.frequency = Current_Data_Record.field.frequency;                         // [8]  frequency
        readings_table[data_table_size].field.sensor_temperature = Current_Data_Record.field.sensor_temperature;       // [9] sensor temperature
        readings_table[data_table_size].field.weather_temperature = Current_Data_Record.field.weather_temperature;     // [10] weather temperature
        readings_table[data_table_size].field.temperature_feels_like = Current_Data_Record.field.temperature_feels_like; // [11] temperature feels like
        readings_table[data_table_size].field.temperature_maximum = Current_Data_Record.field.temperature_maximum;     // [12] temperature maximum
        readings_table[data_table_size].field.temperature_minimum = Current_Data_Record.field.temperature_minimum;     // [13] temperature minimum
        readings_table[data_table_size].field.atmospheric_pressure = Current_Data_Record.field.atmospheric_pressure;   // [14] atmospheric pressure
        readings_table[data_table_size].field.relative_humidity = Current_Data_Record.field.relative_humidity;         // [15] relative humidity
        readings_table[data_table_size].field.wind_speed = Current_Data_Record.field.wind_speed;                       // [16] wind speed
        readings_table[data_table_size].field.weather_temperature = Current_Data_Record.field.weather_temperature;     // [17] wind direction
        strncpy(readings_table[data_table_size].field.weather_description, Current_Data_Record.field.weather_description, sizeof(readings_table[data_table_size].field.weather_description));        // [18] weather description
    }
    else {                                                                          // add the record to the table
        strncpy(readings_table[Global_Data_Table_Pointer].field.ldate, Current_Data_Record.field.ldate, sizeof(readings_table[Global_Data_Table_Pointer].field.ldate));                       // [0]  date
        strncpy(readings_table[Global_Data_Table_Pointer].field.ltime, Current_Data_Record.field.ltime, sizeof(readings_table[Global_Data_Table_Pointer].field.ltime));                       // [1]  time
        readings_table[Global_Data_Table_Pointer].field.voltage = Current_Data_Record.field.voltage;                             // [2]  voltage
        readings_table[Global_Data_Table_Pointer].field.amperage = Current_Data_Record.field.amperage;                           // [3]  amperage
        readings_table[Global_Data_Table_Pointer].field.wattage = Current_Data_Record.field.wattage;                             // [4]  wattage
        readings_table[Global_Data_Table_Pointer].field.uptime = Current_Data_Record.field.uptime;                               // [5]  uptime
        readings_table[Global_Data_Table_Pointer].field.kilowatthour = Current_Data_Record.field.kilowatthour;                   // [6]  kilowatthours
        readings_table[Global_Data_Table_Pointer].field.powerfactor = Current_Data_Record.field.powerfactor;                     // [7]  power factor
        readings_table[Global_Data_Table_Pointer].field.frequency = Current_Data_Record.field.frequency;                         // [8]  frequency
        readings_table[Global_Data_Table_Pointer].field.sensor_temperature = Current_Data_Record.field.sensor_temperature;       // [9] sensor temperature
        readings_table[Global_Data_Table_Pointer].field.weather_temperature = Current_Data_Record.field.weather_temperature;     // [10] weather temperature
        readings_table[Global_Data_Table_Pointer].field.temperature_feels_like = Current_Data_Record.field.temperature_feels_like; // [11] temperature feels like
        readings_table[Global_Data_Table_Pointer].field.temperature_maximum = Current_Data_Record.field.temperature_maximum;     // [12] temperature maximum
        readings_table[Global_Data_Table_Pointer].field.temperature_minimum = Current_Data_Record.field.temperature_minimum;     // [13] temperature minimum
        readings_table[Global_Data_Table_Pointer].field.atmospheric_pressure = Current_Data_Record.field.atmospheric_pressure;   // [14] atmospheric pressure
        readings_table[Global_Data_Table_Pointer].field.relative_humidity = Current_Data_Record.field.relative_humidity;         // [15] relative humidity
        readings_table[Global_Data_Table_Pointer].field.wind_speed = Current_Data_Record.field.wind_speed;                       // [16] wind speed
        readings_table[Global_Data_Table_Pointer].field.weather_temperature = Current_Data_Record.field.weather_temperature;     // [17] wind direction
        strncpy(readings_table[Global_Data_Table_Pointer].field.weather_description, Current_Data_Record.field.weather_description, sizeof(readings_table[Global_Data_Table_Pointer].field.weather_description));        // [18] weather description
    }
}
void Create_New_Console_File() {
    digitalWrite(SD_Active_led_pin, HIGH);                          // turn the SD activity LED on
    Update_TimeInfo(false);
    ConsoleFileName = Sensor_Data.Date + ".txt";                      // yes, so create a new file
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
        Update_TimeInfo(true);
        Consolefile.println(Sensor_Data.Date + "," + Sensor_Data.Time + "," + String(millis()) + ",Console File Started");
        Consolefile.close();
        Consolefile.flush();
        Write_Console_Message("Console File " + String(ConsoleFileName) + " created");
        Global_Console_Record_Count = 1;
        //        Global_Console_Table_Pointer = 1;
    }
    else {
        Write_Console_Message("Console File " + String(ConsoleFileName) + " already exists");
        Prefill_Console_Array();
    }
    digitalWrite(SD_Active_led_pin, LOW);                           // turn the SD activity LED off
}
void Create_New_Data_File() {
    Update_TimeInfo(false);
    DataFileName = Sensor_Data.Date + ".csv";
    digitalWrite(SD_Active_led_pin, HIGH);
    if (!SD.exists("/" + DataFileName)) {
        Datafile = SD.open("/" + DataFileName, FILE_WRITE);
        if (!Datafile) {                                            // log file not opened
            Write_Console_Message("Error opening Data file in Create New Data File [" + String(DataFileName) + "]");
            while (true) {
                digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                delay(500);
                Check_Red_Switch();
            }
        }
        for (int x = 0; x < Number_of_Column_Field_Names; x++) {                               // write data column headings into the SD file
            Datafile.print(Column_Field_Names[x]);
            Datafile.print(",");
        }
        Datafile.println(Column_Field_Names[Number_of_Column_Field_Names]);
        Datafile.close();
        Datafile.flush();
        Global_Data_Record_Count = 0;
        //        Global_Data_Table_Pointer = 0;
        digitalWrite(SD_Active_led_pin, LOW);
        Update_TimeInfo(false);
        This_Date = Sensor_Data.Date;                                 // update the current date
        Write_Console_Message("Data File " + DataFileName + " created");
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
    String pre_date = "1951/18/11";
    String pre_time = "00:00:00";
    if (Post_Setup_Status) {                                                // only write the console message to disk once setup is complete
        if (pre_loop_message_count > 0) {                                   // are there any pre loop console messages stored
            for (int x = 0; x < pre_loop_message_count; x++) {
                Update_TimeInfo(true);
                pre_date = Sensor_Data.Date;
                pre_time = Sensor_Data.Time;
                Write_New_Console_Message_to_Console_File(pre_date, pre_time, pre_loop_messages[x]);
                Add_New_Console_Message_to_Console_Table(pre_date, pre_time, pre_loop_messages[x]);
            }
            pre_loop_message_count = 0;
        }
        Update_TimeInfo(true);
        Date = Sensor_Data.Date;
        Time = Sensor_Data.Time;
        Write_New_Console_Message_to_Console_File(Date, Time, saved_console_message);
        Add_New_Console_Message_to_Console_Table(Date, Time, saved_console_message);
    }
    else {
        pre_loop_messages[pre_loop_message_count] = console_message;
        pre_loop_message_count++;
    }
    console_message = saved_console_message;
    console.print(millis(), DEC); console.print("\t"); console.println(console_message);
}
void Write_New_Console_Message_to_Console_File(String date, String time, String console_message) {
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
    Consolefile.println(date + "," + time + "," + console_message);
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
    Global_Console_Record_Count++;
    digitalWrite(SD_Active_led_pin, LOW);                           // turn the SD activity LED off
}
void Add_New_Console_Message_to_Console_Table(String date, String time, String console_message) {
    if (Global_Console_Table_Pointer > console_table_size) {                           // table full, shuffle fifo
        for (int i = 0; i < console_table_size; i++) {                              // shuffle the rows up, losing row 0, make row [table_size] free
            strncpy(console_table[i].ldate, console_table[i + 1].ldate, sizeof(console_table[i].ldate));             // date
            strncpy(console_table[i].ltime, console_table[i + 1].ltime, sizeof(console_table[i].ltime));             // time
            strncpy(console_table[i].message, console_table[i + 1].message, sizeof(console_table[i].message));
        }
        Global_Console_Table_Pointer = console_table_size;               // subsequent records will be added at the end of the table
        strncpy(console_table[console_table_size].ldate, date.c_str(), sizeof(console_table[console_table_size].ldate));
        strncpy(console_table[console_table_size].ltime, time.c_str(), sizeof(console_table[console_table_size].ltime));
        strncpy(console_table[console_table_size].message, console_message.c_str(), sizeof(console_table[console_table_size].message));
        Global_Console_Table_Pointer = console_table_size;                     // write the new message onto the end of the table
    }
    else {                                                                      // add the record to the table
        strncpy(console_table[Global_Console_Table_Pointer].ldate, date.c_str(), sizeof(console_table[Global_Console_Table_Pointer].ldate));
        strncpy(console_table[Global_Console_Table_Pointer].ltime, time.c_str(), sizeof(console_table[Global_Console_Table_Pointer].ltime));
        strncpy(console_table[Global_Console_Table_Pointer].message, console_message.c_str(), sizeof(console_table[Global_Console_Table_Pointer].message));      // write the new message onto the end of the table
        Global_Console_Table_Pointer++;                                                     // increment the table pointer
    }
    Global_Console_Table_Pointer++;                                             // increment the console table pointer
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
    if (WiFi.status() == WL_CONNECTED) {                              // disconnect to start new wifi connection
        Write_Console_Message("WiFi Already Connected");
        Write_Console_Message("Disconnecting");
        WiFi.disconnect(true);
        Write_Console_Message("Disconnected");
    }
    WiFi.begin(ssid, password);                                         // connect to the wifi network
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
    int datafieldNo = 0;
    char datatemp;
    int prefill_Data_Table_Pointer = 0;
    Global_Data_Table_Pointer = 0;
    SD_Led_Flash_Start_Stop(true);                                              // start the sd led flashing
    Write_Console_Message("Loading datafile from " + String(DataFileName));
    console.print(millis(), DEC); console.print("\t");
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
                case 0: {
                    strncpy(readings_table[prefill_Data_Table_Pointer].field.ldate, Field, sizeof(readings_table[prefill_Data_Table_Pointer].field.ldate));       // Date
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array ldate from file: ");
                    //                    console.println(readings_table[Data_Table_Pointer].ldate);
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [0] Date: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.ldate);
#endif
                    break;
                }
                case 1: {
                    strncpy(readings_table[prefill_Data_Table_Pointer].field.ltime, Field, sizeof(readings_table[prefill_Data_Table_Pointer].field.ltime));       // Time
                    //                    console.print(millis(), DEC); console.print("\tPrefill_Array ltime from file: ");
                    //                    console.println(readings_table[Data_Table_Pointer].ltime);
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [1] Time: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.ltime);
#endif
                    break;
                }
                case 2: {
                    readings_table[prefill_Data_Table_Pointer].field.voltage = atof(Field);         // [02] Voltage
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [2] Voltage: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.voltage);
#endif
                    break;
                }
                case 3: {
                    readings_table[prefill_Data_Table_Pointer].field.amperage = atof(Field);        // [03] Amperage
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [3] Amperage: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.amperage);
#endif
                    break;
                }
                case 4: {
                    readings_table[prefill_Data_Table_Pointer].field.wattage = atof(Field);         // [04] Wattage
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [4] Wattage: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.wattage);
#endif
                    break;
                }
                case 5: {
                    readings_table[prefill_Data_Table_Pointer].field.uptime = atof(Field);          // [05] Uptime
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [5] Uptime: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.uptime);
#endif
                    break;
                }
                case 6: {
                    readings_table[prefill_Data_Table_Pointer].field.kilowatthour = atof(Field);    // [06] Kilowatthour
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [6] KiloWattHour: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.kilowatthour);
#endif
                    break;
                }
                case 7: {
                    readings_table[prefill_Data_Table_Pointer].field.powerfactor = atof(Field);     // [07] Power Factor
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [7] Power Factor: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.powerfactor);
#endif
                    break;
                }
                case 8: {
                    readings_table[prefill_Data_Table_Pointer].field.frequency = atof(Field);       // [09] Frequency
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [9] Frequency: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.frequency);
#endif
                    break;
                }
                case 9: {
                    readings_table[prefill_Data_Table_Pointer].field.sensor_temperature = atof(Field);  // [10] Sensor Temperature
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [10] Sensor Temperature: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.sensor_temperature);
#endif
                    break;
                }
                case 10: {
                    readings_table[prefill_Data_Table_Pointer].field.weather_temperature = atof(Field); // [15] Weather Temperature
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [15] Weather Temperature: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.weather_temperature);
#endif
                    break;
                }
                case 11: {
                    readings_table[prefill_Data_Table_Pointer].field.temperature_feels_like = atof(Field);  // [16] Temperatre Feels Like
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [16] Temperature Feels Like: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.temperature_feels_like);
#endif
                    break;
                }
                case 12: {
                    readings_table[prefill_Data_Table_Pointer].field.temperature_maximum = atof(Field);     // [17] Temperature Maximum
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [17] Temperature Maximum: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.temperature_maximum);
#endif
                    break;
                }
                case 13: {
                    readings_table[prefill_Data_Table_Pointer].field.temperature_minimum = atof(Field);     // [18] Temperature Minimum
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [18] Temperature Minimum: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.temperature_minimum);
#endif
                    break;
                }
                case 14: {
                    readings_table[prefill_Data_Table_Pointer].field.atmospheric_pressure = atof(Field);    // [19] Atmospheric Pressure
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [19] Atmospheric Pressure: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.atmospheric_pressure);
#endif
                    break;
                }
                case 15: {
                    readings_table[prefill_Data_Table_Pointer].field.relative_humidity = atof(Field);       // [20] Relative Humidity
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [20] Relative Humidity: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.relative_humidity);
#endif
                    break;
                }
                case 16: {
                    readings_table[prefill_Data_Table_Pointer].field.wind_speed = atof(Field);              // [21] Wind Speed
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [21] Wind Speed: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.wind_speed);
#endif
                    break;
                }
                case 17: {
                    readings_table[prefill_Data_Table_Pointer].field.wind_direction = atof(Field);          // [22] Wind Direction
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [22] Wind Direction: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.wind_direction);
#endif
                    break;
                }
                case 18: {
                    strncpy(readings_table[prefill_Data_Table_Pointer].field.weather_description, Field, sizeof(readings_table[prefill_Data_Table_Pointer].field.weather_description));
#ifdef DISPLAY_PREFILL_ARRAY_VALUES_COLLECTED
                    console.print(millis(), DEC); console.print("\tPrefill_Array [23] Weather Description: ");
                    console.println(readings_table[prefill_Data_Table_Pointer].field.weather_description);
#endif
                    break;
                }
                default: {
                    break;
                }
                }
                datafieldNo++;
                Field[0] = '\0';
                character_count = 0;
            }
            if (datatemp == '\n') {                                         // at this point the obtained record has been saved in the table
                // end of sd data row
                Update_Webpage_Variables_from_Table(prefill_Data_Table_Pointer);             // update the web variables with the record just saved
                prefill_Data_Table_Pointer++;                               // increment the table array pointer
                Global_Data_Record_Count++;                                 // increment the running record toral
                console.print(".");
                if ((Global_Data_Record_Count % 400) == 0) {
                    console.print(" ("); console.print(Global_Data_Record_Count); console.println(")");
                    console.print(millis(), DEC);
                    console.print("\t");
                    digitalWrite(Running_led_pin, !digitalRead(Running_led_pin))
                        ;
                }
                if (prefill_Data_Table_Pointer > data_table_size) {         // if pointer is greater than table size
                    Shuffle_Data_Table();
                    prefill_Data_Table_Pointer = data_table_size;
                }
                datafieldNo = 0;
            } // end of end of line detected
        } // end of while
    }
    dataFile.close();
    console.print(" ("); console.print(Global_Data_Record_Count); console.println(")");
    Write_Console_Message("Loaded Data Records: " + String(Global_Data_Record_Count));
    SD_Led_Flash_Start_Stop(false);
}
void Shuffle_Data_Table() {
    for (int i = 0; i < data_table_size - 1; i++) {           // shuffle the rows up, losing row 0, make row [table_size] free
        strncpy(readings_table[i].field.ldate, readings_table[i + 1].field.ldate, sizeof(readings_table[i].field.ldate));                           // [0]  date
        strncpy(readings_table[i].field.ltime, readings_table[i + 1].field.ltime, sizeof(readings_table[i].field.ltime));                           // [1]  time
        readings_table[i].field.voltage = readings_table[i + 1].field.voltage;                              // [2]  voltage
        readings_table[i].field.amperage = readings_table[i + 1].field.amperage;                            // [3]  amperage
        readings_table[i].field.wattage = readings_table[i + 1].field.wattage;                              // [4]  wattage
        readings_table[i].field.uptime = readings_table[i + 1].field.uptime;                                // [5]  uptime
        readings_table[i].field.kilowatthour = readings_table[i + 1].field.kilowatthour;                    // [6]  kilowatthours
        readings_table[i].field.powerfactor = readings_table[i + 1].field.powerfactor;                      // [7]  power factor
        readings_table[i].field.frequency = readings_table[i + 1].field.frequency;                          // [8]  frequency
        readings_table[i].field.sensor_temperature = readings_table[i + 1].field.sensor_temperature;        // [9] sensor temperature
        readings_table[i].field.weather_temperature = readings_table[i + 1].field.weather_temperature;      // [10] weather temperature
        readings_table[i].field.temperature_feels_like = readings_table[i + 1].field.temperature_feels_like;// [11] temperature feels like
        readings_table[i].field.temperature_maximum = readings_table[i + 1].field.temperature_maximum;      // [12] temperature maximum
        readings_table[i].field.temperature_minimum = readings_table[i + 1].field.temperature_minimum;      // [13] temperature minimum
        readings_table[i].field.atmospheric_pressure = readings_table[i + 1].field.atmospheric_pressure;    // [14] atmospheric pressure
        readings_table[i].field.relative_humidity = readings_table[i + 1].field.relative_humidity;          // [15] relative humidity
        readings_table[i].field.wind_speed = readings_table[i + 1].field.wind_speed;                        // [16] wind speed
        readings_table[i].field.wind_direction = readings_table[i + 1].field.wind_direction;                // [17] wind direction
        strncpy(readings_table[i].field.weather_description, readings_table[i + 1].field.weather_description, sizeof(readings_table[i].field.weather_description));// [18] weather description
    }
}
void Update_Webpage_Variables_from_Current_Data_Record() {
    // highest voltage ------------------------------------------------------------------------------------------------
    if (Current_Data_Record.field.voltage >= highest_voltage) {
        date_of_highest_voltage = Current_Data_Record.field.ldate;
        time_of_highest_voltage = Current_Data_Record.field.ltime;
        highest_voltage = Current_Data_Record.field.voltage;                       // update the largest current value
    }
    else {
        if (date_of_highest_voltage == "") {
            date_of_highest_voltage = Current_Data_Record.field.ldate;
            time_of_highest_voltage = Current_Data_Record.field.ltime;
        }
    }

    // lowest voltage -------------------------------------------------------------------------------------------------
    if (lowest_voltage >= Current_Data_Record.field.voltage) {
        date_of_lowest_voltage = Current_Data_Record.field.ldate;
        time_of_lowest_voltage = Current_Data_Record.field.ltime;
        lowest_voltage = Current_Data_Record.field.voltage;                        // update the largest current value
    }
    else {
        if (date_of_lowest_voltage == "") {
            date_of_lowest_voltage = Current_Data_Record.field.ldate;
            time_of_lowest_voltage = Current_Data_Record.field.ltime;
        }
    }
    // largest amperage -----------------------------------------------------------------------------------------------    if (Data_Values[1] >= largest_amperage) {                  // load the maximum amperage value
    if (Current_Data_Record.field.amperage >= highest_amperage) {
        date_of_highest_amperage = Current_Data_Record.field.ldate;
        time_of_highest_amperage = Current_Data_Record.field.ltime;
        highest_amperage = Current_Data_Record.field.amperage;                      // update the largest current value
    }
    else {
        if (date_of_highest_amperage == "") {
            date_of_highest_amperage = Current_Data_Record.field.ldate;
            time_of_highest_amperage = Current_Data_Record.field.ltime;
        }
    }
    time_of_latest_reading = Current_Data_Record.field.ltime;
    // latest weather temperature -------------------------------------------------------------------------------------
    latest_weather_temperature = Current_Data_Record.field.weather_temperature;
    // latest weather temperature feels like --------------------------------------------------------------------------
    latest_weather_temperature_feels_like = Current_Data_Record.field.temperature_feels_like;
    // latest weather temperature maximum -----------------------------------------------------------------------------
    latest_weather_temperature_maximum = Current_Data_Record.field.temperature_maximum;
    // latest weather temperature minimum -----------------------------------------------------------------------------
    latest_weather_temperature_minimum = Current_Data_Record.field.temperature_minimum;
    // latest atmospheric pressure ------------------------------------------------------------------------------------
    latest_atmospheric_pressure = Current_Data_Record.field.atmospheric_pressure;
    // latest relative humidity ---------------------------------------------------------------------------------------
    latest_relative_humidity = Current_Data_Record.field.relative_humidity;
    // latest wind speed ----------------------------------------------------------------------------------------------
    latest_wind_speed = Current_Data_Record.field.wind_speed;
    // latest wind direction ------------------------------------------------------------------------------------------
    latest_wind_direction = Current_Data_Record.field.wind_direction;
    // latest weather description -------------------------------------------------------------------------------------
    latest_weather_description = Current_Data_Record.field.weather_description;
    // ----------------------------------------------------------------------------------------------------------------
    SD_freespace_double = (double)SD_freespace / 1000000;
    if (SD_freespace < critical_SD_freespace) {
        Write_Console_Message("\tWARNING - SD Free Space critical " + String(SD_freespace) + "MBytes");
    }
}
void Update_Webpage_Variables_from_Table(int Data_Table_Pointer) {
    if (readings_table[Data_Table_Pointer].field.voltage >= highest_voltage) {
        date_of_highest_voltage = String(readings_table[Data_Table_Pointer].field.ldate);
        time_of_highest_voltage = String(readings_table[Data_Table_Pointer].field.ltime);
        highest_voltage = readings_table[Data_Table_Pointer].field.voltage;
    }
    if ((readings_table[Data_Table_Pointer].field.voltage <= lowest_voltage) || !lowest_voltage) {
        date_of_lowest_voltage = String(readings_table[Data_Table_Pointer].field.ldate);
        time_of_lowest_voltage = String(readings_table[Data_Table_Pointer].field.ltime);
        //       console.print(millis(), DEC); console.print("\tUpdate Webpage Variables ltime : "); console.println(time_of_lowest_voltage);
        lowest_voltage = readings_table[Data_Table_Pointer].field.voltage;                     // update the largest current value
    }
    if (readings_table[Data_Table_Pointer].field.amperage >= highest_amperage) {               // load the maximum amperage value
        date_of_highest_amperage = String(readings_table[Data_Table_Pointer].field.ldate);
        time_of_highest_amperage = String(readings_table[Data_Table_Pointer].field.ltime);
        highest_amperage = readings_table[Data_Table_Pointer].field.amperage;                  // update the largest current value
    }
    latest_weather_temperature_feels_like = readings_table[Data_Table_Pointer].field.temperature_feels_like;
    latest_weather_temperature_maximum = readings_table[Data_Table_Pointer].field.temperature_maximum;
    latest_weather_temperature_minimum = readings_table[Data_Table_Pointer].field.temperature_minimum;
    latest_relative_humidity = readings_table[Data_Table_Pointer].field.relative_humidity;
    latest_atmospheric_pressure = readings_table[Data_Table_Pointer].field.atmospheric_pressure;
    latest_wind_speed = readings_table[Data_Table_Pointer].field.wind_speed;
    latest_wind_direction = readings_table[Data_Table_Pointer].field.wind_direction;
    latest_weather_description = readings_table[Data_Table_Pointer].field.weather_description;
}
void Prefill_Console_Array() {
    int console_character_count = 0;
    char console_txtField[120];
    int console_fieldNo = 1;
    char console_temp;
    Global_Data_Table_Pointer = 0;
    SD_Led_Flash_Start_Stop(true);
    File consoleFile = SD.open("/" + ConsoleFileName, FILE_READ);
    Write_Console_Message("Loading console file from " + String(ConsoleFileName));
    if (consoleFile) {
        console.print(millis(), DEC); console.print("\t");
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
                console.print(".");
                if ((Global_Console_Record_Count % 400) == 0) {
                    console.print(" ("); console.print(Global_Console_Record_Count); console.println(")");
                    console.print(millis(), DEC);
                    console.print("\t");
                }
                if (Global_Console_Table_Pointer > console_table_size) {                                // if pointer is greater than table size
                    for (int i = 0; i < console_table_size; i++) {                              // shuffle the rows up, losing row 0, make row [table_size] free
                        strncpy(console_table[i].ldate, console_table[i + 1].ldate, sizeof(console_table[i].ldate));             // date
                        strncpy(console_table[i].ltime, console_table[i + 1].ltime, sizeof(console_table[i].ltime));             // time
                        strncpy(console_table[i].message, console_table[i + 1].message, sizeof(console_table[i].message));
                    }
                    Global_Console_Table_Pointer = console_table_size;                                                  // subsequent records will be added at the end of the table
                }
                console_fieldNo = 1;
            }
        } // end of while
    }
    consoleFile.close();
    console.print(" ("); console.print(Global_Console_Record_Count); console.println(")");
    Write_Console_Message("Loaded Console Records: " + String(Global_Console_Record_Count));
    SD_Led_Flash_Start_Stop(false);
}
void Display() {
    double maximum_amperage = 0;
    double minimum_amperage = 0;
    double this_amperage = 0;
    Write_Console_Message("Web Display of Graph Requested via Webpage");
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
        if (String(readings_table[i].field.ltime) != "") {                  // if the ltime field contains data
            for (int y = 0; y < 8; y++) {                               // replace the ":"s in ltime with ","
                if (readings_table[i].field.ltime[y] == ':') {
                    readings_table[i].field.ltime[y] = ',';
                }
            }
            webpage += "[[";
            webpage += String(readings_table[i].field.ltime) + "],";
            webpage += String(readings_table[i].field.amperage, 1) + "]";
            this_amperage = readings_table[i].field.amperage;
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
    webpage += String((maximum_amperage + maximum_amperage / 10), 3);
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
    lastcall = "display";
}
void Web_Reset() {
    Write_Console_Message("Web Reset of Processor Requested via Webpage");
    ESP.restart();
}
void Page_Header(bool refresh, String Header) {
    webpage.reserve(5000);
    webpage = "";
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
    Update_TimeInfo(true);
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
    webpage += String(Sensor_Data.Time);
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
    server.send(200, "text/html", webpage);
    webpage = "";
}
void Information() {                                                 // Display file size of the datalog file
    int file_count = Count_Files_on_SD_Drive();
    Write_Console_Message("Web Display of Information Requested via Webpage");
    String ht = String(date_of_highest_voltage + " at " + time_of_highest_voltage + " as " + String(highest_voltage) + " volts");
    //    console.print(millis(), DEC); console.print("\tHT String: "); console.println(ht);
    String lt = String(date_of_lowest_voltage + " at " + time_of_lowest_voltage + " as " + String(lowest_voltage) + " volts");
    //    console.print(millis(), DEC); console.print("\tLT String: "); console.println(lt);
    String ha = String(date_of_highest_amperage + " at " + time_of_highest_amperage + " as " + String(highest_amperage) + " amps");
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
    double Percentage = (Global_Data_Record_Count * (double)100) / (double)17280;
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Number of Data Readings: ");
    webpage += String(Global_Data_Record_Count);
    webpage += F(" (");
    webpage += String(Percentage, 0);
    webpage += F("%), Number of Console Entries: ");
    webpage += String(Global_Console_Record_Count);
    webpage += "</span></strong></p>";
    // Highest Voltage ------------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Highest Voltage was recorded on ");
    webpage += ht;
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Lowest Voltage -------------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Lowest Voltage was recorded on ");
    webpage += lt;
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Highest Amperage ----------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Greatest Amperage was recorded on ");
    webpage += ha;
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
}
void Download_Files() {
    int file_count = Count_Files_on_SD_Drive();                                 // this counts and creates an array of file names on SD
    Write_Console_Message("Download of Files Requested via Webpage");
    Page_Header(false, "Energy Monitor Download Files");
    for (i = 1; i < file_count; i++) {
        webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(i) + " " + String(FileNames[i]) + " ";
        webpage += "&nbsp;<a href=\"/GetFile?file=" + String(FileNames[i]) + " " + "\">Download</a>";
        webpage += "</h3>";
    }
    Page_Footer();
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
}
void Console_Show() {
    Write_Console_Message("Web Display of Console Messages Requested via Webpage");
    Page_Header(true, "Console Messages");
    for (int x = 0; x < Global_Console_Table_Pointer; x++) {
        webpage += F("<p ");
        webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
        webpage += F("'>");
        webpage += "[" + String{ x } + "] ";
        webpage += String(console_table[x].ldate);
        webpage += F(" ");
        webpage += String(console_table[x].ltime);
        webpage += F(": ");
        webpage += String(console_table[x].message);
        webpage += "</span></strong></p>";
    }
    Page_Footer();
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
        case Request_Voltage: {                                                                     // [0]  Voltage
            double voltage = (value[3] << 8) + value[4];                                                    // Received is number of tenths of volt
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\tReceived Voltage (Raw): "); console.print(voltage, 4);
#endif
            voltage = voltage / (double)10;                                                                 // convert to volts
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t/10\t"); console.println(voltage, 4); ;
#endif
            Current_Data_Record.field.voltage = voltage;                                                    // Voltage output format double
            break;
        }
        case Request_Amperage: {                                                                    // [1]  Amperage
            double amperage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);           // Received is number of milli amps 
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\tReceived Amperage (Raw): "); console.print(amperage, 4);
#endif
            amperage /= (double)1000;                                                               // convert milliamps to amps
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t/1000\t"); console.println(amperage, 4);
#endif
            Current_Data_Record.field.amperage = amperage;                                                  // Amperage output format double nn.n
            break;
        }
        case Request_Wattage: {                                                                     // [2]  Wattage
            double wattage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);            // Recieved is number of tenths of watts
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\tReceived Wattage (Raw): "); console.print(wattage, 4);
#endif  
            // no conversion
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t\t"); console.println(wattage, 4);
#endif
            Current_Data_Record.field.wattage = wattage;
            break;
    }
        case Request_UpTime: {                                                                      //  [3] Uptime
            double uptime = (double)(value[3] << 8) + (double)value[4];
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\tReceived Uptime (Raw): "); console.print(uptime, 4);
#endif
            uptime = uptime / (double)60;                                                                   // convert to hours
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t/60\t"); console.println(uptime, 4);
#endif
            Current_Data_Record.field.uptime = uptime;
            break;
}
        case Request_Kilowatthour: {                                                                //  [4] KilowattHour
            double kilowatthour = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\tReceived Kilowatthour (Raw): "); console.print(kilowatthour, 4);
#endif
            kilowatthour = kilowatthour / (double)1000;
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t/1000\t"); console.println(kilowatthour, 4);
#endif
            Current_Data_Record.field.kilowatthour = kilowatthour;
            break;
        }
        case Request_Power_Factor: {                                                                //  [5] Power Factor
            double powerfactor = (double)(value[3] << 8) + (double)value[4];
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\tReceived Power Factor (Raw): "); console.print(powerfactor, 4);
#endif
            powerfactor = powerfactor / (double)100;
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t/100\t"); console.println(powerfactor, 4);
#endif
            Current_Data_Record.field.powerfactor = powerfactor;
            break;
        }
        case Request_Frequency: {                                                                   //  [7] Frequency
            double frequency = (double)(value[3] * (double)256) + (double)value[4];
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\tReceived Frequency (Raw): "); console.print(frequency, 4);
#endif
            frequency = frequency / (double)10;
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t/10\t"); console.println(frequency, 4);
#endif
            Current_Data_Record.field.frequency = frequency;
            break;
        }
        case Request_Sensor_Temperature: {                                                          // [8] Sensor Temperature
            double temperature = (double)(value[3] << 8) + (double)value[4];
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\tReceived Sensor Temperature (Raw): "); console.print(temperature, 4);
#endif
            // no conversion
#ifdef DISPLAY_DATA_VALUES_COLLECTED 
            console.print("\t\t"); console.println(temperature, 4);
#endif
            Current_Data_Record.field.sensor_temperature = temperature;
            break;
        }
        default: {
            console.print(millis(), DEC); console.println("\tRequested Fields != Received Fields");
            break;
        }
        }
        for (int x = 0; x < 10; x++) {
            Current_Data_Record.field.ldate[x] = Sensor_Data.Date[x];
        }
        for (int x = 0; x < 8; x++) {
            Current_Data_Record.field.ltime[x] = Sensor_Data.Time[x];
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
    }
    for (int x = 0; x <= data_table_size; x++) {
        for (int y = 0; y < packet_length; y++)
            readings_table[x].characters[y] = '0';
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
void Update_TimeInfo(bool format) {
    int connection_attempts = 0;
    String temp;
    while (!getLocalTime(&timeinfo)) {                                                  // get date and time from ntpserver
        Write_Console_Message("Attempting to Get Date " + String(connection_attempts));
        delay(500);
        Check_Red_Switch();
        connection_attempts++;
        if (connection_attempts > 20) {
            Write_Console_Message("Time Network Error, Restarting");
            ESP.restart();
        }
    }
    Sensor_Data.Year = timeinfo.tm_year + 1900;
    Sensor_Data.Month = timeinfo.tm_mon + 1;
    Sensor_Data.Day = timeinfo.tm_mday;
    Sensor_Data.Hour = timeinfo.tm_hour;
    Sensor_Data.Minute = timeinfo.tm_min;
    Sensor_Data.Second = timeinfo.tm_sec;
    // ----------------------------------------------------------------------------------------------------------------
    Sensor_Data.Date = String(Sensor_Data.Year);            //  1951
    if (format) {
        Sensor_Data.Date += "/";                            //  1951/
    }
    if (Sensor_Data.Month < 10) {
        Sensor_Data.Date += "0";                            //  1951/0
    }
    Sensor_Data.Date += String(Sensor_Data.Month);          //  1951/11
    if (format) {
        Sensor_Data.Date += "/";                            //  1951/11/
    }
    if (Sensor_Data.Day < 10) {
        Sensor_Data.Date += "0";                            //  1951/11/0
    }
    Sensor_Data.Date += String(Sensor_Data.Day);            //  1951/11/18
    // ----------------------------------------------------------------------------------------------------------------
    Sensor_Data.Time = String(Sensor_Data.Hour);            //  23
    if (format) {
        Sensor_Data.Time += ":";                            //  23:
    }
    if (Sensor_Data.Hour < 10) {
        Sensor_Data.Time += "0";                            //  23:0
    }
    Sensor_Data.Time += String(Sensor_Data.Minute);         //  23:59
    if (format) {
        Sensor_Data.Time += ":";                            //  23:59:
    }
    if (Sensor_Data.Second < 10) {
        Sensor_Data.Time += "0";                            //  23:59:0
    }
    Sensor_Data.Time += String(Sensor_Data.Second);         //  23:59:59
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
    Current_Data_Record.field.weather_temperature = (double)(atof(Parse_Output)) - (double)273.15;
#ifdef DISPLAY_WEATHER_INFORMATION
    console.print(millis(), DEC);
    console.print("\tParsed Weather Temperature: ");
    console.println(Current_Data_Record.field.weather_temperature, DEC);
#endif
    // Temperature Feels Like -----------------------------------------------------------------------------------------
    start = payload.indexOf("feels_like");              // "feels_like":283.47,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Current_Data_Record.field.temperature_feels_like = (double)(atof(Parse_Output)) - (double)273.15;
#ifdef DISPLAY_WEATHER_INFORMATION
    console.print(millis(), DEC);
    console.print("\tParsed Temperature Feels Like: ");
    console.println(Current_Data_Record.field.temperature_feels_like, DEC);
#endif
    // Temperature Maximum --------------------------------------------------------------------------------------------
    start = payload.indexOf("temp_max");                // "temp_max":284.89,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Current_Data_Record.field.temperature_maximum = (double)(atof(Parse_Output)) - (double)273.15;
#ifdef DISPLAY_WEATHER_INFORMATION
    console.print(millis(), DEC);
    console.print("\tParsed Temperature Maximum: ");
    console.println(Current_Data_Record.field.temperature_maximum, DEC);
#endif
    // Temperature Minimum --------------------------------------------------------------------------------------------
    start = payload.indexOf("temp_min");                // "temp_min":282.75,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Current_Data_Record.field.temperature_minimum = (double)(atof(Parse_Output)) - (double)273.15;
#ifdef DISPLAY_WEATHER_INFORMATION
    console.print(millis(), DEC);
    console.print("\tParsed Temperature Minimum: ");
    console.println(Current_Data_Record.field.temperature_minimum, DEC);
#endif
    // Pressure -------------------------------------------------------------------------------------------------------
    start = payload.indexOf("pressure");                // "pressure":1018,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Current_Data_Record.field.atmospheric_pressure = (double)atof(Parse_Output);
#ifdef DISPLAY_WEATHER_INFORMATION
    console.print(millis(), DEC);
    console.print("\tParsed Atmospheric Pressure: ");
    console.println(Current_Data_Record.field.atmospheric_pressure, DEC);
#endif
    // humidity -------------------------------------------------------------------------------------------------------
    start = payload.indexOf("humidity\":");             // "humidity":95}
    start = payload.indexOf(":", start);
    end = payload.indexOf("}", start);
    parse(payload, start, end);
    Current_Data_Record.field.relative_humidity = (double)atof(Parse_Output);
#ifdef DISPLAY_WEATHER_INFORMATION
    console.print(millis(), DEC);
    console.print("\tParsed Relative Humidity: ");
    console.println(Current_Data_Record.field.relative_humidity, DEC);
#endif
    // weather description --------------------------------------------------------------------------------------------
    start = payload.indexOf("description");             // "description":"overcast clouds",
    start = (payload.indexOf(":", start) + 1);
    end = (payload.indexOf(",", start) - 1);
    parse(payload, start, end);
    strncpy(Current_Data_Record.field.weather_description, Parse_Output, sizeof(Current_Data_Record.field.weather_description));
#ifdef DISPLAY_WEATHER_INFORMATION
    console.print(millis(), DEC);
    console.print("\tParsed Weather Description: ");
    console.println(Current_Data_Record.field.weather_description);
#endif
    // wind speed -----------------------------------------------------------------------------------------------------
    start = payload.indexOf("speed");                       // "speed":2.57,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Current_Data_Record.field.wind_speed = (double)(atof(Parse_Output));
#ifdef DISPLAY_WEATHER_INFORMATION
    console.print(millis(), DEC);
    console.print("\tParsed Wind Speed: ");
    console.println(Current_Data_Record.field.wind_speed, DEC);
#endif
    // wind direction -------------------------------------------------------------------------------------------------
    start = payload.indexOf("deg");                         // "deg":20
    start = payload.indexOf(":", start);
    end = payload.indexOf("}", start);
    parse(payload, start, end);
    Current_Data_Record.field.wind_direction = (double)atof(Parse_Output);
#ifdef DISPLAY_WEATHER_INFORMATION
    console.print(millis(), DEC);
    console.print("\tParsed Wind Direction: ");
    console.println(Current_Data_Record.field.wind_direction, DEC);
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