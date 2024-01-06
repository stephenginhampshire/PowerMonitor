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
*/
String version = "V12.16";
// compiler directives ------
//#define PRiNT_PREFiLL_DATA_VALUES       //
//#define PRiNT_SHUFFLiNG_DATA_VALUES
//#define PRiNT_MONTH_DATA_VALUES
// definitions --------------
#define console Serial
#define RS485_Port Serial2
#define WDT_TiMEOUT 5 // 5 second watchdog timeout
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
// -----------
constexpr int eeprom_size = 30;      // year = 4, month = 4, day = 4, hour = 4, minute = 4, second = 4e
const char* host = "esp32";
const char* ssid = "Woodleigh";
const char* password = "2008198399";
String hostname = "Power Monitor " + version;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;                         // offset for the date and time function
const char city[] = { "Basingstoke\0" };
const char region[] = { "uk\0" };
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
constexpr int RS485_Enable_pin = 22;
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
int Data_Record_Count = 0;           // running total of records written to Data Table
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
    unsigned int characters[Data_Table_Record_Length];
};
Data_Table_Union Readings_Table[Data_Table_Size];
/// ConcatenationFile Constants and Variables -------------
File ConcatenationFile;
String ConcatenationFileName = "M202301";       // .cvs
int Concatenation_Data_Record_Count = 0;
File SourceDataFile;
String SourceFileName = "19511118.csv";
bool Month_End_Process_Status = false;
String Month_End_Process_Started = "1951/11/18 20:00";
long Month_End_Records_Processed = 0;
// KWS Request Field Numbers 
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
                        {0x02,0x03,0x00,0x0E,0x00,0x01,0xE5,0xFA},	// [0]  Request Voltage, in tenths of a volt
                        {0x02,0x03,0x00,0x0F,0x00,0x02,0xF4,0x3B},	// [1]  Request Amperage, in milli-amps
                        {0x02,0x03,0x00,0x11,0x00,0x02,0x94,0x3D},	// [2]  Request Watts, in tenths of a watt
                        {0x02,0x03,0x00,0x19,0x00,0x01,0x55,0xFE},	// [3]  Request uptime, in minutes
                        {0x02,0x03,0x00,0x17,0x00,0x02,0x74,0x3C},// [4]  Request kilo_watt_hour, in milli-watts
                        {0x02,0x03,0x00,0x1D,0x00,0x01,0x14,0x3F},// [5]  Request power factor, in one hundredths
                        {0x02,0x03,0x00,0x1E,0x00,0x01,0xE4,0x3F},// [6]  Request Hertz, in tenths of a hertz
                        {0x02,0x03,0x00,0x1A,0x00,0x01,0xA5,0xFE},// [7]  Request temperature, in degrees centigrade
};
String FileNames[50];
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
//double Cumulative_kwh = 0;
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
uint64_t SD_freespace = 0;
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
const char* loginIndex = {
    "<form name = 'loginForm'>"
        "<table width='20%' bgcolor='A09F9F' align='center'>"
            "<tr>"
                "<td colspan=2>"
                    "<center><font size=4><b>Power Monitor Firmware Update Login</b></font></center>"
                    "<br>"
                "</td>"
                "<br>"
                "<br>"
            "</tr>"
            "<tr>"
                "<td>Username:</td>"
                "<td><input type='text' size=25 name='userid'><br></td>"
            "</tr>"
            "<br>"
            "<br>"
            "<tr>"
                "<td>Password:</td>"
                "<td><input type='Password' size=25 name='pwd'><br></td>"
                "<br>"
                "<br>"
            "</tr>"
            "<tr>"
                "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
            "</tr>"
        "</table>"
    "</form>"
    "<script>"
        "function check(form)"
        "{"
        "if(form.userid.value=='W00dle1gh' && form.pwd.value=='W00dle1gh')"
        "{"
        "window.open('/serverIndex')"
        "}"
        "else"
        "{"
        " alert('Error Password or Username')"
        "}"
        "}"
"</script>" };
const char* serverIndex = {
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
        "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
        "$('form').submit(function(e){"
        "e.preventDefault();"
        "var form = $('#upload_form')[0];"
        "var data = new FormData(form);"
        " $.ajax({"
        "url: '/update',"
        "type: 'POST',"
        "data: data,"
        "contentType: false,"
        "processData:false,"
        "xhr: function() {"
        "var xhr = new window.XMLHttpRequest();"
        "xhr.upload.addEventListener('progress', function(evt) {"
        "if (evt.lengthComputable) {"
        "var per = evt.loaded / evt.total;"
        "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
        "}"
        "}, false);"
        "return xhr;"
        "},"
        "success:function(d, s) {"
        "console.log('success!')"
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
"   </script>" };
// setup -----
void setup() {
    console.begin(console_Baudrate);                                        // enable the console
    while (!console);                                                       // wait for port to settle
    delay(4000);
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
    Red_Switch.interval(5);              // sets debounce time
    Blue_Switch.interval(5);
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
    //    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(hostname.c_str()); //define hostname
    StartWiFi(ssid, password);                  // Start WiFi
    initTime("GMT0BST,M3.5.0/1,M10.5.0");   // initialise the Time library to London, GMT0BST,M3.5.0/1,M10.5.0
    console_print("Starting Server");
    server.begin();                             // Start Webserver
    console_print("Server Started");
    server.on("/", i_Display);
    server.on("/Firmware", HTTP_GET, []() {
        console_print("Firmware Upload Requested via Webpage");
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", loginIndex);
        });
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
    server.on("/Reset", i_Web_Reset);               // reset the processor from the webpage
    server.on("/WhatsApp", i_WhatsApp);             // send a test Whatsapp message from the webpage
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
        }, []() {
            HTTPUpload& upload = server.upload();
            if (upload.status == UPLOAD_FILE_START) {
                console_print("Update: " + upload.filename);
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }
            else if (upload.status == UPLOAD_FILE_WRITE) {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                    Update.printError(Serial);
                }
            }
            else if (upload.status == UPLOAD_FILE_END) {
                if (Update.end(true)) {
                    console_print("Update Success: Rebooting");
                    i_Display();
                }
                else {
                    Update.printError(Serial);
                }
            }
            });
    console_print("Starting Server");
    server.begin();                             // Start Webserver
    console_print("Server Started");
    RS485_Port.begin(RS485_Baudrate, SERIAL_8N1, RS485_Rx_pin, RS485_Tx_pin);
    pinMode(RS485_Enable_pin, OUTPUT);
    delay(10);
    console_print("SD Configuration Commenced");
    if (!SD.begin(SS_pin)) {
        console_print("SD Drive Begin Failed");
        while (true) {
            Check_Red_Switch("SD Drive Begin Failed");
        }
    }
    else {
        console_print("SD Drive Begin Succeeded");
        uint8_t cardType = SD.cardType();
        while (SD.cardType() == CARD_NONE) {
            console_print("No SD Card Found");
            Check_Red_Switch("No SD Card Found");
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
        console_print("SD Card Type: " + card);
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        console_print("SD Card Size : " + String(cardSize) + "MBytes");
        console_print("SD Total Bytes : " + String(SD.totalBytes()));
        console_print("SD Used bytes : " + String(SD.usedBytes()));
        console_print("SD Card initialisation Complete");
    }
    console_print("SD Configuration Complete");
    console_print("Date and Time Update");
    Update_Current_Timeinfo();                                              // update This date time info, no /s
    console_print("Setting Previous Date and Time to Today Date and Time");
    Current_Date_Time_Data.field.Day = New_Date_Time_Data.field.Day;
    Current_Date_Time_Data.field.Month = New_Date_Time_Data.field.Month;
    Last_Boot_Time_With = Current_Time_With;
    Last_Boot_Date_With = Current_Date_With;
    This_Date_With = Current_Date_With;
    This_Time_With = Current_Time_With;
    Create_or_Open_New_Data_File();
    console_print("Preparing Customised Weather Request");
    int count = 0;
    for (int x = 0; x <= 120; x++) {
        if (incomplete_Weather_Api_Link[x] == '\0') break;
        if (incomplete_Weather_Api_Link[x] != '#') {
            Complete_Weather_Api_Link[count] = incomplete_Weather_Api_Link[x];
            count++;
        }
        else {
            for (int y = 0; y < strlen(city); y++) {
                Complete_Weather_Api_Link[count++] = city[y];
            }
            Complete_Weather_Api_Link[count++] = ',';
            for (int y = 0; y < strlen(region); y++) {
                Complete_Weather_Api_Link[count++] = region[y];
            }
        }
    }
    console_print("Weather Request Created: " + String(Complete_Weather_Api_Link));
    digitalWrite(Blue_led_pin, LOW);
    console_print("End of Setup");
    console_print("Running in Full Function Mode");
} // end of Setup
//------------
void loop() {
    if (!Once) {
        console_print("Main Process Now Running");
        Once = true;
        digitalWrite(Green_led_pin, HIGH);
        Send_WhatsApp_Message("Power Monitor - System Rebooted " + Standard_Format_Date_Time);
    }
    Check_WiFi();                                                          // check that the WiFi is still connected
    Check_Blue_Switch(false);
    Check_Gas_Switch();                                              // check if the gas switch sense pin has risen
    Check_NewDay();                                                                // check if the date has changed
    server.handleClient();                                                  // handle any messages from the website
    if (millis() > Last_Cycle + (unsigned long)5000) {          // send requests every 5 seconds (5000 millisecods)
        Get_Sensor_Data();
    }
    Check_SD_Space();
}
//------------
void Get_Sensor_Data() {
    //   console_print("Requesting Current Sensor and Weather information");
    Last_Cycle = millis();                                             // update the last read milli second reading
    // weather information request start ---
    HTTPClient http;
    http.begin(Complete_Weather_Api_Link);                                           // start the weather connectio
    int httpCode = http.GET();                                                                  // send the request
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Parse_Weather_info(payload);
        }
        else {
            console_print("Obtaining Weather information Failed, Return code: " + String(httpCode));
        }
        http.end();
    }
    // weather information request end -----
    // sensor information request start ----
    for (int i = 0; i <= Number_of_RS485_Requests; i++) {     // transmit the requests, assembling the Values array
        Send_Request(i);                                            // send the RS485 Port the requests, one by one
        Receive(i);                                                                    // receive the sensor output
    }                                                                         // all values should now be populated
