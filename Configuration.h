/*
Name:		Power_Monitor.ino
Created : 2 / 1 / 2023
Author : Stephen Gould

Installation Specific Information:
Intended that this information is loaded from a configuration file, because this information will be specific
    to the user's location and network information
*/
#pragma once
// Installation Specific Information ----------------------------------------------------------------------------------
const char* ssid = "Woodleigh";                             // network ssid
const char* password = "2008198399";                        // network password
const char city[] = { "Basingstoke\0" };
const char region[] = { "uk\0" };
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;                             // offset for the date and time function
// --------------------------------------------------------------------------------------------------------------------
