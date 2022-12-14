/*
Name:		Power_Monitor.ino
Created : 9 / 10 / 2022 12 : 54 : 46 PM
Author : Stephen Gould
Version : 9.0   Compatible with Hardware Schematic V9
Change Record
24/10/2022  1.0 First Release
17/11/2022  9.0 Wipe SD Button added, Wipe SD Functionality added to Webpage
*/
// compilation switches -----------------------------------------------------------------------------------------------
//#define VERBOSE       // Remove the // at the start of this line to print out on the console the annotated value from the KWS-AC301L
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
const char* ssid = "Woodleigh";
const char* password = "2008198399";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;                             // offset for the date and time function
constexpr long console_Baudrate = 115200;
constexpr long RS485_Baudrate = 9600;                           // baud rate of RS485 Port
// ESP32 Pin Definitions ----------------------------------------------------------------------------------------------
constexpr int WipeSD_switch_pin = 32;
constexpr int Reset_switch_pin = 33;
constexpr int Start_switch_pin = 27;
constexpr int Running_led_pin = 26;
constexpr int Boot_switch_pin = 0;
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
Bounce Reset = Bounce();
Bounce Start = Bounce();
Bounce SD_switch = Bounce();
Bounce Boot = Bounce();
WebServer server(80);                   // WebServer(HTTP port, 80 is defAult)
WiFiClient client;
File RS485file;                         // Full data file, holds all readings from KWS-AC301L
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
String RS485FileName = "20220101";
String RS485_FieldNames[9] = {
                        "Volts",                    // [0]
                        "Amps",                     // [1]
                        "Watts",                    // [2]
                        "Up Time",                  // [3]
                        "kWh",                      // [4]
                        "Power Factor",             // [5]
                        "Unknown",                  // [6]
                        "Hz",                       // [7]
                        "degC",                     // [8]
};
String FileNames[50];
String RS485_Date;
String RS485_Time;
double RS485_Values[9] = {
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
bool started = false;                                   // used to indicate run switch has been pressed and readings are being taken
bool booted = false;
String version = "V1.0";                                // software version number, shown on webpage
String site_width = "1060";                             // width of web page
String site_height = "600";                             // height of web page
int const table_size = 72;
int       record_count, log_count;
String    webpage, lastcall;
String Last_Boot_Date = "11/11/2022 12:12:12";
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
record_type a_readings_table[table_size + 1];
double largest_amperage = 0;
String date_of_largest_amperage = "2022/01/01";
String time_of_largest_amperage = "00:00:00";
unsigned long last_sensor_read = 0;
uint64_t SD_freespace = 0;
uint64_t critical_SD_freespace = 0;
double SD_freespace_double = 0;
int i = 0;
// setup --------------------------------------------------------------------------------------------------------------
void setup() {
    console.begin(console_Baudrate);                                                    // enable the console
    while (!console);                                                                    // wait for port to settle
    delay(4000);
    console.print(millis(), DEC); console.println("\tCommencing Setup");                // print start message
    console.print(millis(), DEC);
    console.println("\t\t1. Initialising Switches and LEDs");                           // initialising Switches and LEDs
    pinMode(Start_switch_pin, INPUT_PULLUP);
    pinMode(Reset_switch_pin, INPUT_PULLUP);
    pinMode(WipeSD_switch_pin, INPUT_PULLUP);
    pinMode(Boot_switch_pin, INPUT_PULLUP);
    pinMode(Running_led_pin, OUTPUT);
    pinMode(SD_Active_led_pin, OUTPUT);
    Reset.attach(Reset_switch_pin);     // setup defaults for debouncing switches
    Start.attach(Start_switch_pin);
    SD_switch.attach(WipeSD_switch_pin);
    Boot.attach(Boot_switch_pin);
    Reset.interval(5);                  // sets debounce time
    Start.interval(5);
    SD_switch.interval(5);
    Reset.update();
    Start.update();
    SD_switch.update();
    digitalWrite(Running_led_pin, LOW);
    digitalWrite(SD_Active_led_pin, LOW);
    console.print(millis(), DEC);
    console.println("\t\t\tSwitches and LEDs Initialised");
    // WiFi and Web Setup -------------------------------------------------------------------------
    console.print(millis(), DEC); console.println("\t\t2. Starting WiFi");
    StartWiFi(ssid, password);                      // Start WiFi
    console.print(millis(), DEC); console.println("\t\t\tWiFi Started");
    console.print(millis(), DEC); console.println("\t\t3. Starting Time Server");
    StartTime();                                    // Start Time
    console.print(millis(), DEC); console.println("\t\t\tTime Started");
    console.print(millis(), DEC); console.println("\t\t4. Starting Web Server");
    server.begin();                                 // Start Webserver
    console.print(millis(), DEC); console.println("\t\t\tServer Started");
    console.print(millis(), DEC); console.print("\t\t\tWiFi IP Address: "); console.println(WiFi.localIP());
    server.on("/", Display);
    server.on("/Start", Start_Readings);
    server.on("/Display", Display);
    server.on("/Download", Download);                       // download the current file
    server.on("/Statistics", Statistics);                   // display statistics
    server.on("/DownloadFiles", Download_Files);            // select a file to download
    server.on("/GetFile", Download_File);                   // download the selectedfile
    server.on("/DeleteFiles", Delete_Files);                // select a file to delete
    server.on("/DelFile", Del_File);                        // delete the selected file
    server.on("/AverageFiles", Average_Files);            // display hourly analysis of current file
    server.on("/AverageFile", Average_File);              // analysis the selected file
    server.begin();
    console.print(millis(), DEC); console.println("\t\t\tWebserver started");
    // RS485 Setup --------------------------------------------------------------------------------
    console.print(millis(), DEC); console.println("\t\t5. Initialising RS485 Interface");
    RS485_Port.begin(RS485_Baudrate, SERIAL_8N1, RS485_RX_pin, RS485_TX_pin);
    pinMode(RS485_Enable_pin, OUTPUT);
    delay(10);
    console.print(millis(), DEC); console.println("\t\t\tRS485 Interface Initialised");
    // SD Setup -----------------------------------------------------------------------------------
    digitalWrite(SD_Active_led_pin, HIGH);
    console.print(millis(), DEC); console.println("\t\t6. SD Drive Configuration");
    console.print(millis(), DEC); console.print("\t\t\tSS pin:["); console.print(SS); console.println("]");
    console.print(millis(), DEC); console.print("\t\t\tMOSI pin:["); console.print(MOSI); console.println("]");
    console.print(millis(), DEC); console.print("\t\t\tMISO pin:["); console.print(MISO); console.println("]");
    console.print(millis(), DEC); console.print("\t\t\tSCK pin:["); console.print(SCK); console.println("]");
    if (!SD.begin(SS_pin)) {
        console.print(millis(), DEC); console.println("\t\tSD Drive Begin Failed @ line 212");
        while (true) {
            digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
            digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
            delay(500);
            Check_Reset_Switch();
        }
    }
    else {
        console.print(millis(), DEC); console.println("\t\t\tSD Drive Begin Succeeded");
        uint8_t cardType = SD.cardType();
        while (SD.cardType() == CARD_NONE) {
            console.print(millis(), DEC); console.println("\t\tNo SD card attached @ line 224");
            while (true) {
                digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                delay(500);
                Check_Reset_Switch();
            }
        }
        console.print(millis(), DEC); console.print("\t\t\tSD Card Type: ");
        if (cardType == CARD_MMC) {
            console.println("MMC");
        }
        else if (cardType == CARD_SD) {
            console.println("SDSC");
        }
        else if (cardType == CARD_SDHC) {
            console.println("SDHC");
        }
        else {
            console.println("UNKNOWN");
        }
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        console.print(millis(), DEC); console.print("\t\t\tSD Card Size: "); console.print(cardSize); console.println("MBytes");
        critical_SD_freespace = cardSize * (uint64_t).9;
        console.print(millis(), DEC); console.print("\t\t\tTotal Bytes: "); console.println(SD.totalBytes());
        console.print(millis(), DEC); console.print("\t\t\tUsed bytes: "); console.println(SD.usedBytes());
        console.print(millis(), DEC); console.println("\t\t\tSD Card Initialisation Complete");
        console.print(millis(), DEC); console.println("\t\t\tCreate Logging File");
    }
    Last_Boot_Date = GetDate(true) + GetTime(true);
    This_Date = GetDate(false);
    RS485FileName = GetDate(false) + ".csv";
    console.print(millis(), DEC); console.print("\t\t\tRS485 File Created: "); console.println(RS485FileName);
    if (!SD.exists("/" + RS485FileName)) {
        RS485file = SD.open("/" + RS485FileName, FILE_WRITE);
        if (!RS485file) {                                     // log file not opened
            console.print(millis(), DEC);
            console.print("\t\tError opening RS485file: @ line 278 ["); console.print(RS485FileName); console.println("]");
            while (true) {
                digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                delay(500);
                Check_Reset_Switch();
            }
        }
        RS485file.print("Date,");                           // write first column header
        RS485file.print("Time,");                           // write second column header
        for (int x = 0; x < 8; x++) {                       // write data column headings into the SD file
            RS485file.print(RS485_FieldNames[x]);
            RS485file.print(",");
        }
        RS485file.println(RS485_FieldNames[8]);
        RS485file.close();
        RS485file.flush();
        digitalWrite(SD_Active_led_pin, LOW);
        console.print(millis(), DEC); console.println("\t\t\tRS485 File Created");
    }
    else {
        console.print(millis(), DEC); console.println("\t\t\tRS485 File Already Exists");
        Prefill_Array();
    }
    digitalWrite(SD_Active_led_pin, LOW);
    console.print(millis(), DEC); console.println("\t\t7. End of Setup");
    console.print(millis(), DEC); console.println("\t\t8. Running in Full Function Mode");
    console.print(millis(), DEC); console.println("\t\t9. Press Start Button to commence readings");
}   // end of Setup
void loop() {
    Check_Reset_Switch();                                           // check if reset switch has been pressed
    Check_Start_Switch();                                           // check if start switch has been pressed
    Check_SD_Switch();                                              // check if wipesd switch has been pressed
    Drive_Running_Led();                                            // on when started, flashing when not, flashing with SD led if waiting for reset
    server.handleClient();                                          // handle any messages from the website
    if (started) {
        String now;
        if (millis() > last_sensor_read + (unsigned long)5000) {    // send requests every 5 seconds (5000 millisecods)
            last_sensor_read = millis();
            for (int i = 0; i < 9; i++) {                           // transmit the requests, assembling the Values array
                Send_Request(i);                                    // send the RS485 Port the requests, one by one
                RS485_Values[i] = Receive(i);                       // get the reply
            }                                                       // all values should now be populated
            RS485_Date = GetDate(true);                             // get the date of the reading
            RS485_Time = GetTime(true);                             // get the time of the reading
            // RS485 File Record Addition ----------------------------------------------------------------------------------------
            digitalWrite(SD_Active_led_pin, HIGH);                  // turn the SD activity LED on
            if (This_Date != GetDate(false)) {                      // has date changed
                RS485FileName = GetDate(false) + ".csv";            // yes, so create a new file
                console.print(millis(), DEC);
                console.print("\t\t\tRS485 File Created: ");
                console.println(RS485FileName);
                record_count = 0;
                log_count = 0;
                largest_amperage = 0;
                if (!SD.exists("/" + RS485FileName)) {
                    RS485file = SD.open("/" + RS485FileName, FILE_WRITE);
                    if (!RS485file) {                                     // log file not opened
                        console.print(millis(), DEC);
                        console.print("\t\tError opening RS485file: [");
                        console.print(RS485FileName); console.println("]");
                        while (true) {
                            digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                            digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                            delay(500);
                            Check_Reset_Switch();
                        }
                    }
                    RS485file.print("Date,");                           // write first column header
                    RS485file.print("Time,");                           // write second column header
                    for (int x = 0; x < 8; x++) {                       // write data column headings into the SD file
                        RS485file.print(RS485_FieldNames[x]);
                        RS485file.print(",");
                    }
                    RS485file.println(RS485FileName[8]);
                    RS485file.close();
                    RS485file.flush();
                    console.print(millis(), DEC); console.println("\t\t\tRS485 File Created");
                }
            }
            RS485file = SD.open("/" + RS485FileName, FILE_APPEND);      // open the SD file
            if (!RS485file) {                                                               // oops - file not available!
                console.print(millis(), DEC);
                console.print("\t\tError re-opening RS485file:");
                console.println(RS485FileName);
                while (true) {
                    digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                    digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                    delay(500);
                    Check_Reset_Switch();                                                   // Reset will restart the processor so no return
                }
            }
            SD_freespace = (SD.totalBytes() - SD.usedBytes());
            RS485file.print(RS485_Date); RS485file.print(",");          // dd/mm/yyyy,
            RS485file.print(RS485_Time); RS485file.print(",");          // dd/mm/yyyy,hh:mm:ss,
            SDprintDouble(RS485_Values[0], 1); RS485file.print(",");    // voltage
            SDprintDouble(RS485_Values[1], 3); RS485file.print(",");    // amperage
            SDprintDouble(RS485_Values[2], 2); RS485file.print(",");    // wattage
            SDprintDouble(RS485_Values[3], 1); RS485file.print(",");    // up time
            SDprintDouble(RS485_Values[4], 3); RS485file.print(",");    // kilowatt hour
            SDprintDouble(RS485_Values[5], 2); RS485file.print(",");    // power factor
            SDprintDouble(RS485_Values[6], 1); RS485file.print(",");    // unknown
            SDprintDouble(RS485_Values[7], 1); RS485file.print(",");    // frequency
            SDprintDouble(RS485_Values[8], 1);                          // temperature
            RS485file.print("\n");                                      // end of record
            RS485file.close();                                          // close the sd file
            RS485file.flush();                                          // make sure it has been written to SD
            digitalWrite(SD_Active_led_pin, LOW);
            if (RS485_Values[1] >= largest_amperage) {                  // load the maximum amperage value
                for (i = 0; i <= RS485_Date.length() + 1; i++) {                              // load the date
                    date_of_largest_amperage[i] = RS485_Date[i];
                }
                for (i = 0; i <= RS485_Time.length() + 1; i++) {                               // load the time
                    time_of_largest_amperage[i] = RS485_Time[i];
                }
                largest_amperage = RS485_Values[1];                     // update the largest current value
            }
            SD_freespace_double = (double)SD_freespace / 1000000;
            if (SD_freespace < critical_SD_freespace) {
                console.print(millis(), DEC);
                console.print("\tWARNING - SD Free Space critical ");
                console.print(SD_freespace); console.println("MB");
            }
            record_count++;                                             // increment the record count, the array pointer
            log_count++;
            //                                                          // add the record to the webpage display array
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
                for (i = 0; i <= RS485_Date.length() + 1; i++) {
                    readings_table[table_size].ldate[i] = RS485_Date[i];
                }                                                                           // write the new reading to the end of the table
                for (i = 0; i <= RS485_Time.length() + 1; i++) {
                    readings_table[table_size].ltime[i] = RS485_Time[i];
                }
                readings_table[table_size].voltage = RS485_Values[Voltage];
                readings_table[table_size].amperage = RS485_Values[Amperage];
                readings_table[table_size].wattage = RS485_Values[Wattage];                 // write the watts value into the table
                readings_table[table_size].uptime = RS485_Values[UpTime];
                readings_table[table_size].kilowatthour = RS485_Values[Kilowatthour];
                readings_table[table_size].powerfactor = RS485_Values[PowerFactor];
                readings_table[table_size].unknown = RS485_Values[Unknown];
                readings_table[table_size].frequency = RS485_Values[Frequency];
                readings_table[table_size].temperature = RS485_Values[Temperature];
            }
            else {
                for (i = 0; i <= RS485_Date.length() + 1; i++) {
                    readings_table[record_count].ldate[i] = RS485_Date[i];
                }                                                                         // write the new reading to the end of the table
                for (i = 0; i <= RS485_Time.length() + 1; i++) {
                    readings_table[record_count].ltime[i] = RS485_Time[i];
                }
                readings_table[record_count].voltage = RS485_Values[Voltage];
                readings_table[record_count].amperage = RS485_Values[Amperage];
                readings_table[record_count].wattage = RS485_Values[Wattage];
                readings_table[record_count].uptime = RS485_Values[UpTime];
                readings_table[record_count].kilowatthour = RS485_Values[Kilowatthour];
                readings_table[record_count].powerfactor = RS485_Values[PowerFactor];
                readings_table[record_count].unknown = RS485_Values[Unknown];
                readings_table[record_count].frequency = RS485_Values[Frequency];
                readings_table[record_count].temperature = RS485_Values[Temperature];
            }                                                                           // end of if record_count > table_size
            console.print(millis(), DEC); console.println("\t\t10. Record Written");
        }                                                                               // end of if millis >5000
    }                                                                                    // end of if started
}                                                                                       // end of loop
int StartWiFi(const char* ssid, const char* password) {
    console.print(millis(), DEC); console.print("\t\t\tConnecting to "); console.println(ssid);
    WiFi.begin(ssid, password);
    console.print(millis(), DEC); console.print("\t\t\tWiFi Status: "); console.println(WiFi.status());
    console.print(millis(), DEC); console.print("\t\t\t");
    int wifi_connection_attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        console.print(".");
        if (wifi_connection_attempts++ > 20) {
            console.print(millis(), DEC); console.println("\t\tNetwork Error, Restarting");
            ESP.restart();
        }
    }
    console.println("");
    console.print(millis(), DEC); console.println("\t\t\tConnected");
    return true;
}
void StartTime() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}
void Prefill_Array() {
    int character_count = 0;
    char csvField[3];
    int fieldNo = 1;
    char temp;
    digitalWrite(SD_Active_led_pin, HIGH);
    File csvFile = SD.open("/" + RS485FileName, FILE_READ);
    if (csvFile) {
        while (csvFile.available()) {                                           // throw the first row, column headers, away
            temp = csvFile.read();
            if (temp == '\n') break;
        }
        console.print(millis(), DEC); console.print("\t\t\tLoading datafile from "); console.println(RS485FileName);
        while (csvFile.available()) {                                           // do while there are data available
            temp = csvFile.read();
            csvField[character_count++] = temp;                                 // add it to the csvfield string
            if (temp == ',' || temp == '\n') {                                  // look for end of field
                csvField[character_count - 1] = '\0';                           // insert termination character where the ',' or '\n' was
                switch (fieldNo) {
                case 1:
                    strcpy(readings_table[record_count].ldate, csvField);       // Date
                    break;
                case 2:
                    strcpy(readings_table[record_count].ltime, csvField);       // Time
                    break;
                case 3:
                    readings_table[record_count].voltage = atof(csvField);      // Voltage
                    break;
                case 4:
                    readings_table[record_count].amperage = atof(csvField);     // Amperage
                    break;
                case 5:
                    readings_table[record_count].wattage = atof(csvField);      // Wattage
                    break;
                case 6:
                    readings_table[record_count].uptime = atof(csvField);       // Up Time
                    break;
                case 7:
                    readings_table[record_count].kilowatthour = atof(csvField); // KiloWatt Hour
                    break;
                case 8:
                    readings_table[record_count].powerfactor = atof(csvField);  // Power Factor
                    break;
                case 9:
                    readings_table[record_count].unknown = atof(csvField);      // Unknown
                    break;
                case 10:
                    readings_table[record_count].frequency = atof(csvField);    // Frequency
                    break;
                case 11:
                    readings_table[record_count].temperature = atof(csvField);  // Temperature
                }
                fieldNo++;
                csvField[0] = '\0';
                character_count = 0;
            }
            if (temp == '\n') {                                                                 // end of sd data row
#ifdef VERBOSE
                console.print("\t\t("); console.print(log_count + 1, DEC); console.print(")");
                console.print("."); console.print(readings_table[record_count].ldate);
                console.print(","); console.print(readings_table[record_count].ltime);
                console.print(","); printDouble(readings_table[record_count].voltage, 1);
                console.print(","); printDouble(readings_table[record_count].amperage, 3);
                console.print(","); printDouble(readings_table[record_count].wattage, 1);
                console.print(","); printDouble(readings_table[record_count].uptime, 1);
                console.print(","); printDouble(readings_table[record_count].kilowatthour, 3);
                console.print(","); printDouble(readings_table[record_count].powerfactor, 2);
                console.print(","); printDouble(readings_table[record_count].unknown, 1);
                console.print(","); printDouble(readings_table[record_count].frequency, 1);
                console.print(","); printDouble(readings_table[record_count].temperature, 1);
                console.println();
#endif
                record_count++;                                                                 // increment array pointer
                log_count++;
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
                fieldNo = 1;
            }
        } // end of while
        console.print(millis(), DEC); console.print("\t\t\tRecords Loaded: "); console.println(log_count++);
    }
    csvFile.close();
    digitalWrite(SD_Active_led_pin, LOW);
}
void Display() {
    console.print(millis(), DEC); console.println("\t\tStart of Display Channels");
    String log_time;
    log_time = "Time";
    webpage = "";                           // don't delete this command, it ensures the server works reliably!
    Page_Header(true, "Energy Useage Monitor");
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
    webpage += F("title:'Electrical Power Consumption',titleTextStyle:{fontName:'Arial', fontSize:20, color: 'blue'},");
    webpage += F("legend:{position:'bottom'},colors:['red'],backgroundColor:'#F3F3F3',chartArea: {width:'85%', height:'65%'},");
    webpage += F("hAxis:{slantedText:true,slantedTextAngle:90,titleTextStyle:{width:'100%',color:'Purple',bold:true,fontSize:16},");
    webpage += F("gridlines:{color:'#333'},showTextEvery:1");
    webpage += F(",title:'Time'");
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
    console.print(millis(), DEC); console.println("\t\tEnd of Display Wattage");
}
void Start_Readings() {
    started = true;
    webpage = "";
    Display();
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
    webpage += F("<span style='color:red; font-size:36pt;'>");                              // start of span
    webpage += Header;
    // </span> end ----------------------------------------------------------------------------------------------------
    webpage += F("</span>");                                                                // end of span
    webpage += F("<span style='font-size: medium;'><align left=''></align></span>");
    // <h1> end -------------------------------------------------------------------------------------------------------
    webpage += F("</h1>");                                                                  // end of h1
    // </style> -------------------------------------------------------------------------------------------------------
    webpage += F("<style>ul{list-style-type:none;margin:0;padding:0;overflow:hidden;background-color:#31c1f9;font-size:14px;}");
    webpage += F("li{float:left;}");
    webpage += F("li a{display:block;text-align:center;padding:5px 25px;text-decoration:none;}");
    webpage += F("li a:hover{background-color:#FFFFFF;}");
    webpage += F("h1{background-color:#31c1f9;}");
    webpage += F("body{width:");
    webpage += site_width;
    webpage += F("px;margin:0 auto;font-family:arial;font-size:14px;text-align:center;color:#ed6495;background-color:#F7F2Fd;}");
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
    if (!started) {
        console.print(millis(), DEC); console.println("\t\tLogger not started");
        webpage += F("<li>");
        webpage += F("<span ");
        webpage += F("style='color:#e03e2d;'>");
        webpage += F("<a ");
        webpage += F("style='color:#e03e2d;");
        webpage += F("'href='/Start' >Start Readings");
        webpage += F("</a>");
        webpage += F("</span>");
        webpage += F("</li>");
    }
    else {
        console.print(millis(), DEC); console.println("\t\tLogger started");
        webpage += F("<li>");
        webpage += F("<span ");
        webpage += F("style='color:#e03e2d;'>");
        webpage += F("<a ");
        webpage += F("style='color:#e03e2d;");
        webpage += F("'href='/Start'>");
        webpage += F("<span ");
        webpage += F("style='color:#000000;'>Start Readings");
        webpage += F("</span>");
        webpage += F("</a>");
        webpage += F("</span>");
        webpage += F("</li>");
    }
    webpage += F("<li><a href='/Display'>Webpage</a> </li>");
    webpage += F("<li><a href='/Statistics'>Display Statistics</a></li>");
    webpage += F("<li><a href='/Download'>Download Current File</a></li>");
    webpage += F("<li><a href='/DownloadFiles'>Download Files</a></li>");
    webpage += F("<li><a href='/DeleteFiles'>Delete Files</a></li>");
    webpage += F("<li><a href='/AverageFiles'>Average Files</a></li>");
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
    webpage += F(" Last Page Update - ");
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
void Download() {                                               // download, to PC accessing webpage, the current datafile
    console.print(millis(), DEC);
    console.println("\t\tStart of Log Download");
    if (started) {
        File datafile = SD.open("/" + RS485FileName, FILE_READ);    // Now read data from FS
        if (datafile) {                                             // if there is a file
            if (datafile.available()) {                             // If data is available and present
                String contentType = "application/octet-stream";
                server.sendHeader("Content-Disposition", "attachment; filename=" + RS485FileName);
                if (server.streamFile(datafile, contentType) != datafile.size()) {
                    console.print(millis(), DEC);
                    console.println("\t\tSent less data than expected");
                }
            }
        }
        datafile.close(); // close the file:
    }
    else {
        console.print(millis(), DEC); console.println("\t\tLogging not started, no current file");
    }
    console.print(millis(), DEC); console.println("\t\tEnd of Log View");
    webpage = "";
    Display();
}
void Statistics() {  // Display file size of the datalog file
    int file_count = Count_Files_on_SD_Drive();
    console.print(millis(), DEC); console.println("\t\tStart of Log Stats");
    if (!started) {
        RS485file = SD.open("/" + RS485FileName, FILE_APPEND);      // open the SD file
        if (!RS485file) {                                                               // oops - file not available!
            console.print(millis(), DEC);
            console.print("\t\tError re-opening RS485file:");
            console.println(RS485FileName);
            while (true) {
                digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                delay(500);
                Check_Reset_Switch();                                                   // Reset will restart the processor so no return
            }
        }
        SD_freespace = (SD.totalBytes() - SD.usedBytes());
    }
    webpage = ""; // don't delete this command, it ensures the server works reliably!
    Page_Header(false, "Energy Monitor Statistics");
    File datafile = SD.open("/" + RS485FileName, FILE_READ);  // Now read data from FS
    // <p style="text-align: left; color: blue,bold:true; font-size: 24px;">Current Data Log file size = 100 Bytes</p>
    // <p style = "text-align: center;"><strong><span style = "color: red;">Freespace on bottom row< / span>< / strong>< / p>
    webpage += F("<p ");
    webpage += F("style='text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:24px;'");
    webpage += F(">Current Data Log file size = ");
    webpage += String(datafile.size());
    webpage += F(" Bytes");
    webpage += "</span></strong></p>";
    if (SD_freespace < critical_SD_freespace) {
        console.print(millis(), DEC); console.println("\t\tFreespace less than critical");
        webpage += F("<p ");
        webpage += F("style='text-align:left;'><strong><span style='color:red;bold:true;font-size:24px'");
        webpage += F("'>SD Free Space = ");
        webpage += String(SD_freespace / 1000000);
        webpage += F(" MB");
        webpage += "</span></strong></p>";
    }
    else {
        console.print(millis(), DEC); console.println("\t\tFreespace => critical");
        webpage += F("<p ");
        webpage += F("style='text-align:left;'><strong><span style='color:DodgerBlue;bold:true;font-size:24px'");
        webpage += F("'>SD Free Space = ");
        webpage += String(SD_freespace / 1000000);
        webpage += F(" MB");
        webpage += "</span></strong></p>";
    }
    webpage += F("<p ");
    webpage += F("style='text-align:left;'><strong><span style='color:DodgerBlue;font-size:24px'");
    webpage += F("'>Last Boot Time and Date: ");
    webpage += Last_Boot_Date;
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    webpage += F("style='text-align:left;'><strong><span style='color:DodgerBlue;font-size:24px'");
    webpage += F("'>Number of readings = ");
    webpage += String(log_count);
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    webpage += F("style='text-align:left;'><strong><span style='color:DodgerBlue;font-size:24px'");
    webpage += F("'>Greatest Amperage on ");
    webpage += String(date_of_largest_amperage) + " at ";
    webpage += String(time_of_largest_amperage) + " = ";
    webpage += String(largest_amperage) + "ma";
    webpage += "</span></strong></p>";
    webpage += F("<p ");
    webpage += F("style='text-align:left;'><strong><span style='color:DodgerBlue;font-size:24px'");
    webpage += F("'> Number of Files on SD : ");
    webpage += String(file_count - 1);
    webpage += "</span></strong></p>";
    datafile.close();
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
    console.print(millis(), DEC); console.println("\t\tEnd of Log Stats");
}
void Download_Files() {
    int file_count = Count_Files_on_SD_Drive();                                 // this counts and creates an array of file names on SD
    console.print(millis(), DEC); console.println("\t\tStart of List Files");
    webpage = ""; // don't delete this command, it ensures the server works reliably!
    Page_Header(false, "Energy Monitor Download Files");
    for (i = 1; i < file_count; i++) {
        webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(i) + " " + String(FileNames[i]) + " ";
        webpage += "&nbsp;<a href=\"/GetFile?file=" + String(FileNames[i]) + " " + "\">Download</a>";
        webpage += "</h3>";
    }
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
    console.print(millis(), DEC); console.println("\t\tEnd of List Files");
}
void Download_File() {                                                          // download the selected file
    console.print(millis(), DEC);
    console.println("\t\tGet File Requested");
    String fileName = server.arg("file");
    console.print(millis(), DEC);
    console.print("\t\tFile Name = ");
    console.println(fileName);
    File datafile = SD.open("/" + fileName, FILE_READ);    // Now read data from FS
    if (datafile) {                                             // if there is a file
        if (datafile.available()) {                             // If data is available and present
            String contentType = "application/octet-stream";
            server.sendHeader("Content-Disposition", "attachment; filename=" + fileName);
            if (server.streamFile(datafile, contentType) != datafile.size()) {
                console.print(millis(), DEC);
                console.println("\t\tSent less data than expected");
            }
        }
    }
    datafile.close(); // close the file:
    webpage = "";
    console.print(millis(), DEC); console.println("\t\tEnd of Get_File");
}
void Delete_Files() {                                                           // allow the cliet to select a file for deletion
    int file_count = Count_Files_on_SD_Drive();                                 // this counts and creates an array of file names on SD
    console.print(millis(), DEC); console.println("\t\tStart of List Files");
    webpage = ""; // don't delete this command, it ensures the server works reliably!
    Page_Header(false, "Energy Monitor Delete Files");
    for (i = 1; i < file_count; i++) {
        if (FileNames[i] != RS485FileName) {                            // do not list the current file
            webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(i) + " " + String(FileNames[i]) + " ";
            webpage += "&nbsp;<a href=\"/DelFile?file=" + String(FileNames[i]) + " " + "\">Delete</a>";
            webpage += "</h3>";
        }
    }
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
    console.print(millis(), DEC); console.println("\t\tEnd of List Files");
}
void Del_File() {                                                       // web request to delete a file
    console.print(millis(), DEC);
    console.println("\t\tDelete File Requested");
    String fileName = "\20221111.csv";                                  // dummy load to get the string space reserved
    fileName = "/" + server.arg("file");
    if (fileName != ("/" + RS485FileName)) {                            // do not delete the current file
        SD.remove(fileName);
    }
    int file_count = Count_Files_on_SD_Drive();                         // this counts and creates an array of file names on SD
    webpage = "";                                                       // don't delete this command, it ensures the server works reliably!
    Page_Header(false, "Energy Monitor Delete Files");
    for (i = 1; i < file_count; i++) {
        webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(i) + " " + String(FileNames[i]) + " ";
        webpage += "&nbsp;<a href=\"/DelFile?file=" + String(FileNames[i]) + " " + "\">Delete</a>";
        webpage += "</h3>";
    }
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
    console.print(millis(), DEC); console.println("\t\tEnd of Delete_File");
}
void Average_Files() {
    int file_count = Count_Files_on_SD_Drive();                                 // this counts and creates an array of file names on SD
    console.print(millis(), DEC); console.println("\t\tStart of Average Files");
    webpage = ""; // don't delete this command, it ensures the server works reliably!
    Page_Header(false, "Energy Monitor Plot Average Files");
    for (i = 1; i < file_count; i++) {
        webpage += "<h3 style=\"text-align:left;color:DodgerBlue;font-size:18px\";>" + String(i) + " " + String(FileNames[i]) + " ";
        webpage += "&nbsp;<a href=\"/AverageFile?file=" + String(FileNames[i]) + " " + "\">Analyse</a>";
        webpage += "</h3>";
    }
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
    console.print(millis(), DEC); console.println("\t\tEnd of List Files");
}
void Average_File() {
    console.print(millis(), DEC); console.println("\t\t10. Start of Analyse File");
    String log_time;
    log_time = "Time";
    char temp;
    int average_table_record_count = 0;
    int records = 0;
    char csvField[10];
    int character_count = 0;
    int fieldNo = 1;
    double total_amperage = 0;
    double average_amperage = 0;
    char time[8];
    int file_record_count = 0;
    int a_logging_count = 0;
    digitalWrite(SD_Active_led_pin, HIGH);
    //  1. find the number of records in the file ---------------------------------------------------------------------
    String fileName = "        ";
    fileName = server.arg("file");
    record_count = 0;                                                 // miss the first record in the file - column titles
    File csvFile = SD.open("/" + fileName, FILE_READ);
    if (csvFile) {                                                    // if file successfully opened 
        console.print(millis(), DEC);
        console.print("\t\tLoading data from ");
        console.println(fileName);
        while (csvFile.available()) {                                 // throw the first row, column headers, away
            temp = csvFile.read();
            if (temp == '\n') break;
        }
        console.print(millis(), DEC);
        console.println("\t\tFirst Record Ignored");
        while (csvFile.available()) {                                 // count the lines in the file
            temp = csvFile.read();
            if (temp == '\n') {                                       // by counting the end of lines
                record_count++;                                       // increment the count
            }
        }
    }
    csvFile.close();
    console.print(millis(), DEC);
    console.print("\t\tNumber of Records in File :");
    console.println(record_count);
    file_record_count = record_count;
    //  2. calculate the number of rows per average -------------------------------------------------------------------
    if (record_count < table_size) {
        console.print(millis(), DEC);
        console.println("\t\tInsufficient Records");
    }
    records = record_count / table_size;
    record_count = 0;
    console.print(millis(), DEC);
    console.print("\t\tNumber of Records per batch:");
    console.println(records);
    // 3. Read the specified number of records and calculate average amperage -----------------------------------------
    csvFile = SD.open("/" + fileName, FILE_READ);               // open the file again
    if (csvFile) {
        console.print(millis(), DEC);
        console.print("\t\tOpening File Again:");
        console.println(fileName);
        while (csvFile.available()) {                           // throw the first row, column headers, away
            temp = csvFile.read();
            if (temp == '\n') break;
        }
        console.print(millis(), DEC);
        console.print("\t\tLoading datafile from ");
        console.println(fileName);
        while (csvFile.available()) {                           // do while there are data available
            temp = csvFile.read();                              // read next character into temp
            csvField[character_count++] = temp;                 // add it to the csvfield string
            if (temp == ',' || temp == '\n') {                  // look for end of field
                csvField[character_count - 1] = '\0';           // insert termination character where the ',' or '\n' was
                switch (fieldNo) {                              // process the field dependent on which fieldNo we are processing
                case 1:                                         // field 1 is Date
                    break;
                case 2:                                         // field 2 is Time
                    for (int x = 0; x < 8; x++) {
                        time[x] = csvField[x];
                    }
                    break;
                case 3:                                         // field 3 ignore
                    break;
                case 4:                                         // field 4 is Amperage 
                    total_amperage += atof(csvField);           // accumulate to the total
                    break;
                case 5:                                         // field 5 ignore
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                    break;
                default:
                    break;
                }
                fieldNo++;                                      // increment the field number
                csvField[0] = '\0';                             // clear the csvField
                character_count = 0;                            // zero the character cout
            }                                                   // finished processing this field
            if (temp == '\n') {
                record_count++;                                                   // increment records read count
                a_logging_count++;
                if (record_count == records) {                                    // create an average table record
                    average_amperage = total_amperage / (double)records;
                    for (int x = 0; x < 8; x++) {
                        a_readings_table[average_table_record_count].ltime[x] = time[x];
                    }
                    a_readings_table[average_table_record_count].amperage = average_amperage;
                    average_table_record_count++;                                   // increment the average table record count
                    record_count = 0;
                }
                fieldNo = 1;                                                        // reset the field pointer
            }
        }   // end of while
    } // end of file.available()
    console.print(millis(), DEC);
    console.print("\t\tTotal Record : ");
    console.println(a_logging_count, DEC);
    console.print(millis(), DEC);
    console.print("\t\t File Count : ");
    console.println(file_record_count, DEC);
    csvFile.close();
    digitalWrite(SD_Active_led_pin, LOW);
    //4. Generate web page
    webpage = "";                           // don't delete this command, it ensures the server works reliably!
    Page_Header(false, "Energy Analysis of " + fileName);
    // <script> -------------------------------------------------------------------------------------------------------
    webpage += F("<script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>");
    webpage += F("<script type=\"text/javascript\">");
    webpage += F("google.charts.load('current',{packages:['corechart','line']});");
    webpage += F("google.setOnLoadCallback(drawChart);");
    webpage += F("google.charts.load('current',{'packages':['bar']});");
    webpage += F("google.charts.setOnLoadCallback(drawChart);");
    webpage += F("function drawChart() {");
    webpage += F("var data=new google.visualization.DataTable();");
    webpage += F("data.addColumn('timeofday','Time');");
    webpage += F("data.addColumn('number','Amperage');");
    webpage += F("data.addRows([");
    for (int i = 0; i < (average_table_record_count); i++) {
        if (String(a_readings_table[i].ltime) != "") {                  // if the ltime field contains data
            for (int y = 0; y < 8; y++) {                               // replace the ":"s in ltime with ","
                if (a_readings_table[i].ltime[y] == ':') {
                    a_readings_table[i].ltime[y] = ',';
                }
            }
            webpage += "[[";
            webpage += String(a_readings_table[i].ltime) + "],";
            webpage += String(a_readings_table[i].amperage) + "]";
            if (i != average_table_record_count) webpage += ",";    // do not add a "," to the last record
        }
    }
    webpage += "]);\n";
    webpage += F("var options={");
    webpage += F("title:'Electrical Power Consumption',titleTextStyle:{fontName:'Arial',fontSize:20,color:'blue'},");
    webpage += F("legend:{position:'bottom'},colors:['red'],backgroundColor:'#F3F3F3',chartArea: {width:'85%', height:'65%'},");
    webpage += F("hAxis:{slantedText:true,slantedTextAngle:90,titleTextStyle:{width:'100%',color:'Purple',bold:true,fontSize:16},");
    webpage += F("gridlines:{color:'#333'},showTextEvery:1");
    webpage += F(",title:'Time'");
    webpage += F("},");
    webpage += F("vAxes:");
    webpage += F("{0:{viewWindowMode:'explicit',gridlines:{color:'black'},viewWindow:{min:0,max:100},scaleType:'lin',title:'Amperage (A)',format:'###'},");
    webpage += F("}, ");
    webpage += F("series:{0:{targetAxisIndex:0},curveType:'none'},};");
    webpage += F("var chart=new google.visualization.LineChart(document.getElementById('line_chart'));chart.draw(data,options);");
    webpage += F("}");
    webpage += F("</script>");
    // </script> ------------------------------------------------------------------------------------------------------
    webpage += F("<div id='line_chart'style='width:960px;height:600px'></div>");
    Page_Footer();
    server.send(200, "text/html", webpage);
    webpage = "";
    lastcall = "display";
    for (int x = 0; x < average_table_record_count; x++) {
        for (int y = 0; y < 8; y++) {
            a_readings_table[x].ltime[y] = 0;
        }
        a_readings_table[x].amperage = 0;
    }
    console.print(millis(), DEC); console.println("\t\tEnd of Display Average");
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
                console.print(millis(), DEC);
                console.print("\t\tFile "); console.print(file_count, DEC);
                console.print(" filename "); console.print(filename);
                console.print(" FileNames "); console.println(FileNames[file_count]);
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
void Wipe_Files() {                                                 // selected by pressing combonation of buttons
    String filename;
    File root = SD.open("/");                                       //  Open the root directory
    while (true) {
        File entry = root.openNextFile();                           //  get the next file
        if (entry) {
            filename = entry.name();
            console.print(millis(), DEC); console.print("\t\tRemoving "); console.println(filename);
            SD.remove(entry.name());                                //  delete the file
        }
        else {
            root.close();
            console.print(millis(), DEC); console.println("\t\tAll files removed from root directory, rebooting");
            ESP.restart();
        }
    }
}
void Send_Request(int field) {
    //    console.print(millis(), DEC); console.print("\tSend Request:");
    digitalWrite(RS485_Enable_pin, transmit);                                               // set RS485_Enable HIGH to transmit values to RS485
    for (int x = 0; x < 7; x++) {
        RS485_Port.write(RS485_Requests[field][x]);                                     // write the Request string to the RS485 Port
        //        console.print("("); console.print(RS485_Requests[field][x], DEC); console.print("),");
        delay(2);
    }
    RS485_Port.write(RS485_Requests[field][7]);
    //    console.print("("); console.print(RS485_Requests[field][7], DEC); console.println(").");
    delay(2);
}
double Receive(int field) {
    byte pointer = 0;
    byte required_bytes = 0;
    int received_character = 0;
    bool value_status = false;
    unsigned long start_time;                                                   // take the start time, used to check for time out
    long value[7] = { 0,0,0,0,0,0,0 };
    //    console.print(millis(), DEC); console.println("\t\tIn Receive Mode");
    digitalWrite(RS485_Enable_pin, receive);                                    // set Enable pin low to receive values from RS485
    start_time = millis();                                                      // take the start time, used to check for time out
    do {
        while (!RS485_Port.available()) {                                       // wait for some data to arrive
            if (millis() > start_time + (unsigned long)500) {                   // no data received within 500 ms so timeout
                console.print(millis(), DEC);
                console.print("\tNo Reply from RS485 within 500 ms");
                while (true) {                                                          // wait for reset to be pressed
                    digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
                    digitalWrite(SD_Active_led_pin, !digitalRead(SD_Active_led_pin));
                    delay(500);
                    Check_Reset_Switch();                                               // Reset will restart the processor so no return
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
            volts = volts / (double)10;                                                                 // convert to vv.v
#ifdef VERBOSE
            console.print(millis(), DEC); console.print("\t\tVoltage = [");
            printDouble(volts, 2); console.println("]");                                              // print to monitor as nnnn.n
#endif            
            return volts;                                                                               // Voltage output format double
            break;
        }
        case Amperage: {                                                                                // Amperage
            double amperage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);       // Received is number of milli amps 
            amperage = amperage / (double)1000;                                                         // convert to amps 0.00n
#ifdef VERBOSE
            console.print(millis(), DEC); console.print("\t\tAmperage = [");
            printDouble(amperage, 4); console.println("]");
#endif
            return amperage;                                                                            // Amperage output format double nn.n
            break;
        }
        case Wattage: {                                                                                 // Wattage
            double wattage = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);        // Recieved is number of tenths of watts
            wattage = wattage / (double)10;                                                             // convert to watts
#ifdef VERBOSE
            console.print(millis(), DEC); console.print("\t\tWattage = [");
            printDouble(wattage, 2); console.println("]");
#endif
            return wattage;
            break;
        }
        case UpTime: {
            double uptime = (value[3] << 8) + value[4];
            uptime = uptime / (double)60;                                                              // convert to hours
#ifdef VERBOSE
            console.print(millis(), DEC); console.print("\t\tUp Time = [");
            printDouble(uptime, 3); console.println("]");
#endif
            return uptime;
            break;
        }
        case Kilowatthour: {
            double kilowatthour = (value[5] << 24) + (value[6] << 16) + ((value[3] << 8) + value[4]);
            kilowatthour = kilowatthour / (double)1000;
#ifdef VERBOSE
            console.print(millis(), DEC); console.print("\t\tKiloWatts = [");
            printDouble(kilowatthour, 4); console.println("]");
#endif
            return kilowatthour;
            break;
        }
        case PowerFactor: {
            double powerfactor = (value[3] << 8) + value[4];
            powerfactor = powerfactor / (double)100;
#ifdef VERBOSE
            console.print(millis(), DEC); console.print("\t\tPower Factor = [");
            printDouble(powerfactor, 3); console.println("]");
#endif
            return powerfactor;
            break;
        }
        case Unknown: {
            double unknown = (value[3] << 8) + value[4];
#ifdef VERBOSE
            console.print(millis(), DEC); console.print("\t\tUnknown = [");
            printDouble(unknown, 0); console.println("]");
#endif
            return unknown;
            break;
        }
        case Frequency: {
            double frequency = (value[3] * 256) + value[4];
            frequency = frequency / (double)10;
#ifdef VERBOSE
            console.print(millis(), DEC); console.print("\t\tFrequency = [");
            printDouble(frequency, 2); console.println("]");
#endif
            return frequency;
            break;
        }
        case Temperature: {
            double temperature = (value[3] << 8) + value[4];
#ifdef VERBOSE
            console.print(millis(), DEC); console.print("\t\tTemperature = [");
            printDouble(temperature, 2); console.println("]");
#endif
            return temperature;
            break;
        }
        default: {
        }
        }
    }
    return (double)0;
}
void Check_Start_Switch() {
    Start.update();
    if (Start.fell()) {
        console.print(millis(), DEC); console.println("\t\tStart Button Pressed");
        if (started) {
            if (RS485file) {
                RS485file.close();
                RS485file.flush();
            }
            started = false;
            console.print(millis(), DEC); console.println("\t\tStandby Mode Started");
        }
        else {
            started = true;
            console.print(millis(), DEC); console.println("\t\tRunning Mode Started");
        }
    }
}
void Check_SD_Switch() {
    char Running_led_state = 0;
    char SD_led_state = 0;
    SD_switch.update();                                                                // update wipe switch
    if (SD_switch.fell()) {
        console.print(millis(), DEC);
        console.println("\t\tSave/Wipe SD Button Pressed");
        console.print(millis(), DEC);
        console.println("\t\tPress Start Button to commence Save/Wipe Operation");
        Running_led_state = digitalRead(Running_led_pin);                           // save the current state of the leds
        SD_led_state = digitalRead(SD_Active_led_pin);
        if (!started) {                                                             // do not allow Wipe if logging started
            do {
                digitalWrite(Running_led_pin, HIGH);                                // turn the run led on
                digitalWrite(SD_Active_led_pin, LOW);                               // turn the sd led off
                Boot.update();
                if (Boot.fell()) booted = true;                                     // WIPE + Boot = wipe directory
                Reset.update();                                                     // reset will cancel the Wipe
                if (Reset.fell()) ESP.restart();;
                delay(150);
                digitalWrite(Running_led_pin, LOW);                                 // turn the run led off
                digitalWrite(SD_Active_led_pin, HIGH);                              // turn the sd led on
                delay(150);
            } while (!started);
            if (booted) {
                digitalWrite(SD_Active_led_pin, HIGH);
                Wipe_Files();                                                       // delete all files on the SD, Rebooted when compete
            }
            digitalWrite(Running_led_pin, Running_led_state);                       // restore the previous state of the leds
            digitalWrite(SD_Active_led_pin, SD_led_state);
        }
        else {
            console.print(millis(), DEC); console.println("\t\tWipe ONLY when not started");
        }
    }
}
void Drive_Running_Led() {
    if (started) {
        digitalWrite(Running_led_pin, HIGH);
    }
    else {
        digitalWrite(Running_led_pin, !digitalRead(Running_led_pin));
        delay(500);
    }
}
void Check_Reset_Switch() {
    Reset.update();
    if (Reset.fell()) {
        console.print(millis(), DEC); console.println("\t\tReset Depressed");
        ESP.restart();
    }
}
void Check_Boot_Switch() {
    Boot.update();
    if (Boot.fell()) {
        console.print(millis(), DEC); console.println("\t\tBoot Depressed");
        booted = true;
    }
    if (Boot.rose()) {
        console.print(millis(), DEC); console.println("\t\tBoot Released");
        booted = false;
    }
    return;
}
String GetDate(bool format) {
    int time_connection_attempts = 0;
    while (!getLocalTime(&timeinfo)) {
        console.print(millis(), DEC); console.print("\t\tFailed to obtain time ");
        delay(500);
        console.print(".");
        if (time_connection_attempts > 20) {
            console.print(millis(), DEC); console.print("\t\tTime Network Error, Restarting");
            ESP.restart();
        }
    }
    console.print("");
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
    int time_connection_attempts = 0;
    while (!getLocalTime(&timeinfo)) {
        console.print(millis(), DEC); console.println("\t\tFailed to obtain time");
        delay(500);
        console.print(".");
        if (time_connection_attempts > 20) {
            console.print(millis(), DEC); console.print("\t\tTime Network Error, Restarting");
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

    console.print(int(val));  //prints the int part
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
        console.print(frac, DEC);
    }
}
void SDprintDouble(double val, byte precision) {
    RS485file.print(int(val));  //prints the int part
    if (precision > 0) {
        RS485file.print("."); // print the decimal point
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
            RS485file.print("0");
        RS485file.print(frac, DEC);
    }
}