// sensor information request end ------
    Update_Current_Timeinfo();
    Current_Date_With.toCharArray(Current_Data_Record.field.ldate, Current_Date_With.length() + 1);
    Current_Time_With.toCharArray(Current_Data_Record.field.ltime, Current_Time_With.length() + 1);
    Write_New_Data_Record_to_Data_File();                                       // write the new record to SD Drive
    Add_New_Data_Record_to_Display_Table();                                  // add the record to the display table
}// end of if millis >5000
void Check_NewDay() {
    Update_Current_Timeinfo();                                                      // update the Today_Date and Time
    if ((New_Date_Time_Data.field.Day != Current_Date_Time_Data.field.Day)) {       // is the day different to previous day
        if (Current_Date_Time_Data.field.Day < 1 || Current_Date_Time_Data.field.Day > 31) { // if the Previous Date not set, we have rebooted
            console_print("Reopening Data File");
            Create_or_Open_New_Data_File();                                         // open the day file, do not create a new one
            Send_WhatsApp_Message("Power Monitor - Day Data File Reopened " + Standard_Format_Date_Time);
        }
        else {
            console_print("New Day Detected");                                      // proven day change
            Create_or_Open_New_Data_File();                                         // create a new day file
            Send_WhatsApp_Message("Power Monitor - New Day Data File Opened " + Standard_Format_Date_Time);
            Check_NewMonth();                                                       // if the date changed check if a new month started
        }
        Clear_Arrays();                                                             // clear memory
        Current_Date_Time_Data.field.Day = New_Date_Time_Data.field.Day;         // so set yesterday month
    }
}
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
void console_print(String Monitor_Message) {
    console.print(millis(), DEC); console.println("\t" + Monitor_Message);
}
void Month_End_Process() {
    int month_datatemp = 0;
    char month_field[50];
    int month_character_count = 0;
    int month_data_field_number = 0;
    int file_count = 0;
    int csv_count = 0;
    String month_end_file_month = "00";
    String this_file_month = "00";
    console_print("Commencing Month End Process for Month " + String(Month_End_Date_Time_Data.field.Month));
    Send_WhatsApp_Message("Power Monitor - Month End Process Commenced for " + String(Month_Names[Month_End_Date_Time_Data.field.Month]));
    Month_End_Process_Status = true;
    Month_End_Process_Started = Current_Date_With + " " + Current_Time_With;
    ConcatenationFileName = "MF";                                               // "MF" - create the month file name
    ConcatenationFileName += String(Month_End_Date_Time_Data.field.Year);       // "MF1951"
    if (Month_End_Date_Time_Data.field.Month < 10) {
        month_end_file_month = "0" + String(Month_End_Date_Time_Data.field.Month);
    }
    else {
        month_end_file_month = String(Month_End_Date_Time_Data.field.Month);
    }
    ConcatenationFileName += month_end_file_month + ".csm";                                            // "MF195111.csm"
    console_print("Month End Stage 1. Create a Concatenation File - " + ConcatenationFileName);
    if (SD.exists("/" + ConcatenationFileName)) {           // check if the monthly file exists, if so delete it
        SD.remove("/" + ConcatenationFileName);
        console_print("\tPrevious " + ConcatenationFileName + " deleted");
    }
    ConcatenationFile = SD.open("/" + ConcatenationFileName, FILE_WRITE);
    if (!ConcatenationFile) {                    // log file not opened
        console_print("Error opening Concatenation file (" + String(ConcatenationFileName) + ")");
        Check_Red_Switch("Power Monitor - Error opening Concatenation file");
    }
    else {
        console_print("\tConcatenation File " + ConcatenationFileName + " created");
    }
    console_print("\tWriting column headings into Concatenation File");
    for (int x = 0; x < Number_of_Column_Field_Names - 1; x++) {    // write data column headings into the SD file
        ConcatenationFile.print(Column_Field_Names[x]);
        ConcatenationFile.print(",");
    }
    // write the column headings to the concatenation file
    ConcatenationFile.println(Column_Field_Names[Number_of_Column_Field_Names - 1]);
    console_print("\tColumn Titles written to new Concatenation File");
    ConcatenationFile.close();
    ConcatenationFile.flush();
    console_print("\tConcatenation File Closed");
    console_print("Stage 2. Create an array of the .csv files names");
    file_count = Count_Files_on_SD_Drive();                                       // creates an array of filenames
    console_print("\tThere are currently " + String(file_count) + " files on the SD Drive");
    csv_count = 0;
    console_print("\tLooking for .csv files with month of " + month_end_file_month);
    for (int file = 0; file < file_count; file++) {                             // select .cvs file names from the month end month
        //        console_print("Character at position 4 of file name = [" + String(FileNames[file].charAt(4)) + "]");
        if (FileNames[file].indexOf(month_end_file_month) == 4) {               // MF1951xx.csv
            //            console_print("\t\tFile Name: " + FileNames[file] + " contains the required month number in the correct position");
            //            console_print("Character at position 8 of file name = [" + String(FileNames[file].charAt(8)) + "]");
            if (FileNames[file].indexOf(".csv") == 8) {                         // 012345678select only ,csv files
                //                console_print("\t\tFile Name " + FileNames[file] + " does have a .csv extension");
                Csv_File_Names[csv_count] = "";
                for (int x = 0; x < 8; x++) {
                    Csv_File_Names[csv_count] += FileNames[file][x];          // move the filename, minus the extension
                }
                csv_count++;                                                  // increment the count of csv file names
            }
            else {
                console_print("\tFile Name " + FileNames[file] + " does not have a .csv extension");
            }
        }
        else {
            console_print("\tFile Name: " + FileNames[file] + " does not contain the required month number in the correct position");
        }
    }
    if (!csv_count) {
        console_print("Month End No relevant files found");
        console_print("Month End Process Terminated");
        Send_WhatsApp_Message("Power Monitor - Month End Process Terminated - No relevant files to concatenate, " + ConcatenationFileName + " Removed");
        Month_End_Process_Status = false;
        return;             // Abort the Month End Routine
    }
    console_print("\tThere are " + String(csv_count) + ".csv files");
    console_print("\tSort the .csv file names into date order");
    sortArray(Csv_File_Names, csv_count);         // sort into rising order
    console_print("\tList the Sorted Files");
    for (int x = 0; x < csv_count; x++) {
        console_print("\t\tFile Name Found " + Csv_File_Names[x]);
    }
    console_print("\tEnd of Sorted File List");
    console_print("Stage 3. Concatenating .csv files into " + ConcatenationFileName);
    for (int file = 0; file < csv_count; file++) {                                            // for each csv file
        ConcatenationFile = SD.open("/" + ConcatenationFileName, FILE_APPEND);    // open the month file for append
        if (!ConcatenationFile) {                                                 // log file not opened
            console_print("Error opening Concateation file (" + String(ConcatenationFileName) + ")");
            Check_Red_Switch("Power Monitor - Error opening Concatenation File");
        }
        Concatenation_Data_Record_Count = 0;
        SourceFileName = Csv_File_Names[file] + ".csv";
        SourceDataFile = SD.open("/" + SourceFileName, FILE_READ);                            // open the data file
        if (!SourceDataFile) {                                                               // log file not opened
            console_print("Error opening Source file (" + String(SourceFileName) + ")");
            Check_Red_Switch("Power Monitor - Error opening Source File");
        }
        else {
            console_print("\tMonth End - Processing " + SourceFileName);
        }
        while (SourceDataFile.available()) {                                // throw the first row of each file away (column headers)
            month_datatemp = SourceDataFile.read();                         // read a character from the DataFile
            if (month_datatemp == '\n') break;                              // finish when end of row detected
        }
        while (SourceDataFile.available()) {                                // do while there are data available
            digitalWrite(Blue_led_pin, !digitalRead(Blue_led_pin));         // flash the blue led
            month_datatemp = SourceDataFile.read();                         // read a character from the DataFile
            month_field[month_character_count++] = month_datatemp;          // add the read character to the csvfield string
            if (month_datatemp == ',' || month_datatemp == '\n') {          // look for end of field, comma or end of row
                month_field[month_character_count - 1] = '\0';              // make Field a String
                switch (month_data_field_number) {
                case 0: {
                    for (int x = 0; x < 11; x++) {
                        Concatenation_Data_Record.field.ldate[x] = month_field[x];
                    }
                    //                    console_print("\tMonth Data: Date: " + String(Concatenation_Data_Record.field.ldate));
                    break;
                }
                case 1: {
                    for (int x = 0; x < 9; x++) {
                        Concatenation_Data_Record.field.ltime[x] = month_field[x];
                    }
                    //                    console_print("\tMonth Data: Time: " + String(Concatenation_Data_Record.field.ltime));
                    break;
                }
                case 2: {
                    Concatenation_Data_Record.field.Voltage = atof(month_field);                         // Voltage
                    //                    console_print("\tMonth Data: Voltage: " + String(Concatenation_Data_Record.field.Voltage));
                    break;
                }
                case 3: {
                    Concatenation_Data_Record.field.Amperage = atof(month_field);                       // Amperage
                    //                    console_print("\tMonth Data: Amperage: " + String(Concatenation_Data_Record.field.Amperage));
                    break;
                }
                case 4: {
                    Concatenation_Data_Record.field.wattage = atof(month_field);                         // Wattage
                    //                    console_print("\tMonth Data: Wattage: "+ String(Concatenation_Data_Record.field.wattage));
                    break;
                }
                case 5: {
                    Concatenation_Data_Record.field.uptime = atof(month_field);                           // Uptime
                    //                    console_print("\tMonth Data: Uptime: " + String(Concatenation_Data_Record.field.uptime));
                    break;
                }
                case 6: {
                    Concatenation_Data_Record.field.kilowatthour = atof(month_field);               // Kilowatthour
                    //                    console_print("\tMonth Data: Kilowatthours: " + String(Concatenation_Data_Record.field.kilowatthour));
                    break;
                }
                case 7: {
                    Concatenation_Data_Record.field.powerfactor = atof(month_field);                // Power Factor
                    //                    console_print("\tMonth Data: Power Factor: " + String(Concatenation_Data_Record.field.powerfactor));
                    break;
                }
                case 8: {
                    Concatenation_Data_Record.field.frequency = atof(month_field);                     // Frequency
                    //                    console_print("\tMonth Data: Frequency: " + String(Concatenation_Data_Record.field.frequency));
                    break;
                }
                case 9: {
                    Concatenation_Data_Record.field.sensor_temperature = atof(month_field);   // Sensor Temperature
                    //                    console_print("\tMonth Data: Sensor Temperature: " + String(Concatenation_Data_Record.field.sensor_temperature));
                    break;
                }
                case 10: {
                    Concatenation_Data_Record.field.weather_temperature = atof(month_field); // Weather Temperature
                    //                    console_print("\tMonth Data: Weather Temperature: " + String(Concatenation_Data_Record.field.weather_temperature));
                    break;
                }
                case 11: {
                    Concatenation_Data_Record.field.temperature_feels_like = atof(month_field);  // Temp Feels Like
                    //                    console_print("\tMonth Data: Temperature Feels Like: " + String(Concatenation_Data_Record.field.temperature_feels_like));
                    break;
                }
                case 12: {
                    Concatenation_Data_Record.field.temperature_maximum = atof(month_field); // Temperature Maximum
                    //                    console_print("\tMonth Data: Maximum Temperatre: " + String(Concatenation_Data_Record.field.temperature_maximum));
                    break;
                }
                case 13: {
                    Concatenation_Data_Record.field.temperature_minimum = atof(month_field); // Temperature Minimum
                    //                    console_print("\tConcatenation_Data: Temperature Minimum: " + String(Concatenation_Data_Record.field.temperature_minimum));
                    break;
                }
                case 14: {
                    Concatenation_Data_Record.field.atmospheric_pressure = atof(month_field);     // Atmos Pressure
                    //                    console_print("\tMonth Data: Atmospheric Pressure: " + String(Concatenation_Data_Record.field.atmospheric_pressure));
                    break;
                }
                case 15: {
                    Concatenation_Data_Record.field.relative_humidity = atof(month_field);     // Relative Humidity
                    //                    console_print("\tMonth Data: Relative Humidity: " + String(Concatenation_Data_Record.field.relative_humidity));
                    break;
                }
                case 16: {
                    Concatenation_Data_Record.field.wind_speed = atof(month_field);                   // Wind Speed
                    //                    console_print("\tMonth Data: Wind Speed: " + String(Concatenation_Data_Record.field.wind_speed));
                    break;
                }
                case 17: {
                    Concatenation_Data_Record.field.wind_direction = atof(month_field);           // Wind Direction
                    //                    console_print("\tMonth Data: Wind Direction: " + String(Concatenation_Data_Record.field.wind_direction));
                    break;
                }
                case 18: {
                    Concatenation_Data_Record.field.gas_volume = atof(month_field);                   // Gas Volume
                    if (Concatenation_Data_Record.field.gas_volume == 0) {          // 1st record of each data file
                        Concatenation_Data_Record.field.gas_volume += Cumulative_Gas_Volume; // accumulate gas volume
                    }
                    Cumulative_Gas_Volume = Concatenation_Data_Record.field.gas_volume;          // save the volume
                    //                    console_print("\tMonth Data: Gas Volume: " + String(Concatenation_Data_Record.field.gas_volume));
                    break;
                }
                case 19: {
                    for (int x = 0; x < month_character_count; x++) {
                        Concatenation_Data_Record.field.weather_description[x] = month_field[x];
                    }
                    //                    console_print("\tMonth Data: Weather Description: " + String(Concatenation_Data_Record.field.weather_description));
                    break;
                }
                default: {
                    break;
                }
                } // end of switch
                month_data_field_number++;
                month_field[0] = '\0';
                month_character_count = 0;
            } // assembling the fields - end of if line end  
            if (month_datatemp == '\n') {// if the character /n (end of line) save fields to the Concatenation File
                month_data_field_number = 0;
                ConcatenationFile.print(Concatenation_Data_Record.field.ldate);
                ConcatenationFile.print(",");                                                               // [00]
                ConcatenationFile.print(Concatenation_Data_Record.field.ltime);
                ConcatenationFile.print(",");                                                               // [01]
                ConcatenationFile.print(Concatenation_Data_Record.field.Voltage);
                ConcatenationFile.print(",");                                                               // [02]
                ConcatenationFile.print(Concatenation_Data_Record.field.Amperage);
                ConcatenationFile.print(",");                                                               // [03]
                ConcatenationFile.print(Concatenation_Data_Record.field.wattage);
                ConcatenationFile.print(",");                                                               // [04]
                ConcatenationFile.print(Concatenation_Data_Record.field.uptime);
                ConcatenationFile.print(",");                                                               // [05]
                ConcatenationFile.print(Concatenation_Data_Record.field.kilowatthour);
                ConcatenationFile.print(",");
                ConcatenationFile.print(Concatenation_Data_Record.field.powerfactor);
                ConcatenationFile.print(",");
                ConcatenationFile.print(Concatenation_Data_Record.field.frequency);
                ConcatenationFile.print(",");
                ConcatenationFile.print(Concatenation_Data_Record.field.sensor_temperature);
                ConcatenationFile.print(",");                                                               // [09]
                ConcatenationFile.print(Concatenation_Data_Record.field.weather_temperature);
                ConcatenationFile.print(",");                                                               // [10]
                ConcatenationFile.print(Concatenation_Data_Record.field.temperature_feels_like);
                ConcatenationFile.print(",");                                                               // [11]
                ConcatenationFile.print(Concatenation_Data_Record.field.temperature_maximum);
                ConcatenationFile.print(",");                                                               // [12]
                ConcatenationFile.print(Concatenation_Data_Record.field.temperature_minimum);
                ConcatenationFile.print(",");                                                               // [13]
                ConcatenationFile.print(Concatenation_Data_Record.field.atmospheric_pressure);
                ConcatenationFile.print(",");                                                               // [14]
                ConcatenationFile.print(Concatenation_Data_Record.field.relative_humidity);
                ConcatenationFile.print(",");                                                               // [15]
                ConcatenationFile.print(Concatenation_Data_Record.field.wind_speed);
                ConcatenationFile.print(",");                                                               // [16]
                ConcatenationFile.print(Concatenation_Data_Record.field.wind_direction);
                ConcatenationFile.print(",");                                                               // [17]
                ConcatenationFile.print(Concatenation_Data_Record.field.gas_volume);
                ConcatenationFile.print(",");                                                               // [18]
                ConcatenationFile.print(Concatenation_Data_Record.field.weather_description);               // [19]
                // ConcatenationFile.println();                                                    // end of record
                Concatenation_Data_Record_Count++;                            // increment the running record total
                if (millis() > Last_Cycle + (unsigned long)5000) {                 // send requests every 5 seconds
                    Get_Sensor_Data();
                    server.handleClient();                                  // handle any messages from the website
                }
                Month_End_Records_Processed++;
                //                console_print("\tRecord Added to " + ConcatenationFileName + " from " + SourceFileName);
            } // end of end of line detected
        }// end of while
        console_print("\tProcessing of " + SourceFileName + " Complete");
        SourceDataFile.close();                                                               // close the DataFile 
        SourceDataFile.flush();
        ConcatenationFile.close();                                  // close the ConcatenationFile to preserve file
        ConcatenationFile.flush();
    }
    console_print("Stage 5. Delete Data Files");
    String fileName = "/99999999.csv";
    for (int i = 0; i < csv_count; i++) {
        fileName = "/" + Csv_File_Names[i] + ".csv";
        SD.remove(fileName);
        console_print("\t" + fileName + " Removed");
    }
    console_print("\tFile Deletion Completed");
    console_print("Month End Process Complete");
    Month_End_Process_Status = false;
    Send_WhatsApp_Message("Power Monitor - Month End Process Complete, " + ConcatenationFileName + " Now available for download");
}
void Send_WhatsApp_Message(String message) {
    String WhatsApp_url = "https://api.callmebot.com/whatsapp.php?phone=447785938200&apikey=8876034&text=" + urlEncode(message);
    HTTPClient http;
    http.begin(WhatsApp_url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");    // Specify content-type header
    int httpResponseCode = http.POST(WhatsApp_url);      // Send HTTP POST request
    if (httpResponseCode == 200) {
        console_print("WhatsApp Message " + message + " sent successfully");
    }
    else {
        console_print("Error sending the WhatsApp message" + message);
        console_print("WhatsApp HTTP response code: " + httpResponseCode);
    }
    http.end();
}
void Write_New_Data_Record_to_Data_File() {
    digitalWrite(Blue_led_pin, !digitalRead(Blue_led_pin));                              // flash the SD activity LED on
    DataFile = SD.open("/" + DataFileName, FILE_APPEND);                                // open the SD file
    if (!DataFile) {                                                                    // oops - file not available!
        console_print("Error re-opening DataFile:" + String(DataFileName));
        Check_Red_Switch("Power Monitor - Error re-opening DataFile: " + String(DataFileName));
    }
    console_print("New Data Record Written to " + DataFileName);
    DataFile.print(Current_Data_Record.field.ldate); DataFile.print(",");               // [00] Date
    DataFile.print(Current_Data_Record.field.ltime); DataFile.print(",");               // [01] Time
    DataFile.print(Current_Data_Record.field.Voltage); DataFile.print(",");             // [02] Voltage
    DataFile.print(Current_Data_Record.field.Amperage); DataFile.print(",");            // [03] Amperage
    DataFile.print(Current_Data_Record.field.wattage); DataFile.print(",");             // [04] Wattage
    DataFile.print(Current_Data_Record.field.uptime); DataFile.print(",");              // [05] UpTime
    DataFile.print(Current_Data_Record.field.kilowatthour); DataFile.print(",");        // [06] Kiliowatthour
    DataFile.print(Current_Data_Record.field.powerfactor); DataFile.print(",");         // [07] Power Factor
    DataFile.print(Current_Data_Record.field.frequency); DataFile.print(",");           // [08] Frequency
    DataFile.print(Current_Data_Record.field.sensor_temperature); DataFile.print(",");  // [09] Sensor Temperature
    DataFile.print(Current_Data_Record.field.weather_temperature); DataFile.print(","); // [10] Weather Temperature
    DataFile.print(Current_Data_Record.field.temperature_feels_like); DataFile.print(",");// [11] Temp Feels Like
    DataFile.print(Current_Data_Record.field.temperature_maximum); DataFile.print(","); // [12] Temperature Maximum
    DataFile.print(Current_Data_Record.field.temperature_minimum); DataFile.print(","); // [13] Temperature Minimum
    DataFile.print(Current_Data_Record.field.atmospheric_pressure); DataFile.print(",");// [14] Atmos Pressure
    DataFile.print(Current_Data_Record.field.relative_humidity); DataFile.print(",");   // [15] Relative Humidity
    DataFile.print(Current_Data_Record.field.wind_speed); DataFile.print(",");          // [16] Wind Speed
    DataFile.print(Current_Data_Record.field.wind_direction); DataFile.print(",");      // [17] Wind Direction
    DataFile.print(Current_Data_Record.field.gas_volume); DataFile.print(",");          // [18] Gas Volume
    DataFile.print(Current_Data_Record.field.weather_description);                      // [19] Weather Description
    DataFile.println();                                                                 // end of record
    DataFile.close();                                                                   // close the sd file
    DataFile.flush();                                                        // make sure it has been written to SD
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
        DataFile = SD.open("/" + DataFileName, FILE_WRITE);                         // does not exist so create it for write
        if (!DataFile) {
            console_print("Error creating Data file: " + String(DataFileName));
            Check_Red_Switch("Power Monitor - Error creating Data file: " + String(DataFileName));
        }
        console_print("Day Data File " + DataFileName + " created");
        for (int x = 0; x < Number_of_Column_Field_Names - 1; x++) {                // write data column headings into the SD file
            DataFile.print(Column_Field_Names[x]);
            DataFile.print(",");
        }
        DataFile.println(Column_Field_Names[Number_of_Column_Field_Names - 1]);
        console_print("Column Headings written to Day Data File");
        DataFile.close();
        DataFile.flush();
        console_print("Day Data File Closed and Flushed");
        Data_Record_Count = 0;
    }
    else {                                                                          // file already exists, so open it for append
        DataFile = SD.open("/" + DataFileName, FILE_APPEND);                        // yes so open it for append
        if (!DataFile) {                                                            // log file not opened
            console_print("Error opening Data file: " + String(DataFileName));
            Check_Red_Switch("Power Monitor - Error reopening Data File " + String(DataFileName));
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
void Check_WiFi() {
    int wifi_connection_attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {                              // whilst it is not connected keep trying
        console_print("WiFi Connection Failed, Attempting to Reconnect");
        delay(3000);                                                             // wait 3 seconds before retrying
        console_print("Connection attempt " + String(wifi_connection_attempts));
        if (wifi_connection_attempts++ > 20) {
            console_print("Network Error, WiFi lost >20 seconds, Restarting Processor");
            ESP.restart();
        }
        StartWiFi(ssid, password);
    }
}
int StartWiFi(const char* ssid, const char* password) {
    console_print("WiFi Connecting to " + String(ssid));
    if (WiFi.status() == WL_CONNECTED) {                                // disconnect to start new wifi connection
        console_print("WiFi Already Connected");
        console_print("Disconnecting");
        WiFi.disconnect(true);
        console_print("Disconnected");
    }
    WiFi.begin(ssid, password);                                                     // connect to the wifi network
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
void initTime(String timezone) {
    console_print("Starting Time Server");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    setenv("TZ", timezone.c_str(), 1);                                        // adjust time settings to London
    tzset();
    console_print("Time Server Started");
}
void Prefill_Data_Array() {
    int character_count = 0;
    char field[50];
    int field_number = 0;
    char data_temp;
    String message;
    digitalWrite(Blue_led_pin, !digitalRead(Blue_led_pin));                          // flash the blue led
    console_print("Loading DataFile from " + String(DataFileName));
    File DataFile = SD.open("/" + DataFileName, FILE_READ);
    if (DataFile) {
        while (DataFile.available()) {                                 // throw the first row, column headers, away
            data_temp = DataFile.read();
            if (data_temp == '\n') break;
        }
        while (DataFile.available()) {                                       // do while there are data available
            digitalWrite(Blue_led_pin, !digitalRead(Blue_led_pin));
            data_temp = DataFile.read();
            field[character_count++] = data_temp;                        // add it to the csvfield string
            if (data_temp == ',' || data_temp == '\n') {                              // look for end of field
                field[character_count - 1] = '\0';       // insert termination character where the ',' or '\n' was
                switch (field_number) {
                case 0: {
                    strncpy(Readings_Table[Data_Table_Pointer].field.ldate, field, sizeof(Readings_Table[Data_Table_Pointer].field.ldate));   // Date
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Date: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.ldate);
                    console_print(message);
#endif                    
                    break;
                }
                case 1: {
                    strncpy(Readings_Table[Data_Table_Pointer].field.ltime, field, sizeof(Readings_Table[Data_Table_Pointer].field.ltime));
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Time: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.ltime);
                    console_print(message);
#endif                    
                    break;
                }
                case 2: {
                    Readings_Table[Data_Table_Pointer].field.Voltage = atof(field);     // [02] Voltage
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Voltage: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.Voltage);
                    console_print(message);
#endif                    
                    break;
                }
                case 3: {
                    Readings_Table[Data_Table_Pointer].field.Amperage = atof(field);    // [03] Amperage
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Amperage: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.Amperage);
                    console_print(message);
#endif                    
                    break;
                }
                case 4: {
                    Readings_Table[Data_Table_Pointer].field.wattage = atof(field);     // [04] Wattage
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Wattage: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.wattage);
                    console_print(message);
#endif                    
                    break;
                }
                case 5: {
                    Readings_Table[Data_Table_Pointer].field.uptime = atof(field);      // [05] Uptime
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Uptime: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.uptime);
                    console_print(message);
#endif
                    break;
                }
                case 6: {
                    Readings_Table[Data_Table_Pointer].field.kilowatthour = atof(field);// [06] Kilowatthour
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Kilowatthour: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.kilowatthour);
                    console_print(message);
#endif                    
                    break;
                }
                case 7: {
                    Readings_Table[Data_Table_Pointer].field.powerfactor = atof(field); // [07] Power Factor
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Power Factor: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.powerfactor);
                    console_print(message);
#endif                    
                    break;
                }
                case 8: {
                    Readings_Table[Data_Table_Pointer].field.frequency = atof(field);   // [09] Frequency
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Frequency: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.frequency);
                    console_print(message);
#endif
                    break;
                }
                case 9: {
                    Readings_Table[Data_Table_Pointer].field.sensor_temperature = atof(field);// [10] Sensor Temp
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Sensor Temperature: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.sensor_temperature);
                    console_print(message);
