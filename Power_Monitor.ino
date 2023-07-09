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
21/01/2023  11.1 Coverted receive milli Amperage to Amperage in amps.
25/01/2023  11.2 Removed milliseconds from console records
28/01/2023  11.3 Fixed Time to cope with hours <10 & changed width to show the y axis labels
15/03/2023  11.4 Gas Meter Pulse Output monitoring added, 1 pulse = 0.1 cubic metres of gas used
16/04/2023  11.5 Added creation of monthly file
07/05/2023  11.6 Added watchdog timer, reboots if processor lock or no WiFi for over 20 seconds
07/07/2023  12.0 Removed console file and tidy up
*/
// Compiler Directives ------------------------------------------------------------------------------------------------
//#define PRINT_PREFILL_RECORDS
// --------------------------------------------------------------------------------------------------------------------
#include <ESP32Time.h>
String version = "V12.0";                       // software version number, shown on webpage
// compiler directives ------------------------------------------------------------------------------------------------
//#define PRINT_PREFILL_DATA_VALUES           //
//#define PRINT_SHUFFLING_DATA_VALUES
#define PRINT_MONTH_DATA_VALUES
// definitions --------------------------------------------------------------------------------------------------------
#define console Serial
#define RS485_Port Serial2
#define WDT_TIMEOUT 3                           // 3 second watchdog timeout
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
#include <ESP32Time.h>
//#include <time.h>
#include <Bounce2.h>
#include <Uri.h>
#include <HTTP_Method.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <stdio.h>
#include <ArduinoSort.h>
#include <esp_task_wdt.h>
// --------------------------------------------------------------------------------------------------------------------
constexpr int eeprom_size = 30;          // year = 4, month = 4, day = 4, hour = 4, minute = 4, second = 4e
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
// ESP32 Pin Definitions ----------------------------------------------------------------------------------------------
constexpr int Blue_Switch_pin = 32;
constexpr int Red_Switch_pin = 33;
constexpr int Green_Switch_pin = 27;
constexpr int Green_led_pin = 26;
constexpr int Yellow_Switch_pin = 0;
constexpr int Blue_led_pin = 4;
constexpr int RS485_Enable_pin = 22;
constexpr int RS485_TX_pin = 17;
constexpr int RS485_RX_pin = 16;
constexpr int SS_pin = 5;
constexpr int SCK_pin = 18;
constexpr int MISO_pin = 19;
constexpr int MOSI_pin = 23;
constexpr int ONBOARDLED = 2;
constexpr int Gas_Switch_pin = 21;
constexpr double Gas_Volume_Per_Sensor_Rise = (double).10;
// Date and Time Fields -----------------------------------------------------------------------------------------------0
struct Date_Time {
    int Second;                     // [0 - 3]
    int Minute;                     // [4 - 7]
    int Hour;                       // [8 - 11]
    int Day;                        // [12 - 15]
    int Month;                      // [16 - 19]
    int Year;                       // [20 - 23]
    double Gas_Volume;              // [24 - 27]
}__attribute__((packed));
constexpr int Date_Time_Record_Length = 27;
union Date_Time_Union {
    Date_Time field;
    unsigned char character[Date_Time_Record_Length + 1];
};
Date_Time_Union Current_Date_Time_Data;
Date_Time_Union Last_Gas_Date_Time_Data;
String Current_Date_With = "1951/11/18";
String Current_Date_Without = "19511118";
String Current_Time_With = "00:00:00";
String Current_Time_Without = "000000";
String Latest_Gas_Date_With = "1951/11/18";
String Latest_Gas_Time_With = "00:00:00";
double Latest_Gas_Volume = 0;
String This_Date_With = "1951/11/18";
String This_Date_Without = "19511116";
String This_Time_With = "00:00:00";
String This_Time_Without = "000000";
constexpr int Days_n_Month[13][2] = {
    // Leap,Normal
        {00,00},
        {31,31},    // January
        {29,28},    // February
        {31,31},    // March                         
        {30,30},    // April
        {31,31},    // May
        {30,30},    // June
        {31,31},    // July
        {31,31},    // August
        {30,30},    // September
        {31,31},    // October
        {30,30},    // November
        {31,31},    // December
};
// Instantiations -----------------------------------------------------------------------------------------------------
struct tm timeinfo;
Bounce Red_Switch = Bounce();
Bounce Green_Switch = Bounce();
Bounce Blue_Switch = Bounce();
Bounce Yellow_Switch = Bounce();
Bounce Gas_Switch = Bounce();
hw_timer_t* Timer0_Cfg = NULL;
WebServer server(80);                   // WebServer(HTTP port, 80 is defAult)
WiFiClient client;
HTTPClient http;
// DataFile constants and variables -----------------------------------------------------------------------------------
File DataFile;                         // Full data file, holds all readings from KWS-AC301L
String DataFileName = "20220101";           // .cvs
constexpr int Number_of_Column_Field_Names = 20;                // indicate the available addresses [0 - 19]
String Column_Field_Names[Number_of_Column_Field_Names] = {
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
                        "Gas Volume",               // [18]
                        "Weather Description",      // [19]
};
struct Data_Record_Values {
    char ldate[11];                 //  [0 - 10]  [00]  date record was taken
    char ltime[9];                  //  [11 - 19] [01]  time record was taken
    double Voltage;                 //  [20 - 23] [02]
    double Amperage;                //  [24 - 27] [03]
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
    double gas_volume;              //  [84 - 87] [18]  gas volume 
    char weather_description[20];   //  [88 - 110] [19] weather description
}__attribute__((packed));
constexpr int Data_Record_Length = 111;
union Data_Record_Union {
    Data_Record_Values field;
    unsigned char character[Data_Record_Length];
};
Data_Record_Union Current_Data_Record;
Data_Record_Union Month_Data_Record;
//String Data_File_Values_19 = "                                   ";     // [24] space for the read weather descriptionconstexpr int Data_Table_Size = 60;             // number of data table rows (5 minutes worth)
int Data_Table_Pointer = 0;              // points to the next index of Data_Table
int Data_Record_Count = 0;               // running total of records written to Data Table
// Data Table Constants and Variables ---------------------------------------------------------------------------------
constexpr char Data_Table_Size = 60;
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
    unsigned char characters[Data_Table_Record_Length];
};
Data_Table_Union Readings_Table[Data_Table_Size];
unsigned long Data_Record_Start_Time = 0;
unsigned long First_Data_Record_Start_Time = 0;
/// MonthFile Constants and Variables ----------------------------------------------------------------------------------
File MonthFile;
String MonthFileName = "M202301";           // .cvs
// DateTimeFile Constants and Variables -------------------------------------------------------------------------------
//File DateTimeFile;
//String DateTimeFileName = "DT01";           // .txt
// KWS Request Field Numbers ------------------------------------------------------------------------------------------
constexpr int Request_Voltage = 0;
constexpr int Request_Amperage = 1;
constexpr int Request_Wattage = 2;
constexpr int Request_UpTime = 3;
constexpr int Request_Kilowatthour = 4;
constexpr int Request_Power_Factor = 5;
constexpr int Request_Frequency = 6;
constexpr int Request_Sensor_Temperature = 7;
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
//char print_buffer[80];
String FileNames[50];
bool Yellow_Switch_Pressed = false;
bool Green_Switch_Pressed = false;
bool Blue_Switch_Pressed = false;
String site_width = "1060";                     // width of web page
String site_height = "600";                     // height of web page
String webpage;
String lastcall;
double temperature_calibration = (double)16.5 / (double)22.0;   // temperature reading = 22, actual temperature = 16.5
String Last_Boot_Time_With = "12:12:12";
String Last_Boot_Date_With = "2022/29/12";
int This_Day = 0;
double Previous_Gas_Volume = 0;

