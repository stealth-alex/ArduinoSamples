// CAN-BUS shiled SD card writing with software SPI interface

#include <SPI.h>
#include "SdFat.h"

// https://github.com/greiman/SdFat.git
#if SD_SPI_CONFIGURATION >= 3  // Must be set in SdFat/SdFatConfig.h

#define MEGA

// Pin numbers in templates must be constants.
#ifndef MEGA
    // UNO
    const uint8_t SOFT_MISO_PIN = 12;
    const uint8_t SOFT_MOSI_PIN = 11;
    const uint8_t SOFT_SCK_PIN  = 13;
    const uint8_t SD_CHIP_SELECT_PIN = 4;
#else
    // MEGA
    const uint8_t SOFT_MISO_PIN = 50; 
    const uint8_t SOFT_MOSI_PIN = 51; 
    const uint8_t SOFT_SCK_PIN  = 52;  
    const uint8_t SD_CHIP_SELECT_PIN = 53;
#endif

// SdFat software SPI template
SdFatSoftSpi<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> sd;

// Test file.
SdFile file;

void setup() 
{
    Serial.begin(9600);
    // Wait for USB Serial 
    while (!Serial) 
    {
        SysCall::yield();
    }
  
    Serial.println("Type any character to start");

    while (!Serial.available()) 
    {
        SysCall::yield();
    }

    pinMode(SD_CHIP_SELECT_PIN, OUTPUT);
    
    if (!sd.begin(SD_CHIP_SELECT_PIN)) 
    {
        sd.initErrorHalt();
    }

    if (!file.open("SoftSPI.txt", O_CREAT | O_RDWR)) 
    {
        sd.errorHalt(F("open failed"));
    }
    
    file.println(F("This line was printed using software SPI."));

    file.rewind();
  
    while (file.available()) 
    {
        Serial.write(file.read());
    }

    file.close();

    Serial.println(F("Done."));
}

//------------------------------------------------------------------------------
void loop() 
{
}

#else  // SD_SPI_CONFIGURATION >= 3
#error SD_SPI_CONFIGURATION must be set to 3 in SdFat/SdFatConfig.h
#endif  //SD_SPI_CONFIGURATION >= 3