#endif                    
                    break;
                }
                case 10: {
                    Readings_Table[Data_Table_Pointer].field.weather_temperature = atof(field);           // [15] 
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Weather Temperature: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.weather_temperature);
                    console_print(message);
#endif                    
                    break;
                }
                case 11: {
                    Readings_Table[Data_Table_Pointer].field.temperature_feels_like = atof(field);       // [16]
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Temperature Feels Like: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.temperature_feels_like);
                    console_print(message);
#endif                    
                    break;
                }
                case 12: {
                    Readings_Table[Data_Table_Pointer].field.temperature_maximum = atof(field);          // [17]
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Maximum Temperatre: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.temperature_maximum);
                    console_print(message);
#endif                    
                    break;
                }
                case 13: {
                    Readings_Table[Data_Table_Pointer].field.temperature_minimum = atof(field);          // [18]
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Temperature Minimum: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.temperature_minimum);
                    console_print(message);
#endif                    
                    break;
                }
                case 14: {
                    Readings_Table[Data_Table_Pointer].field.atmospheric_pressure = atof(field);         // [19]
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Atmospheric Pressure: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.atmospheric_pressure);
                    console_print(message);
#endif                    
                    break;
                }
                case 15: {
                    Readings_Table[Data_Table_Pointer].field.relative_humidity = atof(field); // [20] Relative Humidity
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Relative Humidity: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.relative_humidity);
                    console_print(message);