// Lowest Voltage -----------------------------------------------------------------------------------------------------
double Lowest_Voltage = 0;
String Time_of_Lowest_Voltage = "00:00:00";
String Date_of_Lowest_Voltage = "0000/00/00";
// Highest Voltage ----------------------------------------------------------------------------------------------------
double Highest_Voltage = 0;
String Time_of_Highest_Voltage = "00:00:00";
String Date_of_Highest_Voltage = "0000/00/00";
// Highest Amperage ---------------------------------------------------------------------------------------------------
double Highest_Amperage = 0;
String Time_of_Highest_Amperage = "00:00:00";
String Date_of_Highest_Amperage = "0000/00/00";
// Daily total KiloWattHours ------------------------------------------------------------------------------------------
double Cumulative_kwh = 0;
// Time of Latest Weather Reading -------------------------------------------------------------------------------------
String Time_of_Latest_reading = "00:00:00";
// Latest Weather Temperature -----------------------------------------------------------------------------------------
double Latest_weather_temperature = 0;
// Latest Temperature Feels Like --------------------------------------------------------------------------------------
double Latest_weather_temperature_feels_like = 0;
// Latest Temperature Maximum -----------------------------------------------------------------------------------------
double Latest_weather_temperature_maximum = 0;
// Latest Temperature Minimum -----------------------------------------------------------------------------------------
double Latest_weather_temperature_minimum = 0;
// Latest Atmospheric Pressure ----------------------------------------------------------------------------------------
double Latest_atmospheric_pressure = 0;
// Latest Relative Humidity -------------------------------------------------------------------------------------------
double Latest_relative_humidity = 0;
// Latest Wind Speed --------------------------------------------------------------------------------------------------
double Latest_wind_speed = 0;
// Latest Wind Direction ----------------------------------------------------------------------------------------------
double Latest_wind_direction = 0;
// Latest Weather Description -----------------------------------------------------------------------------------------
String Latest_weather_description = "                                     ";
// -------------------------------------------------------------------------------------------------------------------
constexpr int Days_in_Month[13][2] = {
    // Leap,Normal
        {00,00},    // 
        {31,31},    // Jan 31/31
        {29,28},    // Feb 29/28
        {31,30},    // Mar 31/31
        {30,30},    // Apr 30/30
        {31,31},    // May 31/31
        {30,30},    // Jun 30/30
        {31,31},    // Jul 31/31
        {31,31},    // Aug 31/31
        {30,30},    // Sep 30/30
        {31,31},    // Oct 31/31
        {30,30},    // Nov 30/30
        {31,31}     // Dec 31/31
};
// --------------------------------------------------------------------------------------------------------------------
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
bool Setup_Complete = false;
char complete_weather_api_link[120];
int i = 0;
char Parse_Output[25];
bool New_Day_File_Required = true;
bool Month_File_Required = true;
unsigned long sd_off_time = 0;
unsigned long sd_on_time = 0;
int WiFi_Signal_Strength = 0;
int WiFi_Retry_Counter = 0;
// Debug / Test Variables ---------------------------------------------------------------------------------------------
bool Once = false;
// setup --------------------------------------------------------------------------------------------------------------
void setup() {
    console.begin(console_Baudrate);                                            // enable the console
    while (!console);                                                           // wait for port to settle
    delay(4000);
    console.print(millis(), DEC); console.println("\tCommencing Setup");
    console.print(millis(), DEC); console.println("\tBooting - Commencing Setup");
    console.print(millis(), DEC); console.println("\tConfiguring Watchdog Timer");
    //    esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
    //    esp_task_wdt_add(NULL); //add current thread to WDT watch
    console.print(millis(), DEC); console.println("\tConfiguring IO");
    pinMode(Blue_led_pin, OUTPUT);
    pinMode(Green_Switch_pin, INPUT_PULLUP);
    pinMode(Red_Switch_pin, INPUT_PULLUP);
    pinMode(Blue_Switch_pin, INPUT_PULLUP);
    pinMode(Yellow_Switch_pin, INPUT_PULLUP);
    pinMode(Green_led_pin, OUTPUT);
    pinMode(Gas_Switch_pin, INPUT_PULLUP);
    Gas_Switch.attach(Gas_Switch_pin);
    Red_Switch.attach(Red_Switch_pin);     // setup defaults for debouncing switches
    Green_Switch.attach(Green_Switch_pin);
    Blue_Switch.attach(Blue_Switch_pin);
    Yellow_Switch.attach(Yellow_Switch_pin);
    Gas_Switch.interval(100);
    Red_Switch.interval(5);                  // sets debounce time
    Green_Switch.interval(5);
    Blue_Switch.interval(5);
    Gas_Switch.update();
    Red_Switch.update();
    Green_Switch.update();
    Blue_Switch.update();
    digitalWrite(0, HIGH);
    digitalWrite(Green_led_pin, LOW);
    digitalWrite(Blue_led_pin, LOW);
    console.print(millis(), DEC); console.println("\tIO Configuration Complete");
    // WiFi and Web Setup -------------------------------------------------------------------------
    StartWiFi(ssid, password);                      // Start WiFi
    InitTime("GMT0BST, M3.5.0 / 1, M10.5.0");       // Initialise the Time library to London
    console.print(millis(), DEC); console.println("\tStarting Server");
    server.begin();                                 // Start Webserver
    console.print(millis(), DEC); console.println("\tServer Started");
    server.on("/", Display);                        // nothing specified so display main web page
    server.on("/Display", Display);                 // display the main web page
    server.on("/Information", Information);         // display information
    server.on("/DownloadFiles", Download_Files);    // select a file to download
    server.on("/GetFile", Download_File);           // download the selectedfile
    server.on("/DeleteFiles", Delete_Files);        // select a file to delete
    server.on("/DelFile", Del_File);                // delete the selected file
    server.on("/Reset", Web_Reset);                 // reset the orocessor from the webpage
    RS485_Port.begin(RS485_Baudrate, SERIAL_8N1, RS485_RX_pin, RS485_TX_pin);
    pinMode(RS485_Enable_pin, OUTPUT);
    delay(10);
    if (!SD.begin(SS_pin)) {
        console.print(millis(), DEC); console.print("SD Drive Begin Failed");
        while (true) {
            digitalWrite(Green_led_pin, !digitalRead(Green_led_pin));
            digitalWrite(Blue_led_pin, !digitalRead(Blue_led_pin));
            delay(500);
            Check_Red_Switch();
        }
    }
    else {
        console.print(millis(), DEC); console.println("\tSD Drive Begin Succeeded");
        uint8_t cardType = SD.cardType();
        while (SD.cardType() == CARD_NONE) {
            console.print(millis(), DEC); console.println("\tNo SD Card Found");
            while (true) {
                digitalWrite(Green_led_pin, !digitalRead(Green_led_pin));
                digitalWrite(Blue_led_pin, !digitalRead(Blue_led_pin));
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
        console.print(millis(), DEC); console.print("\tSD Card Type: "); console.println(card);;
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        critical_SD_freespace = cardSize * (uint64_t).9;
        console.print(millis(), DEC); console.println("\tSD Card Size : " + String(cardSize) + "MBytes");
        console.print(millis(), DEC); console.println("\tSD Total Bytes : " + String(SD.totalBytes()));
        console.print(millis(), DEC); console.println("\tSD Used bytes : " + String(SD.usedBytes()));
        console.print(millis(), DEC); console.println("\tSD Card Initialisation Complete");
    }
    Update_Current_TimeInfo();                                                  // update This date time info, no /s
    Last_Boot_Time_With = Current_Time_With;
    Last_Boot_Date_With = Current_Date_With;
    This_Date_With = Current_Date_With;
    This_Time_With = Current_Time_With;
    Create_New_Data_File();
    console.print(millis(), DEC); console.println("\tPreparing Customised Weather Request");
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
    console.print(millis(), DEC); console.println("\tWeather Request Created");
    digitalWrite(Blue_led_pin, LOW);
    console.print(millis(), DEC); console.println("\tEnd of Setup");
    console.print(millis(), DEC); console.println("\tRunning in Full Function Mode");
}   // end of Setup
//---------------------------------------------------------------------------------------------------------------------
void loop() {
    if (!Once) {
        console.print(millis(), DEC); console.println("\tProcess Now Running");
        Once = true;
        digitalWrite(Green_led_pin, HIGH);
    }
    esp_task_wdt_reset();                                   // reset the watchdog timer every loop
    Check_WiFi();                                           // check that the WiFi is still conected
    Check_Green_Switch();                                   // check if start switch has been pressed
    Check_Blue_Switch();                                    // check if wipesd switch has been pressed
    Check_Gas_Switch();                                     // check if the gas switch sense pin has risen
    Check_NewDay();                                         // check if the date has changed
    server.handleClient();                                  // handle any messages from the website
    if (millis() > last_cycle + (unsigned long)5000) {      // send requests every 5 seconds (5000 millisecods)
        console.print(millis(), DEC); console.println("\tRequesting Current Information");
        last_cycle = millis();                              // update the last read milli second reading
        //    weather information request start --------------------------------------------------------------------------
        HTTPClient http;
        http.begin(complete_weather_api_link);              // start the weather connectio
        int httpCode = http.GET();                          // send the request
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                Parse_Weather_Info(payload);
            }
            else {
                console.print(millis(), DEC); console.print("Obtaining Weather Information Failed, Return code: "); console.println(String(httpCode));
            }
            http.end();
        }
        // weather information request end ----------------------------------------------------------------------------
        // sensor information request start -------------------------------------------------------------------------
        for (int i = 0; i <= Number_of_RS485_Requests; i++) {    // transmit the requests, assembling the Values array
            Send_Request(i);                                    // send the RS485 Port the requests, one by one
            Receive(i);                                         // receive the sensor output
        }                                                       // all values should now be populated
            // sensor information request end -----------------------------------------------------------------------------
        Update_Current_TimeInfo();
        Current_Date_With.toCharArray(Current_Data_Record.field.ldate, Current_Date_With.length() + 1);
        Current_Time_With.toCharArray(Current_Data_Record.field.ltime, Current_Time_With.length() + 1);
        Write_New_Data_Record_to_Data_File();                   // write the new record to SD Drive
        Add_New_Data_Record_to_Display_Table();                 // add the record to the display table
    }// end of if millis >5000
}
//---------------------------------------------------------------------------------------------------------------------
void Check_NewDay() {
    Update_Current_TimeInfo();                                                      // update the Date and Time
    if ((This_Date_With != Current_Date_With) && New_Day_File_Required == true) {   // check we are inn the same day as setup
        console.print(millis(), DEC);
        console.print("\tNew Day Process Commenced - This Date:");
        console.print(This_Date_With);
        console.print(" Now Date: ");
        console.print(Current_Date_With);
        console.print("New_Day_File_Required: ");
        console.println(String(New_Day_File_Required));
        Check_New_Month();                                  // check if this is the end of the month
        Create_New_Data_File();                             // no, so create a new Data File with new file name
        Clear_Arrays();                                     // clear memory
        This_Date_With = Current_Date_With;
        New_Day_File_Required = false;                      // this flag ensures that we only create a new day file once each day
    }
    else {
        New_Day_File_Required = true;                       // reset the flag
    }
}
void Check_New_Month() {                                    // Only executed when change of day is going to happen.
    if (Current_Date_Time_Data.field.Day == 1 && Month_File_Required == true) {   // is this the first day of the month?
        console.print(millis(), DEC); console.print("New Month Detected");
        Month_File_Required = false;                                                // ensure this is done only once per mnnth
        Month_End_Process();
    }
    else {
        Month_File_Required = true;
    }
}
void Month_End_Process() {
    int character_count = 0;
    char Field[50];
    int datafieldNo = 0;
    char datatemp;
    String message;
    int file_count = 0;
    String previous_year = "0000";
    String previous_month = "00";
    String csv_file_names[31];                              // possibly 31 csv files
    String this_file_name = "        ";
    int csv_count = 0;
    char datatemp = 0;
    // Stage 1 Create sorted file name arrays ---------------------------------------------------------------------
    console.print(millis(), DEC); console.print("Stage 1 - Create an array with the .csv files name");
    file_count = Count_Files_on_SD_Drive();                         // creates an array of filenames
    for (int file = 0; file < file_count; file++) {                 // for each file name
        this_file_name = FileNames[file];                           // take the file name
        if (FileNames[file].indexOf(".csv", 1)) {                   // is it a .csv file
            this_file_name = FileNames[file];                       // take the full csv file name
            for (int x = 0; x < 8; x++) {
                csv_file_names[csv_count] += this_file_name[x];     // move the filename, minus the extension
            }
            csv_count++;                                            // increment the count of csv file names
        }
    }
    console.print(millis(), DEC); console.print("\tThere are " + String(csv_count) + ".csv files");
    console.print(millis(), DEC); console.print("\tStage 2 - Sort the .csv file names into date order");
    sortArray(csv_file_names, --csv_count);             // sort into rising order
    // Stage 2 ----------------------------------------------------------------------------------------------------
    Update_Current_TimeInfo();
    for (int x = 0; x < 4; x++) {
        previous_year[x] = This_Date_With[x];
    }
    for (int x = 5; x < 7; x++) {
        previous_month[x] = This_Date_With[x];
    }
    // Create Monthly .csv file -----------------------------------------------------------------------------------
    MonthFileName = "M" + previous_year + previous_month + ".csv";
    console.print(millis(), DEC); console.print("Stage 3 - Create the Monthly .csv file " + MonthFileName);
    if (!SD.exists("/" + MonthFileName)) {               // create the monthly file
        MonthFile = SD.open("/" + MonthFileName, FILE_WRITE);
        if (!MonthFile) {                        // log file not opened
            console.print(millis(), DEC); console.print("Error opening Month file in Create New Data File (" + String(MonthFileName) + ")");
            Check_Red_Switch();
        }
        console.print(millis(), DEC); console.print("\tMonth File " + MonthFileName + " created");

        for (int x = 0; x < Number_of_Column_Field_Names; x++) {                   // write data column headings into the SD file
            MonthFile.print(Column_Field_Names[x]);
            MonthFile.print(",");
        }
        MonthFile.println(Column_Field_Names[Number_of_Column_Field_Names]);
        console.print(millis(), DEC); console.print("\tColumn Titles written to new Month File");

    }
    else {
        console.print(millis(), DEC); console.print("\tMonth File " + String(MonthFileName) + " already exists");     // file already exists
        MonthFile = SD.open("/" + MonthFileName, FILE_WRITE);
        if (!MonthFile) {                                                                   // log file not opened
            console.print(millis(), DEC); console.print("Error opening Month file in Create New Data File (" + String(MonthFileName) + ")");
            Check_Red_Switch();
        }
        console.print("has been opened, and will remain open until all Data Files have been copied");
    }
    console.print(millis(), DEC); console.print("\tAmalgamating Daily .csv files into Monthly .csv file");
    for (int file = 0; file < csv_count; file++) {                                          // for each csv file
        esp_task_wdt_reset();                                                               // reset the watchdog timer
        DataFile = SD.open("/" + csv_file_names[file] + ".csv", FILE_READ);
        console.print(millis(), DEC); console.println("\tCreate Month File - Skipping Column Heading Row");
        while (DataFile.available()) {                                                      // throw the first row, column headers, away
            datatemp = DataFile.read();
            if (datatemp == '\n') break;
        }
        console.print(millis(), DEC); console.println("\tReading Data Records");
        while (DataFile.available()) {                                                      // do while there are data available
            datatemp = DataFile.read();                                                     // open the SD file
            console.print(millis(), DEC); console.print("\tProcessing " + csv_file_names[file] + ".csv");
            if (!DataFile) {                                                                // oops - file not available!
                console.print(millis(), DEC); console.print("Error re-opening DataFile:" + String(DataFileName));
                Check_Red_Switch();                                                         // Reset will restart the processor so no return
            }
            else {
                if (DataFile) {
                    Field[character_count++] = datatemp;                                    // add it to the csvfield string
                    if (datatemp == ',' || datatemp == '\n') {                                  // look for end of field
                        Field[character_count - 1] = '\0';                                      // insert termination character where the ',' or '\n' was
                        switch (datafieldNo) {
                        case 0: {
                            strncpy(Month_Data_Record.field.ldate, Field, sizeof(Month_Data_Record.field.ldate));       // Date
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Date: ";
                            message += Month_Data_Record.field.ldate;
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 1: {
                            strncpy(Month_Data_Record.field.ltime, Field, sizeof(Month_Data_Record.field.ltime));       // Time
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Time: ";
                            message += Month_Data_Record.field.ltime;
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 2: {
                            Month_Data_Record.field.Voltage = atof(Field);                                              // Voltage
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Voltage: ";
                            message += String(Month_Data_Record.field.Voltage);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 3: {
                            Month_Data_Record.field.Amperage = atof(Field);                                             // Amperage
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Amperage: ";
                            message += String(Month_Data_Record.field.Amperage);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 4: {
                            Month_Data_Record.field.wattage = atof(Field);                                              // Wattage
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Wattage: ";
                            message += String(Month_Data_Record.field.wattage);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 5: {
                            Month_Data_Record.field.uptime = atof(Field);                                               // Uptime
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Uptime: ";
                            message += String(Month_Data_Record.field.uptime);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif
                            break;
                        }
                        case 6: {
                            Month_Data_Record.field.kilowatthour = atof(Field);                                         // Kilowatthour
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Kilowatthours: ";
                            message += String(Month_Data_Record.field.kilowatthour);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 7: {
                            Month_Data_Record.field.powerfactor = atof(Field);                                          // Power Factor
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Power Factor: ";
                            message += String(Month_Data_Record.field.powerfactor);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 8: {
                            Month_Data_Record.field.frequency = atof(Field);                                            // Frequency
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Frequency: ";
                            message += String(Month_Data_Record.field.frequency);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif
                            break;
                        }
                        case 9: {
                            Month_Data_Record.field.sensor_temperature = atof(Field);                                   // Sensor Temperature
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Sensor Temperature: ";
                            message += String(Month_Data_Record.field.sensor_temperature);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 10: {
                            Month_Data_Record.field.weather_temperature = atof(Field);                                  // Weather Temperature
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Weather Temperature: ";
                            message += String(Month_Data_Record.field.weather_temperature);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 11: {
                            Month_Data_Record.field.temperature_feels_like = atof(Field);                               // Temperatre Feels Like
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Temperature Feels Like: ";
                            message += String(Month_Data_Record.field.temperature_feels_like);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 12: {
                            Month_Data_Record.field.temperature_maximum = atof(Field);                                  // Temperature Maximum
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Maximum Temperatre: ";
                            message += String(Month_Data_Record.field.temperature_maximum);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 13: {
                            Month_Data_Record.field.temperature_minimum = atof(Field);                                  // Temperature Minimum
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth_Data: Temperature Minimum: ";
                            message += String(Month_Data_Record.field.temperature_minimum);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 14: {
                            Month_Data_Record.field.atmospheric_pressure = atof(Field);                                 // Atmospheric Pressure
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Atmospheric Pressure: ";
                            message += String(Month_Data_Record.field.atmospheric_pressure);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 15: {
                            Month_Data_Record.field.relative_humidity = atof(Field);                                    // Relative Humidity
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Relative Humidity: ";
                            message += String(Month_Data_Record.field.relative_humidity);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 16: {
                            Month_Data_Record.field.wind_speed = atof(Field);                                           // Wind Speed
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Wind Speed: ";
                            message += String(Month_Data_Record.field.wind_speed);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 17: {
                            Month_Data_Record.field.wind_direction = atof(Field);                                       // Wind Direction
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Wind Direction: ";
                            message += String(Month_Data_Record.field.wind_direction);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif            
                            break;
                        }
                        case 18: {
                            Month_Data_Record.field.gas_volume = atof(Field);                                           // Gas Volume
                            if (Month_Data_Record.field.gas_volume == 0) {                  // if it is zero (first record of each data file
                                Month_Data_Record.field.gas_volume += Previous_Gas_Volume;          // make the gas volume monthly cumulative
                            }
                            Previous_Gas_Volume = Month_Data_Record.field.gas_volume;               // save the new cumulative volume
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Gas Volume: ";
                            message += String(Month_Data_Record.field.gas_volume);
                            console.print(millis(), DEC); console.println("\t" + message);
#endif          
                            break;
                        }
                        case 19: {
                            strncpy(Month_Data_Record.field.weather_description, Field, sizeof(Month_Data_Record.field.weather_description));
#ifdef PRINT_MONTH_DATA_VALUES
                            message = "\tMonth Data: Weather Description: ";
                            message += String(Month_Data_Record.field.weather_description);
                            console.print(millis(), DEC); console.println("\t" + message);
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
                    if (datatemp == '\n') {
                        datafieldNo = 0;
                        MonthFile.print(Month_Data_Record.field.ldate); MonthFile.print(",");                   // [00] Date
                        MonthFile.print(Month_Data_Record.field.ltime); MonthFile.print(",");                   // [01] Time
                        MonthFile.print(Month_Data_Record.field.Voltage); MonthFile.print(",");                 // [02] Voltage
                        MonthFile.print(Month_Data_Record.field.Amperage); MonthFile.print(",");                // [03] Amperage
                        MonthFile.print(Month_Data_Record.field.wattage); MonthFile.print(",");                 // [04] Wattage
                        MonthFile.print(Month_Data_Record.field.uptime); MonthFile.print(",");                  // [05] UpTime
                        MonthFile.print(Month_Data_Record.field.kilowatthour); MonthFile.print(",");            // [06] Kiliowatthour
                        MonthFile.print(Month_Data_Record.field.powerfactor); MonthFile.print(",");             // [07] Power Factor
                        MonthFile.print(Month_Data_Record.field.frequency); MonthFile.print(",");               // [08] Frequency
                        MonthFile.print(Month_Data_Record.field.sensor_temperature); MonthFile.print(",");      // [09] Sensor Temperature
                        MonthFile.print(Month_Data_Record.field.weather_temperature); MonthFile.print(",");     // [10] Weaather Temperature
                        MonthFile.print(Month_Data_Record.field.temperature_feels_like); MonthFile.print(",");  // [11] Temperature Feels Like
                        MonthFile.print(Month_Data_Record.field.temperature_maximum); MonthFile.print(",");     // [12] Temperature Maximum
                        MonthFile.print(Month_Data_Record.field.temperature_minimum); MonthFile.print(",");     // [13] Temperature Minimum
                        MonthFile.print(Month_Data_Record.field.atmospheric_pressure); MonthFile.print(",");    // [14] Atmospheric Pressure
                        MonthFile.print(Month_Data_Record.field.relative_humidity); MonthFile.print(",");       // [15] Relative Humidity
                        MonthFile.print(Month_Data_Record.field.wind_speed); MonthFile.print(",");              // [16] Wind Speed
                        MonthFile.print(Month_Data_Record.field.wind_direction); MonthFile.print(",");          // [17] Wind Direction
                        MonthFile.print(Month_Data_Record.field.gas_volume); MonthFile.print(",");              // [18] Gas Volume
                        MonthFile.print(Month_Data_Record.field.weather_description);                           // [19] Weather Description
                        MonthFile.println();                                                                    // end of record
                    } // end of end of line detected
                } // end of while
            }
            DataFile.close();
            DataFile.flush();
        }
    }
    MonthFile.close();
    MonthFile.flush();
}
void Write_New_Data_Record_to_Data_File() {
    digitalWrite(Blue_led_pin, HIGH);                          // turn the SD activity LED on
    DataFile = SD.open("/" + DataFileName, FILE_APPEND);            // open the SD file
    //    console.print(millis(), DEC); console.println("\tOpening Datafile: " + String(DataFileName));
    if (!DataFile) {                                                // oops - file not available!
        console.print(millis(), DEC); console.println("\tError re-opening DataFile:" + String(DataFileName));
        while (true) {
            digitalWrite(Green_led_pin, !digitalRead(Green_led_pin));
            digitalWrite(Blue_led_pin, !digitalRead(Blue_led_pin));
            delay(500);
            Check_Red_Switch();                               // Reset will restart the processor so no return
        }
    }
    SD_freespace = (SD.totalBytes() - SD.usedBytes());
    console.print(millis(), DEC); console.println("\tNew Data Record Written to " + DataFileName);
    DataFile.print(Current_Data_Record.field.ldate); DataFile.print(",");                   // [00] Date
    DataFile.print(Current_Data_Record.field.ltime); DataFile.print(",");                   // [01] Time
    DataFile.print(Current_Data_Record.field.Voltage); DataFile.print(",");                 // [02] Voltage
    DataFile.print(Current_Data_Record.field.Amperage); DataFile.print(",");                // [03] Amperage
    DataFile.print(Current_Data_Record.field.wattage); DataFile.print(",");                 // [04] Wattage
    DataFile.print(Current_Data_Record.field.uptime); DataFile.print(",");                  // [05] UpTime
    DataFile.print(Current_Data_Record.field.kilowatthour); DataFile.print(",");            // [06] Kiliowatthour
    DataFile.print(Current_Data_Record.field.powerfactor); DataFile.print(",");             // [07] Power Factor
    DataFile.print(Current_Data_Record.field.frequency); DataFile.print(",");               // [08] Frequency
    DataFile.print(Current_Data_Record.field.sensor_temperature); DataFile.print(",");      // [09] Sensor Temperature
    DataFile.print(Current_Data_Record.field.weather_temperature); DataFile.print(",");     // [10] Weaather Temperature
    DataFile.print(Current_Data_Record.field.temperature_feels_like); DataFile.print(",");  // [11] Temperature Feels Like
    DataFile.print(Current_Data_Record.field.temperature_maximum); DataFile.print(",");     // [12] Temperature Maximum
    DataFile.print(Current_Data_Record.field.temperature_minimum); DataFile.print(",");     // [13] Temperature Minimum
    DataFile.print(Current_Data_Record.field.atmospheric_pressure); DataFile.print(",");    // [14] Atmospheric Pressure
    DataFile.print(Current_Data_Record.field.relative_humidity); DataFile.print(",");       // [15] Relative Humidity
    DataFile.print(Current_Data_Record.field.wind_speed); DataFile.print(",");              // [16] Wind Speed
    DataFile.print(Current_Data_Record.field.wind_direction); DataFile.print(",");          // [17] Wind Direction
    DataFile.print(Current_Data_Record.field.gas_volume); DataFile.print(",");              // [18] Gas Volume
    DataFile.print(Current_Data_Record.field.weather_description);                          // [19] Weather Description
    DataFile.println();                                                                     // end of record
    DataFile.close();                                                                       // close the sd file
    DataFile.flush();                                                                       // make sure it has been written to SD
    Data_Record_Count++;                                                             // increment the current record count
    digitalWrite(Blue_led_pin, LOW);           // turn the SD activity LED on
    Update_Webpage_Variables_from_Current_Data_Record();
}
void Add_New_Data_Record_to_Display_Table() {
    if (Data_Table_Pointer == Data_Table_Size) {                                                   // table full, shuffle fifo
        Shuffle_Data_Table();
        Data_Table_Pointer = Data_Table_Size - 1;                                                  // subsequent records will be added at the end of the table
    }
    strncpy(Readings_Table[Data_Table_Pointer].field.ldate, Current_Data_Record.field.ldate, sizeof(Readings_Table[Data_Table_Pointer].field.ldate));                       // [0]  date
    strncpy(Readings_Table[Data_Table_Pointer].field.ltime, Current_Data_Record.field.ltime, sizeof(Readings_Table[Data_Table_Pointer].field.ltime));                       // [1]  time
    Readings_Table[Data_Table_Pointer].field.Voltage = Current_Data_Record.field.Voltage;                                // [2]  Voltage
    Readings_Table[Data_Table_Pointer].field.Amperage = Current_Data_Record.field.Amperage;                              // [3]  Amperage
    Readings_Table[Data_Table_Pointer].field.wattage = Current_Data_Record.field.wattage;                                // [4]  wattage
    Readings_Table[Data_Table_Pointer].field.uptime = Current_Data_Record.field.uptime;                                  // [5]  uptime
    Readings_Table[Data_Table_Pointer].field.kilowatthour = Current_Data_Record.field.kilowatthour;                      // [6]  kilowatthours
    Readings_Table[Data_Table_Pointer].field.powerfactor = Current_Data_Record.field.powerfactor;                        // [7]  power factor
    Readings_Table[Data_Table_Pointer].field.frequency = Current_Data_Record.field.frequency;                            // [8]  frequency
    Readings_Table[Data_Table_Pointer].field.sensor_temperature = Current_Data_Record.field.sensor_temperature;          // [9] sensor temperature
    Readings_Table[Data_Table_Pointer].field.weather_temperature = Current_Data_Record.field.weather_temperature;        // [10] weather temperature
    Readings_Table[Data_Table_Pointer].field.temperature_feels_like = Current_Data_Record.field.temperature_feels_like;  // [11] temperature feels like
    Readings_Table[Data_Table_Pointer].field.temperature_maximum = Current_Data_Record.field.temperature_maximum;        // [12] temperature maximum
    Readings_Table[Data_Table_Pointer].field.temperature_minimum = Current_Data_Record.field.temperature_minimum;        // [13] temperature minimum
    Readings_Table[Data_Table_Pointer].field.atmospheric_pressure = Current_Data_Record.field.atmospheric_pressure;      // [14] atmospheric pressure
    Readings_Table[Data_Table_Pointer].field.relative_humidity = Current_Data_Record.field.relative_humidity;            // [15] relative humidity
    Readings_Table[Data_Table_Pointer].field.wind_speed = Current_Data_Record.field.wind_speed;                          // [16] wind speed
    Readings_Table[Data_Table_Pointer].field.weather_temperature = Current_Data_Record.field.weather_temperature;        // [17] wind direction
    strncpy(Readings_Table[Data_Table_Pointer].field.weather_description, Current_Data_Record.field.weather_description, sizeof(Readings_Table[Data_Table_Pointer].field.weather_description));        // [18] weather description
    Readings_Table[Data_Table_Pointer].field.gas_volume = Current_Data_Record.field.gas_volume;                          // [19] gas volume
    Data_Table_Pointer++;
}
void Create_New_Data_File() {
    Update_Current_TimeInfo();
    DataFileName = String(Current_Date_Without) + ".csv";
    digitalWrite(Blue_led_pin, HIGH);
    if (!SD.exists("/" + DataFileName)) {
        DataFile = SD.open("/" + DataFileName, FILE_WRITE);
        if (!DataFile) {                                            // log file not opened
            console.print(millis(), DEC); console.print("Error opening Data file in Create New Data File [" + String(DataFileName) + "]");
            while (true) {
                digitalWrite(Green_led_pin, !digitalRead(Green_led_pin));
                digitalWrite(Blue_led_pin, !digitalRead(Blue_led_pin));
                delay(500);
                Check_Red_Switch();
            }
        }
        console.print(millis(), DEC); console.println("\tDay Data File " + DataFileName + " Opened");
        for (int x = 0; x < Number_of_Column_Field_Names; x++) {           // write data column headings into the SD file
            DataFile.print(Column_Field_Names[x]);
            DataFile.print(",");
        }
        DataFile.println(Column_Field_Names[Number_of_Column_Field_Names]);
        console.print(millis(), DEC); console.println("\tColumn Headings written to Day Data File");
        DataFile.close();
        DataFile.flush();
        console.print(millis(), DEC); console.println("\tDay Data File Closed");
        Data_Record_Count = 0;
        digitalWrite(Blue_led_pin, LOW);
        Update_Current_TimeInfo();
        This_Date_With = Current_Date_With;                                 // update the current date
        This_Date_Without = Current_Date_Without;
        This_Day = Current_Date_Time_Data.field.Day;
    }
    else {
        console.print(millis(), DEC); console.println("\tData File " + String(DataFileName) + " already exists");
        Prefill_Data_Array();
    }
}
void Check_WiFi() {
    int wifi_connection_attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {                     // whilst it is not connected keep trying
        console.print(millis(), DEC); console.print("WiFi Connection Failed, Attempting to Reconnect");
        delay(3000);                                           // wait 3 seconds before retrying
        console.print(millis(), DEC); console.print("Connection attempt " + String(wifi_connection_attempts));
        if (wifi_connection_attempts++ > 20) {
            console.print(millis(), DEC); console.print("Network Error, WiFi lost >20 seconds, Restarting Processor");
            ESP.restart();
        }
        StartWiFi(ssid, password);
    }
}
int StartWiFi(const char* ssid, const char* password) {
    console.print(millis(), DEC); console.println("\tWiFi Connecting to " + String(ssid));
    if (WiFi.status() == WL_CONNECTED) {                              // disconnect to start new wifi connection
        console.print(millis(), DEC); console.println("\tWiFi Already Connected");
        console.print(millis(), DEC); console.println("\tDisconnecting");
        WiFi.disconnect(true);
        console.print(millis(), DEC); console.println("\tDisconnected");
    }
    WiFi.begin(ssid, password);                                         // connect to the wifi network
    int WiFi_Status = WiFi.status();
    console.print(millis(), DEC); console.println("\tWiFi Status: " + String(WiFi_Status) + " " + WiFi_Status_Message[WiFi_Status]);
    int wifi_connection_attempts = 0;                           // zero the attempt counter
    while (WiFi.status() != WL_CONNECTED) {                     // whilst it is not connected keep trying
        delay(3000);
        console.print(millis(), DEC); console.println("\tConnection attempt " + String(wifi_connection_attempts));
        if (wifi_connection_attempts++ > 20) {
            console.print(millis(), DEC); console.println("\tNetwork Error, Not able to open WiFi, Restarting Processor");
            ESP.restart();
        }
    }
    WiFi_Status = WiFi.status();
    console.print(millis(), DEC); console.println("\tWiFi Status: " + String(WiFi_Status) + " " + WiFi_Status_Message[WiFi_Status]);
    WiFi_Signal_Strength = (int)WiFi.RSSI();
    console.print(millis(), DEC); console.println("\tWiFi Signal Strength:" + String(WiFi_Signal_Strength));
    console.print(millis(), DEC); console.println("\tWiFi IP Address: " + String(WiFi.localIP().toString().c_str()));
    return true;
}
void InitTime(String timezone) {
    console.print(millis(), DEC); console.println("\tStarting Time Server");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    setenv("TZ", timezone.c_str(), 1);                                            // adjust time settings to London
    tzset();
    console.print(millis(), DEC); console.println("\tTime Server Started");
}
void Prefill_Data_Array() {
    int character_count = 0;
    char Field[50];
    int datafieldNo = 0;
    char datatemp;
    int prefill_Data_Table_Pointer = 0;
    String message;
    Data_Table_Pointer = 0;
    SD_Led_Flash_Start_Stop(true);                                              // start the sd led flashing
    console.print(millis(), DEC); console.println("\tLoading DataFile from " + String(DataFileName));
    File DataFile = SD.open("/" + DataFileName, FILE_READ);
    if (DataFile) {
        console.print(millis(), DEC); console.println("\tPrefill Array - Skipping Column Heading Row");
        while (DataFile.available()) {                                           // throw the first row, column headers, away
            datatemp = DataFile.read();
            if (datatemp == '\n') break;
        }
        console.print(millis(), DEC); console.println("\tPrefill Array - Reading Data Records");
        while (DataFile.available()) {                                           // do while there are data available
            Flash_SD_LED();                                                     // flash the sd led
            datatemp = DataFile.read();
            Field[character_count++] = datatemp;                            // add it to the csvfield string
            if (datatemp == ',' || datatemp == '\n') {                                  // look for end of field
                Field[character_count - 1] = '\0';                           // insert termination character where the ',' or '\n' was
                switch (datafieldNo) {
                case 0: {
                    strncpy(Readings_Table[prefill_Data_Table_Pointer].field.ldate, Field, sizeof(Readings_Table[prefill_Data_Table_Pointer].field.ldate));       // Date
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Date: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.ldate);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 1: {
                    strncpy(Readings_Table[prefill_Data_Table_Pointer].field.ltime, Field, sizeof(Readings_Table[prefill_Data_Table_Pointer].field.ltime));
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Time: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.ltime);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 2: {
                    Readings_Table[prefill_Data_Table_Pointer].field.Voltage = atof(Field);         // [02] Voltage
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Voltage: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.Voltage);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 3: {
                    Readings_Table[prefill_Data_Table_Pointer].field.Amperage = atof(Field);        // [03] Amperage
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Amperage: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.Amperage);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 4: {
                    Readings_Table[prefill_Data_Table_Pointer].field.wattage = atof(Field);         // [04] Wattage
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Wattage: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.wattage);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 5: {
                    Readings_Table[prefill_Data_Table_Pointer].field.uptime = atof(Field);          // [05] Uptime
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Uptime: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.uptime);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif
                    break;
                }
                case 6: {
                    Readings_Table[prefill_Data_Table_Pointer].field.kilowatthour = atof(Field);    // [06] Kilowatthour
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Kilowatthour: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.kilowatthour);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 7: {
                    Readings_Table[prefill_Data_Table_Pointer].field.powerfactor = atof(Field);     // [07] Power Factor
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Power Factor: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.powerfactor);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 8: {
                    Readings_Table[prefill_Data_Table_Pointer].field.frequency = atof(Field);       // [09] Frequency
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Frequency: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.frequency);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif
                    break;
                }
                case 9: {
                    Readings_Table[prefill_Data_Table_Pointer].field.sensor_temperature = atof(Field);  // [10] Sensor Temperature
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Sensor Temperature: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.sensor_temperature);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 10: {
                    Readings_Table[prefill_Data_Table_Pointer].field.weather_temperature = atof(Field); // [15] Weather Temperature
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Weather Temperature: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.weather_temperature);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 11: {
                    Readings_Table[prefill_Data_Table_Pointer].field.temperature_feels_like = atof(Field);  // [16] Temperatre Feels Like
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Temperature Feels Like: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.temperature_feels_like);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 12: {
                    Readings_Table[prefill_Data_Table_Pointer].field.temperature_maximum = atof(Field);     // [17] Temperature Maximum
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Maximum Temperatre: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.temperature_maximum);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 13: {
                    Readings_Table[prefill_Data_Table_Pointer].field.temperature_minimum = atof(Field);     // [18] Temperature Minimum
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Temperature Minimum: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.temperature_minimum);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 14: {
                    Readings_Table[prefill_Data_Table_Pointer].field.atmospheric_pressure = atof(Field);    // [19] Atmospheric Pressure
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Atmospheric Pressure: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.atmospheric_pressure);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 15: {
                    Readings_Table[prefill_Data_Table_Pointer].field.relative_humidity = atof(Field);       // [20] Relative Humidity
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Relative Humidity: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.relative_humidity);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 16: {
                    Readings_Table[prefill_Data_Table_Pointer].field.wind_speed = atof(Field);              // [21] Wind Speed
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Wind Speed: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.wind_speed);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 17: {
                    Readings_Table[prefill_Data_Table_Pointer].field.wind_direction = atof(Field);          // [22] Wind Direction
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Wind Direction: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.wind_direction);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif                    
                    break;
                }
                case 18: {
                    Readings_Table[prefill_Data_Table_Pointer].field.gas_volume = atof(Field);
                    Current_Data_Record.field.gas_volume = Readings_Table[prefill_Data_Table_Pointer].field.gas_volume;     // update the gas volume
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Gas Volume: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.gas_volume);
                    console.print(millis(), DEC); console.println("\t" + message);
#endif              
                    Latest_Gas_Date_With = Readings_Table[prefill_Data_Table_Pointer].field.ldate;
                    Latest_Gas_Time_With = Readings_Table[prefill_Data_Table_Pointer].field.ltime;
                    Latest_Gas_Volume = Current_Data_Record.field.gas_volume;
                    break;
                }
                case 19: {
                    strncpy(Readings_Table[prefill_Data_Table_Pointer].field.weather_description, Field, sizeof(Readings_Table[prefill_Data_Table_Pointer].field.weather_description));
#ifdef PRINT_PREFILL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(prefill_Data_Table_Pointer);
                    message += "] Weather Description: ";
                    message += String(Readings_Table[prefill_Data_Table_Pointer].field.weather_description);
                    console.print(millis(), DEC); console.println("\t" + message);
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
            if (datatemp == '\n') {                     // at this point the obtained record has been saved in the table
                Update_Webpage_Variables_from_Table(prefill_Data_Table_Pointer);    // update the web variables with the record just saved
                if (prefill_Data_Table_Pointer == 0) {
                    console.print(millis(), DEC); console.print("\t");
                    Data_Record_Start_Time = micros();         // initialise start time for read timing
                    First_Data_Record_Start_Time = Data_Record_Start_Time;
                }
                prefill_Data_Table_Pointer++;                                       // increment the table array pointer
                Data_Record_Count++;                                         // increment the running record toral
                console.print(".");
                if ((Data_Record_Count % 400) == 0) {
                    console.print(" (");
                    console.print(Data_Record_Count);
                    console.print(") [");
                    console.print(((Data_Record_Start_Time - millis()) / Data_Record_Count) / 1000);
                    console.println("s]");
                    console.print(millis(), DEC);                                   // start another line
                    console.print("\t");
                }
                if (prefill_Data_Table_Pointer == Data_Table_Size) {                 // if pointer is greater than table size
                    Shuffle_Data_Table();
                    prefill_Data_Table_Pointer = Data_Table_Size - 1;
                }
                datafieldNo = 0;
            } // end of end of line detected
        } // end of while
    }
    DataFile.close();
    console.print(" ("); console.print(Data_Record_Count);
    console.print(") [");
    if (Data_Record_Count > 0) {
        console.print(((millis() - First_Data_Record_Start_Time) / Data_Record_Count) / 1000);
    }
    else {
        console.print(millis() - First_Data_Record_Start_Time);
    }
    console.println("s]");
    console.print(millis(), DEC); console.println("\tLoaded Data Records: " + String(Data_Record_Count));
    SD_Led_Flash_Start_Stop(false);
}
void Shuffle_Data_Table() {
#ifdef PRINT_SHUFFLING_DATA_VALUES
    console.print(millis(), DEC); console.println("\tShuffling Data Table");
#endif
    for (int i = 0; i < (Data_Table_Size - 1); i++) {           // shuffle the rows up, losing row 0, make row [table_size] free
#ifdef PRINT_SHUFFLING_DATA_VALUES
        console.print(millis(), DEC); console.print("\tShuffling Data Record "); console.print(i + 1); console.print(" to "); console.println((i));
#endif
        strncpy(Readings_Table[i].field.ldate, Readings_Table[i + 1].field.ldate, sizeof(Readings_Table[i].field.ldate));                           // [0]  date
        strncpy(Readings_Table[i].field.ltime, Readings_Table[i + 1].field.ltime, sizeof(Readings_Table[i].field.ltime));                           // [1]  time
        Readings_Table[i].field.Voltage = Readings_Table[i + 1].field.Voltage;                              // [2]  Voltage
        Readings_Table[i].field.Amperage = Readings_Table[i + 1].field.Amperage;                            // [3]  Amperage
        Readings_Table[i].field.wattage = Readings_Table[i + 1].field.wattage;                              // [4]  wattage
        Readings_Table[i].field.uptime = Readings_Table[i + 1].field.uptime;                                // [5]  uptime
        Readings_Table[i].field.kilowatthour = Readings_Table[i + 1].field.kilowatthour;                    // [6]  kilowatthours
        Readings_Table[i].field.powerfactor = Readings_Table[i + 1].field.powerfactor;                      // [7]  power factor
        Readings_Table[i].field.frequency = Readings_Table[i + 1].field.frequency;                          // [8]  frequency
        Readings_Table[i].field.sensor_temperature = Readings_Table[i + 1].field.sensor_temperature;        // [9] sensor temperature
        Readings_Table[i].field.weather_temperature = Readings_Table[i + 1].field.weather_temperature;      // [10] weather temperature
        Readings_Table[i].field.temperature_feels_like = Readings_Table[i + 1].field.temperature_feels_like;// [11] temperature feels like
        Readings_Table[i].field.temperature_maximum = Readings_Table[i + 1].field.temperature_maximum;      // [12] temperature maximum
        Readings_Table[i].field.temperature_minimum = Readings_Table[i + 1].field.temperature_minimum;      // [13] temperature minimum
        Readings_Table[i].field.atmospheric_pressure = Readings_Table[i + 1].field.atmospheric_pressure;    // [14] atmospheric pressure
        Readings_Table[i].field.relative_humidity = Readings_Table[i + 1].field.relative_humidity;          // [15] relative humidity
        Readings_Table[i].field.wind_speed = Readings_Table[i + 1].field.wind_speed;                        // [16] wind speed
        Readings_Table[i].field.wind_direction = Readings_Table[i + 1].field.wind_direction;                // [17] wind direction
        Readings_Table[i].field.gas_volume = Readings_Table[i + 1].field.gas_volume;                        // [18] gas volume
        strncpy(Readings_Table[i].field.weather_description, Readings_Table[i + 1].field.weather_description, sizeof(Readings_Table[i].field.weather_description));// [19] weather description
    }
}
void Update_Webpage_Variables_from_Current_Data_Record() {
    if (Current_Data_Record.field.Voltage >= Highest_Voltage) {
        Date_of_Highest_Voltage = Current_Data_Record.field.ldate;
        Time_of_Highest_Voltage = Current_Data_Record.field.ltime;
        Highest_Voltage = Current_Data_Record.field.Voltage;                       // update the largest current value
    }
    else {
        if (Date_of_Highest_Voltage == "") {
            Date_of_Highest_Voltage = Current_Data_Record.field.ldate;
            Time_of_Highest_Voltage = Current_Data_Record.field.ltime;
        }
    }
    if (Lowest_Voltage >= Current_Data_Record.field.Voltage) {
        Date_of_Lowest_Voltage = Current_Data_Record.field.ldate;
        Time_of_Lowest_Voltage = Current_Data_Record.field.ltime;
        Lowest_Voltage = Current_Data_Record.field.Voltage;                        // update the largest current value
    }
    else {
        if (Date_of_Lowest_Voltage == "") {
            Date_of_Lowest_Voltage = Current_Data_Record.field.ldate;
            Time_of_Lowest_Voltage = Current_Data_Record.field.ltime;
        }
    }
    if (Current_Data_Record.field.Amperage >= Highest_Amperage) {
        Date_of_Highest_Amperage = Current_Data_Record.field.ldate;
        Time_of_Highest_Amperage = Current_Data_Record.field.ltime;
        Highest_Amperage = Current_Data_Record.field.Amperage;                      // update the largest current value
    }
    else {
        if (Date_of_Highest_Amperage == "") {
            Date_of_Highest_Amperage = Current_Data_Record.field.ldate;
            Time_of_Highest_Amperage = Current_Data_Record.field.ltime;
        }
    }
    Cumulative_kwh += Current_Data_Record.field.kilowatthour;
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
    Latest_Gas_Volume = Current_Data_Record.field.gas_volume;
    Latest_Gas_Date_With = Current_Data_Record.field.ldate;
    Latest_Gas_Time_With = Current_Data_Record.field.ltime;
    SD_freespace_double = (double)SD_freespace / 1000000;
    if (SD_freespace < critical_SD_freespace) {
        console.print(millis(), DEC); console.println("\tWARNING - SD Free Space critical " + String(SD_freespace) + "MBytes");
    }
}
void Update_Webpage_Variables_from_Table(int Data_Table_Pointer) {
    if (Readings_Table[Data_Table_Pointer].field.Voltage >= Highest_Voltage) {
        Date_of_Highest_Voltage = String(Readings_Table[Data_Table_Pointer].field.ldate);
        Time_of_Highest_Voltage = String(Readings_Table[Data_Table_Pointer].field.ltime);
        Highest_Voltage = Readings_Table[Data_Table_Pointer].field.Voltage;
    }
    if ((Readings_Table[Data_Table_Pointer].field.Voltage <= Lowest_Voltage) || !Lowest_Voltage) {
        Date_of_Lowest_Voltage = String(Readings_Table[Data_Table_Pointer].field.ldate);
        Time_of_Lowest_Voltage = String(Readings_Table[Data_Table_Pointer].field.ltime);
        Lowest_Voltage = Readings_Table[Data_Table_Pointer].field.Voltage;                     // update the largest current value
    }
    if (Readings_Table[Data_Table_Pointer].field.Amperage >= Highest_Amperage) {               // load the maximum Amperage value
        Date_of_Highest_Amperage = String(Readings_Table[Data_Table_Pointer].field.ldate);
        Time_of_Highest_Amperage = String(Readings_Table[Data_Table_Pointer].field.ltime);
        Highest_Amperage = Readings_Table[Data_Table_Pointer].field.Amperage;                  // update the largest current value
    }
    Cumulative_kwh += Readings_Table[Data_Table_Pointer].field.kilowatthour;
    Latest_weather_temperature_feels_like = Readings_Table[Data_Table_Pointer].field.temperature_feels_like;
    Latest_weather_temperature_maximum = Readings_Table[Data_Table_Pointer].field.temperature_maximum;
    Latest_weather_temperature_minimum = Readings_Table[Data_Table_Pointer].field.temperature_minimum;
    Latest_relative_humidity = Readings_Table[Data_Table_Pointer].field.relative_humidity;
    Latest_atmospheric_pressure = Readings_Table[Data_Table_Pointer].field.atmospheric_pressure;
    Latest_wind_speed = Readings_Table[Data_Table_Pointer].field.wind_speed;
    Latest_wind_direction = Readings_Table[Data_Table_Pointer].field.wind_direction;
    Latest_weather_description = Readings_Table[Data_Table_Pointer].field.weather_description;
    Latest_Gas_Volume = Readings_Table[Data_Table_Pointer].field.gas_volume;
    Latest_Gas_Date_With = Readings_Table[Data_Table_Pointer].field.ldate;
    Latest_Gas_Time_With = Readings_Table[Data_Table_Pointer].field.ltime;
}
void Display() {
    double maximum_Amperage = 0;
    console.print(millis(), DEC); console.println("\tWeb Display of Graph Requested via Webpage");
    webpage = "";
    Page_Header(true, "Energy Usage Monitor " + String(Current_Date_With) + " " + String(Current_Time_With));
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
        if (String(Readings_Table[i].field.ltime) != "") {                  // if the ltime field contains data
            for (int y = 0; y < 8; y++) {                                   // replace the ":"s in ltime with ","
                if (Readings_Table[i].field.ltime[y] == ':') {
                    Readings_Table[i].field.ltime[y] = ',';
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
    webpage += F("title:'Electrical and Gas Power Consumption logarithmic scale',");
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
    webpage += F("series:{0:{targetAxisIndex:0},1:{targetAxisIndex:1},curveType:'none'},};");
    webpage += F("var chart = new google.visualization.LineChart(document.getElementById('line_chart'));chart.draw(data, options);");
    webpage += F("}");
    webpage += F("</script>");
    webpage += F("<div id='line_chart' style='width:960px; height:600px'></div>");
    Page_Footer();
    lastcall = "display";
}
void Web_Reset() {
    console.print(millis(), DEC); console.println("\tWeb Reset of Processor Requested via Webpage");
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
    Update_Current_TimeInfo();
    // <ul> start -----------------------------------------------------------------------------------------------------
    webpage += F("<ul>");
    webpage += F("<li><a href='/Display'>Webpage</a> </li>");
    webpage += F("<li><a href='/Information'>Display Information</a></li>");
    webpage += F("<li><a href='/DownloadFiles'>Download Files</a></li>");
    webpage += F("<li><a href='/DeleteFiles'>Delete Files</a></li>");
    webpage += F("<li><a href='/Reset'>Reset Processor</a></li>");
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
    webpage += String(Current_Time_With);
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
    console.print(millis(), DEC); console.println("\tWeb Display of Information Requested via Webpage");
    String ht = String(Date_of_Highest_Voltage + " at " + Time_of_Highest_Voltage + " as " + String(Highest_Voltage) + " volts");
    String lt = String(Date_of_Lowest_Voltage + " at " + Time_of_Lowest_Voltage + " as " + String(Lowest_Voltage) + " volts");
    String ha = String(Date_of_Highest_Amperage + " at " + Time_of_Highest_Amperage + " as " + String(Highest_Amperage) + " amps");
    Page_Header(true, "Energy Usage Monitor " + String(Current_Date_With) + " " + String(Current_Time_With));
    File DataFile = SD.open("/" + DataFileName, FILE_READ);  // Now read data from FS
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
    webpage += String(DataFile.size());
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
    webpage += Last_Boot_Date_With + " at " + Last_Boot_Time_With;
    webpage += "</span></strong></p>";
    // Data Record Count ----------------------------------------------------------------------------------------------
    double Percentage = (Data_Record_Count * (double)100) / (double)17280;
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Number of Data Readings: ");
    webpage += String(Data_Record_Count);
    webpage += F(" Percentage of Full Day: ");
    webpage += String(Percentage, 0);
    webpage += F("%");
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
    // Cumulative KiloWattHours ---------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Daily KiloWatt Hours: ");
    webpage += String(Cumulative_kwh, 3);
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Latest Weather Temperature Feels Like, Maximum & Minimum -----------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
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
    // Weather Latest Weather Relative Humidity -----------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Relative Humidity: ");
    webpage += String(Latest_relative_humidity) + "%";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Latest Atmospheric Pressure ----------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Atmospheric Pressure: ");
    webpage += String(Latest_atmospheric_pressure) + " millibars";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Latest Wind Speed and Direction ------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Wind Speed: ");
    webpage += String(Latest_wind_speed) + "m/s,";
    webpage += F("Direction: ");
    webpage += String(Latest_wind_direction);
    webpage += F("&deg;");
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Weather Latest Weather Description -----------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F("'>Weather: ");
    webpage += String(Latest_weather_description);
    webpage += "</span></strong></p>";
    // Last Gas Sensor Interrupt
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Date and Time of Last Gas Switch Signal: ");
    webpage += String(Latest_Gas_Date_With) + " at " + Latest_Gas_Time_With;
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Latest Gas Volume ---------------------------------------------------------------------------------------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:12px;'");
    webpage += F(">Cumulative Gas Volume: ");
    webpage += String(Current_Data_Record.field.gas_volume) + " cubic metres";
    webpage += "</span></strong></p>";
    webpage += F("<p </h3>");
    // ----------------------------------------------------------------------------------------------------------------
    DataFile.close();
    Page_Footer();
}
void Download_Files() {
    int file_count = Count_Files_on_SD_Drive();                                 // this counts and creates an array of file names on SD
    console.print(millis(), DEC); console.println("\tDownload of Files Requested via Webpage");
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
    console.print(millis(), DEC); console.println("\tDownload of File " + fileName + " Requested via Webpage");
    File DataFile = SD.open("/" + fileName, FILE_READ);    // Now read data from FS
    if (DataFile) {                                             // if there is a file
        if (DataFile.available()) {                             // If data is available and present
            String contentType = "application/octet-stream";
            server.sendHeader("Content-Disposition", "attachment; filename=" + fileName);
            if (server.streamFile(DataFile, contentType) != DataFile.size()) {
                console.print(millis(), DEC); console.print("Sent less data (" + String(server.streamFile(DataFile, contentType)) + ") from " + fileName + " than expected (" + String(DataFile.size()) + ")");
            }
        }
    }
    DataFile.close(); // close the file:
    webpage = "";
}
void Delete_Files() {                                                           // allow the cliet to select a file for deleti
    int file_count = Count_Files_on_SD_Drive();                                 // this counts and creates an array of file names on SD
    console.print(millis(), DEC); console.println("\tDelete Files Requested via Webpage");
    Page_Header(false, "Energy Monitor Delete Files");
    if (file_count > 3) {
        for (i = 1; i < file_count; i++) {
            if (FileNames[i] != DataFileName) {   // do not list the current file
                webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(i) + " " + String(FileNames[i]) + " ";
                webpage += "&nbsp;<a href=\"/DelFile?file=" + String(FileNames[i]) + " " + "\">Delete</a>";
                webpage += "</h3>";
            }
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
void Del_File() {                                                       // web request to delete a file
    String fileName = "\20221111.csv";                                  // dummy load to get the string space reserved
    fileName = "/" + server.arg("file");
    if (fileName != ("/" + DataFileName)) {                            // do not delete the current file
        SD.remove(fileName);
        console.print(millis(), DEC); console.println("\t" + String(DataFileName) + " Removed");
    }
    int file_count = Count_Files_on_SD_Drive();                         // this counts and creates an array of file names on SD
    webpage = "";                                                       // don't delete this command, it ensures the server works reliably!
    Page_Header(false, "Energy Monitor Delete Files");
    if (file_count > 3) {
        for (i = 1; i < file_count; i++) {
            if (FileNames[i] != DataFileName) {   // do not list the current file
                webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(i) + " " + String(FileNames[i]) + " ";
                webpage += "&nbsp;<a href=\"/DelFile?file=" + String(FileNames[i]) + " " + "\">Delete</a>";
                webpage += "</h3>";
            }
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
    File root = SD.open("/");                                       //  Open the root directory
    do {
        File entry = root.openNextFile();                           //  get the next file
        if (entry) {
            filename = entry.name();
            files_present = true;
            File DataFile = SD.open("/" + filename, FILE_READ);     // Now read data from FS
            if (DataFile) {                                         // if there is a file
                FileNames[file_count] = filename;
                //                console_message = "File " + String(file_count) + " filename " + String(filename);
                //                console.print(millis(), DEC); console.print(console_message);
                file_count++;                                       // increment the file count
            }
            DataFile.close(); // close the file:
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
    console.print(millis(), DEC); console.print("Start of Wipe Files Request by Switch");
    String filename;
    File root = SD.open("/");                                       //  Open the root directory
    while (true) {
        File entry = root.openNextFile();                           //  get the next file
        if (entry) {
            filename = entry.name();
            console.print(millis(), DEC); console.print("Removing " + filename);
            SD.remove(entry.name());                                //  delete the file
        }
        else {
            root.close();
            console.print(millis(), DEC); console.print("All files removed from root directory, rebooting");
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
                console.print(millis(), DEC); console.println("\tNo Reply from RS485 within 500 ms");
                while (true) {                                                          // wait for reset to be pressed
                    digitalWrite(Green_led_pin, !digitalRead(Green_led_pin));
                    digitalWrite(Blue_led_pin, !digitalRead(Blue_led_pin));
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
            double Voltage = (value[3] << 8) + value[4];                                                    // Received is number of tenths of volt
            //            console.print("\tReceived Voltage (Raw): "); console.print(Voltage, 4);
            Voltage = Voltage / (double)10;                                                                 // convert to volts
            //            console.print("\t/10\t"); console.println(Voltage, 4); ;
            Current_Data_Record.field.Voltage = Voltage;                                                    // Voltage output format double
            break;
        }
        case Request_Amperage: {                                                                    // [1]  Amperage
            double Amperage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);           // Received is number of milli amps 
            //            console.print("\tReceived Amperage (Raw): "); console.print(Amperage, 4);
            Amperage /= (double)1000;                                                               // convert milliamps to amps
            //           console.print("\t/1000\t"); console.println(Amperage, 4);
            Current_Data_Record.field.Amperage = Amperage;                                                  // Amperage output format double nn.n
            break;
        }
        case Request_Wattage: {                                                                     // [2]  Wattage
            double wattage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);            // Recieved is number of tenths of watts
            //            console.print("\tReceived Wattage (Raw): "); console.print(wattage, 4);
            //            console.print("\t\t"); console.println(wattage, 4);
            Current_Data_Record.field.wattage = wattage;
            break;
        }
        case Request_UpTime: {                                                                      //  [3] Uptime
            double uptime = (double)(value[3] << 8) + (double)value[4];
            //            console.print("\tReceived Uptime (Raw): "); console.print(uptime, 4);
            uptime = uptime / (double)60;                                                                   // convert to hours
            //            console.print("\t/60\t"); console.println(uptime, 4);
            Current_Data_Record.field.uptime = uptime;
            break;
        }
        case Request_Kilowatthour: {                                                                //  [4] KilowattHour
            double kilowatthour = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);
            //            console.print("\tReceived Kilowatthour (Raw): "); console.print(kilowatthour, 4);
            kilowatthour = kilowatthour / (double)1000;
            //            console.print("\t/1000\t"); console.println(kilowatthour, 4);
            Current_Data_Record.field.kilowatthour = kilowatthour;
            break;
        }
        case Request_Power_Factor: {                                                                //  [5] Power Factor
            double powerfactor = (double)(value[3] << 8) + (double)value[4];
            //            console.print("\tReceived Power Factor (Raw): "); console.print(powerfactor, 4);
            powerfactor = powerfactor / (double)100;
            //            console.print("\t/100\t"); console.println(powerfactor, 4);
            Current_Data_Record.field.powerfactor = powerfactor;
            break;
        }
        case Request_Frequency: {                                                                   //  [7] Frequency
            double frequency = (double)(value[3] * (double)256) + (double)value[4];
            //            console.print("\tReceived Frequency (Raw): "); console.print(frequency, 4);
            frequency = frequency / (double)10;
            //            console.print("\t/10\t"); console.println(frequency, 4);
            Current_Data_Record.field.frequency = frequency;
            break;
        }
        case Request_Sensor_Temperature: {                                                          // [8] Sensor Temperature
            double temperature = (double)(value[3] << 8) + (double)value[4];
            //            console.print("\tReceived Sensor Temperature (Raw): "); console.print(temperature, 4);
                        // no conversion
            //            console.print("\t\t"); console.println(temperature, 4);
            Current_Data_Record.field.sensor_temperature = temperature;
            break;
        }
        default: {
            console.print(millis(), DEC); console.println("\tRequested Fields != Received Fields");
            break;
        }
        }
        Update_Current_TimeInfo();
        for (int x = 0; x < 10; x++) {
            Current_Data_Record.field.ldate[x] = Current_Date_With[x];
        }
        for (int x = 0; x < 8; x++) {
            Current_Data_Record.field.ltime[x] = Current_Time_With[x];
        }
    }
}
void Check_Green_Switch() {
    Green_Switch.update();
    if (Green_Switch.fell()) {
        console.print(millis(), DEC); console.println("\tGreen Button Pressed");
        Green_Switch_Pressed = true;
    }
    if (Green_Switch.rose()) {
        console.print(millis(), DEC); console.println("\tGreen Button Released");
        Green_Switch_Pressed = false;
    }
}
void Check_Blue_Switch() {
    Blue_Switch.update();
    if (Blue_Switch.fell()) {
        console.print(millis(), DEC); console.println("\tBlue Button Pressed");
        Blue_Switch_Pressed = true;
    }
    if (Blue_Switch.rose()) {
        console.print(millis(), DEC); console.println("\tBlue Button Released");
        Blue_Switch_Pressed = false;
    }
}
void Check_Gas_Switch() {
    Gas_Switch.update();                                                            // read the gas switch
    if (Gas_Switch.fell()) {
        console.print(millis(), DEC); console.println("\tGas Switch Fell");
        Current_Data_Record.field.gas_volume += Gas_Volume_Per_Sensor_Rise;         // increment the gas volume when gas switch on rising edge
        digitalWrite(Green_led_pin, !digitalRead(Green_led_pin));                   // flash the green light if Gas Switch changed
    }

}
void Check_Red_Switch() {
    while (1) {
        digitalWrite(Green_led_pin, !digitalRead(Green_led_pin));
        digitalWrite(Blue_led_pin, !digitalRead(Blue_led_pin));
        Red_Switch.update();
        if (Red_Switch.fell()) {
            console.print(millis(), DEC); console.print("Red Button Pressed");
            ESP.restart();
        }
        delay(500);
    }
}
void Check_Yellow_Switch() {
    Yellow_Switch.update();
    if (Yellow_Switch.fell()) {
        console.print(millis(), DEC); console.print("Yellow Button Pressed");
        Yellow_Switch_Pressed = true;
    }
    if (Yellow_Switch.rose()) {
        console.print(millis(), DEC); console.print("Yellow Button Released");
        Yellow_Switch_Pressed = false;
    }
}
void Clear_Arrays() {                                           // clear the web arrays of old records
    for (int x = 0; x < Data_Table_Size; x++) {
        for (int y = 0; y < Data_Table_Record_Length; y++)
            Readings_Table[x].characters[y] = '0';
    }
    Data_Table_Pointer = 0;
    for (int x = 0; x < Data_Record_Length; x++) {
        Current_Data_Record.character[x] = 0;
    }
}
void SD_Led_Flash_Start_Stop(bool state) {
    if (state) {
        digitalWrite(Blue_led_pin, HIGH);                  // turn the led on (flash)
        sd_on_time = millis();                                  // set the start time (immediate)
        sd_off_time = millis() + (unsigned long)300;            // set the stoptime (start + period)
    }
    else {
        digitalWrite(Blue_led_pin, LOW);                   // turn the led off
    }
}
void Flash_SD_LED() {
    if (millis() > sd_on_time && millis() < sd_off_time) {      // turn the led on (flash)
        digitalWrite(Blue_led_pin, HIGH);
    }
    else {
        digitalWrite(Blue_led_pin, LOW);
    }
    if (millis() >= (sd_off_time)) {                             // turn the led on (flash)
        sd_on_time = millis() + (unsigned long)300;                // set the next on time (flash)
        sd_off_time = millis() + (unsigned long)600;
    }
}
void SetTimeZone(String timezone) {
    console.print(millis(), DEC); console.println("\tSetting Timezone to " + String(timezone));
    setenv("TZ", timezone.c_str(), 1);                                                  // ADJUST CLOCK SETINGS TO LOCAL TIME
    tzset();
}
void Update_Current_TimeInfo() {
    int connection_attempts = 0;
    String temp;
    while (!getLocalTime(&timeinfo)) {                                                  // get date and time from ntpserver
        console.print(millis(), DEC); console.println("\tAttempting to Get Date " + String(connection_attempts));
        delay(500);
        Check_Red_Switch();
        connection_attempts++;
        if (connection_attempts > 20) {
            console.print(millis(), DEC); console.println("\tTime Network Error, Restarting");
            ESP.restart();
        }
    }
    Current_Date_Time_Data.field.Year = timeinfo.tm_year + 1900;
    Current_Date_Time_Data.field.Month = timeinfo.tm_mon + 1;
    Current_Date_Time_Data.field.Day = timeinfo.tm_mday;
    Current_Date_Time_Data.field.Hour = timeinfo.tm_hour;
    Current_Date_Time_Data.field.Minute = timeinfo.tm_min;
    Current_Date_Time_Data.field.Second = timeinfo.tm_sec;
    // DATE -----------------------------------------------------------------------------------------------------------
    Current_Date_With = String(Current_Date_Time_Data.field.Year);            //  1951
    Current_Date_Without = String(Current_Date_Time_Data.field.Year);            //  1951
    Current_Date_With += "/";                                             //  1951/
    if (Current_Date_Time_Data.field.Month < 10) {
        Current_Date_With += "0";                                             //  1951/0
        Current_Date_Without += "0";
    }
    Current_Date_With += String(Current_Date_Time_Data.field.Month);          //  1951/11
    Current_Date_Without += String(Current_Date_Time_Data.field.Month);
    Current_Date_With += "/";                                             //  1951/11/

    if (Current_Date_Time_Data.field.Day < 10) {
        Current_Date_With += "0";                                             //  1951/11/0
        Current_Date_Without += "0";
    }
    Current_Date_With += String(Current_Date_Time_Data.field.Day);            //  1951/11/18
    Current_Date_Without += String(Current_Date_Time_Data.field.Day);            //  1951/11/18
    // TIME -----------------------------------------------------------------------------------------------------------
    Current_Time_With = "";
    Current_Time_Without = "";
    if (Current_Date_Time_Data.field.Hour < 10) {                               // if hours are less than 10 add a 0
        Current_Time_With = "0";
        Current_Time_Without = "0";
    }
    Current_Time_With += String(Current_Date_Time_Data.field.Hour);           //  add hours
    Current_Time_Without += String(Current_Date_Time_Data.field.Hour);           //  add hours
    Current_Time_With += ":";                                             //  23:
    if (Current_Date_Time_Data.field.Minute < 10) {                             //  if minutes are less than 10 add a 0
        Current_Time_With += "0";                                             //  23:0
        Current_Time_Without += "0";
    }
    Current_Time_With += String(Current_Date_Time_Data.field.Minute);         //  23:59
    Current_Time_Without += String(Current_Date_Time_Data.field.Minute);         //  23:59
    Current_Time_With += ":";                                             //  23:59:
    if (Current_Date_Time_Data.field.Second < 10) {
        Current_Time_With += "0";
        Current_Time_Without += "0";
    }
    Current_Time_With += String(Current_Date_Time_Data.field.Second);         //  23:59:59
    Current_Time_Without += String(Current_Date_Time_Data.field.Second);         //  23:59:59
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
    // Temperature ----------------------------------------------------------------------------------------------------
    start = payload.indexOf("temp\":");                 // "temp":272.77,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Current_Data_Record.field.weather_temperature = (double)(atof(Parse_Output)) - (double)273.15;
    // Temperature Feels Like -----------------------------------------------------------------------------------------
    start = payload.indexOf("feels_like");              // "feels_like":283.47,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Current_Data_Record.field.temperature_feels_like = (double)(atof(Parse_Output)) - (double)273.15;
    // Temperature Maximum --------------------------------------------------------------------------------------------
    start = payload.indexOf("temp_max");                // "temp_max":284.89,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Current_Data_Record.field.temperature_maximum = (double)(atof(Parse_Output)) - (double)273.15;
    // Temperature Minimum --------------------------------------------------------------------------------------------
    start = payload.indexOf("temp_min");                // "temp_min":282.75,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Current_Data_Record.field.temperature_minimum = (double)(atof(Parse_Output)) - (double)273.15;
    // Pressure -------------------------------------------------------------------------------------------------------
    start = payload.indexOf("pressure");                // "pressure":1018,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Current_Data_Record.field.atmospheric_pressure = (double)atof(Parse_Output);
    // humidity -------------------------------------------------------------------------------------------------------
    start = payload.indexOf("humidity\":");             // "humidity":95}
    start = payload.indexOf(":", start);
    end = payload.indexOf("}", start);
    parse(payload, start, end);
    Current_Data_Record.field.relative_humidity = (double)atof(Parse_Output);
    // weather description --------------------------------------------------------------------------------------------
    start = payload.indexOf("description");             // "description":"overcast clouds",
    start = (payload.indexOf(":", start) + 1);
    end = (payload.indexOf(",", start) - 1);
    parse(payload, start, end);
    strncpy(Current_Data_Record.field.weather_description, Parse_Output, sizeof(Current_Data_Record.field.weather_description));
    // wind speed -----------------------------------------------------------------------------------------------------
    start = payload.indexOf("speed");                       // "speed":2.57,
    start = payload.indexOf(":", start);
    end = payload.indexOf(",", start);
    parse(payload, start, end);
    Current_Data_Record.field.wind_speed = (double)(atof(Parse_Output));
    // wind direction -------------------------------------------------------------------------------------------------
    start = payload.indexOf("deg");                         // "deg":20
    start = payload.indexOf(":", start);
    end = payload.indexOf("}", start);
    parse(payload, start, end);
    Current_Data_Record.field.wind_direction = (double)atof(Parse_Output);
}
void parse(String payload, int start, int end) {
    int ptr = 0;
    for (int pos = start + 1; pos < end; pos++) {
        Parse_Output[ptr++] = payload[pos];
    }
    Parse_Output[ptr] = '\0';
}