#endif                    
                    break;
                }
                case 16: {
                    Readings_Table[Data_Table_Pointer].field.wind_speed = atof(field);          // [21] Wind Speed
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Wind Speed: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.wind_speed);
                    console_print(message);
#endif                    
                    break;
                }
                case 17: {
                    Readings_Table[Data_Table_Pointer].field.wind_direction = atof(field);  // [22] Wind Direction
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Wind Direction: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.wind_direction);
                    console_print(message);
#endif                    
                    break;
                }
                case 18: {
                    Readings_Table[Data_Table_Pointer].field.gas_volume = atof(field);
                    Current_Data_Record.field.gas_volume = Readings_Table[Data_Table_Pointer].field.gas_volume;
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Gas Volume: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.gas_volume);
                    console_print(message);
#endif              
                    if (Current_Data_Record.field.gas_volume != 0) {
                        if (Current_Data_Record.field.gas_volume != Latest_Gas_Volume) {
                            Latest_Gas_Volume = Current_Data_Record.field.gas_volume;
                            Latest_Gas_Date_With = Readings_Table[Data_Table_Pointer].field.ldate;
                            Latest_Gas_Time_With = Readings_Table[Data_Table_Pointer].field.ltime;
                        }
                    }
                    break;
                }
                case 19: {
                    strncpy(Readings_Table[Data_Table_Pointer].field.weather_description, field, sizeof(Readings_Table[Data_Table_Pointer].field.weather_description));
#ifdef PRiNT_PREFiLL_DATA_VALUES
                    message = "\tPrefill Data Array [";
                    message += String(pointer);
                    message += "] Weather Description: ";
                    message += String(Readings_Table[Data_Table_Pointer].field.weather_description);
                    console_print(message);
#endif                    
                    break;
                }
                default: {
                    break;
                }
                }
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
    DataFile.close();
    console_print("Loaded Data Records: " + String(Data_Record_Count));
}
void Shuffle_Data_Table() {
#ifdef PRiNT_SHUFFLiNG_DATA_VALUES
    console_print("Shuffling Data Table");
#endif
    for (int i = 0; i < (Data_Table_Size - 1); i++) {       // shuffle the rows up, losing row 0, make row [table_size] free
#ifdef PRiNT_SHUFFLiNG_DATA_VALUES
        console_print("Shuffling Data Record "); console.print(i + 1); console.print(" to "); console.println((i));
#endif
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
    webpage.reserve(5000);
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
    webpage += F("li a{display:block;text-align:center;padding:5px 25px;text-decoration:none;}");
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
    webpage += F("<li><a href='/WhatsApp'>WhatsApp Test</a></li>");
    webpage += F("<li><a href='/Reset'>Reset</a></li>");
    webpage += F("<li><a href='/Firmware'>Firmware</a></li>");
    webpage += F("</ul>");
    webpage += F("<footer>");
    webpage += F("<p style='text-align: center;'>");
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
    console_print("Web Display of Graph Requested via Webpage");
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
    Page_Footer();
    lastcall = "display";
}
void i_WhatsApp() {
    Send_WhatsApp_Message("Power Monitor - Test Message from Webpage, ");
}
void i_Web_Reset() {
    console_print("Web Reset of Processor Requested via Webpage");
    ESP.restart();
}
void i_Information() {                                                    // Display file size of the datalog file
    int file_count = Count_Files_on_SD_Drive();
    console_print("Web Display of information Requested via Webpage");
    String ht = String(Date_of_Highest_Voltage + " at " + Time_of_Highest_Voltage + " as ");
    ht += String(Highest_Voltage) + " volts";
    String lt = String(Date_of_Lowest_Voltage + " at " + Time_of_Lowest_Voltage + " as ");
    lt += String(Lowest_Voltage) + " volts";
    String ha = String(Date_of_Highest_Amperage + " at " + Time_of_Highest_Amperage + " as ");
    ha += String(Highest_Amperage) + " amps";
    Page_Header(true, "Energy Usage Monitor " + String(Current_Date_With) + " " + String(Current_Time_With));
    File DataFile = SD.open("/" + DataFileName, FILE_READ);                               // Now read data from FS
    // 1.Wifi Signal Strength --------------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true;font-size:14px; '");
    webpage += F(">WiFi Signal Strength = ");
    webpage += String(WiFi_Signal_Strength) + " Dbm";
    webpage += "</span></strong></p>";
    // Row 2.Data File Size -----
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F(">Current Data file size = ");
    webpage += String(DataFile.size());
    webpage += F(" Bytes");
    webpage += "</span></strong></p>";
    // Row 3.Freespace ----------
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
    // Row 4.File Count ---------
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F("'> Number of Files on SD : ");
    webpage += String(file_count);
    webpage += "</span></strong></p>";
    // Row 5.Last Boot Time -----
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    webpage += F("'>Date the System was Booted : ");
    webpage += Last_Boot_Date_With + " at " + Last_Boot_Time_With;
    webpage += "</span></strong></p>";
    // Row 6.Data Record Count --
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
    // Row 19.Month ENd CUrrent File being Processed
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font-size:14px; '");
    if (Month_End_Process_Status) {
        webpage += F(">Currently processing file: ");
        webpage += SourceFileName;
    }
    else {
        webpage += F(" ");
    }
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // Row 20.Month End Records Written Percentage of a month
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font - size:12px; '");
    if (Month_End_Process_Status) {
        webpage += F(">Month End Records Written: ");
        webpage += String(Month_End_Records_Processed);
        webpage += F("Percentage of Full Month : ");
        webpage += String(((double)Month_End_Records_Processed * (double)100) / (((double)24 * (double)60 * (double)60 * (double)31) / (double)5)) + "%";
    }
    else {
        webpage += F(".");
    }
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // 21.Month End Record Written
    webpage += F("<p ");
    webpage += F("style='line-height:75%;text-align:left;'><strong><span style='color:DodgerBlue;");
    webpage += F("bold:true; font - size:12px; '");
    if (Month_End_Process_Status) {
        webpage += F(">Month End Records Written: ");
        webpage += String(Month_End_Records_Processed);
    }
    else {
        webpage += F(".");
    }
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    // -------
    webpage += F("<</h3>");
    DataFile.close();
    Page_Footer();
}
void i_Download_Files() {
    int file_count = Count_Files_on_SD_Drive();             // this counts and creates an array of file names on SD
    sortArray(FileNames, file_count);         // sort into rising order
    console_print("Download of Files Requested via Webpage");
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
void i_Wipe_Files() {                                                // selected by pressing combonation of buttons
    console_print("Start of Wipe Files Request by Switch");
    String filename;
    File root = SD.open("/");                                                          //  Open the root directory
    while (true) {
        File entry = root.openNextFile();                                                    //  get the next file
        if (entry) {
            filename = entry.name();
            console_print("Removing " + filename);
            SD.remove(entry.name());                                                           //  delete the file
        }
        else {
            root.close();
            console_print("All files removed from root directory, rebooting");
            ESP.restart();
        }
    }
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
                if ((filename.indexOf(".csv") > 0) || (filename.indexOf(".csm") > 0)) {
                    FileNames[file_count] = filename;
                    file_count++;                                                     // increment the file count
                }
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
void Send_Request(int field) {
    digitalWrite(RS485_Enable_pin, transmit);                 // set RS485_Enable HiGH to transmit values to RS485
    for (int x = 0; x < 7; x++) {
        RS485_Port.write(RS485_Requests[field][x]);                  // write the Request string to the RS485 Port
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
    unsigned long start_time;                                   // take the start time, used to check for time out
    long value[7] = { 0,0,0,0,0,0,0 };
    digitalWrite(RS485_Enable_pin, receive);                    // set Enable pin low to receive values from RS485
    start_time = millis();                                      // take the start time, used to check for time out
    do {
        while (!RS485_Port.available()) {                                          // wait for some data to arrive
            if (millis() > start_time + (unsigned long)500) {         // no data received within 500 ms so timeout
                console_print("No Reply from RS485 within 500 ms");
                Check_Red_Switch("Power Monitor - No Reply from RS485 within 500ms"); // Reset will restart the processor so no return
            }
        }
        value_status = true;                          // set the value status to true, which is true at this point
        while (RS485_Port.available()) {                                // Data is available so add them to Result
            received_character = RS485_Port.read();                                          // take the character
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
            Current_Data_Record.field.Amperage = Amperage;                   // Amperage output format double nn.n
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
    }
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
    Gas_Switch.update();                                                                // read the gas switch
    if (Gas_Switch.fell()) {
        console_print("Gas Switch Fell");
        Current_Data_Record.field.gas_volume += Gas_Volume_Per_Sensor_Rise; // +gas when gas switch on rising edge
        Latest_Gas_Volume = Current_Data_Record.field.gas_volume;
        Latest_Gas_Date_With = Current_Date_With;
        Latest_Gas_Time_With = Current_Time_With;
        digitalWrite(Green_led_pin, !digitalRead(Green_led_pin)); // flash the green light if Gas Switch changed
    }
    if (Gas_Switch.rose()) {
        console_print("Gas Switch Rose");
    }
}
void Check_Red_Switch(String message) {
    Send_WhatsApp_Message("Power Monitor - Fatal Error Detected - " + message);
    while (1) {
        server.handleClient();                                             // handle any messages from the website
        digitalWrite(Green_led_pin, !digitalRead(Green_led_pin));
        digitalWrite(Blue_led_pin, !digitalRead(Blue_led_pin));
        Red_Switch.update();
        if (Red_Switch.fell()) {
            console_print("Red Button Pressed");
            ESP.restart();
        }
        delay(500);
    }
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
    int connection_attempts = 0;
    String temp;
    while (!getLocalTime(&timeinfo)) {                                      // get date and time from ntpserver
        console_print("Attempting to Get Date " + String(connection_attempts));
        delay(500);
        connection_attempts++;
        if (connection_attempts > 20) {
            console_print("Time Network Error, Restarting");
            Send_WhatsApp_Message("Power Monitor - Time Network Error, Restarting");
            ESP.restart();
        }
    }
    New_Date_Time_Data.field.Year = timeinfo.tm_year + 1900;
    New_Date_Time_Data.field.Month = timeinfo.tm_mon + 1;
    New_Date_Time_Data.field.Day = timeinfo.tm_mday;
    New_Date_Time_Data.field.Hour = timeinfo.tm_hour;
    New_Date_Time_Data.field.Minute = timeinfo.tm_min;
    New_Date_Time_Data.field.Second = timeinfo.tm_sec;
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
void Parse_Weather_info(String payload) {
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
}
void Parse(String payload, int start, int end) {
    int ptr = 0;
    for (int pos = start + 1; pos < end; pos++) {
        Parse_Output[ptr++] = payload[pos];
    }
    Parse_Output[ptr] = '\0';
}