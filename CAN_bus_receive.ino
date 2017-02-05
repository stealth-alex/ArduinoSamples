// CAN-BUS shiled SD card writing with software SPI interface

#include <SPI.h>
#include "SdFat.h"
#include "mcp_can.h"

// https://github.com/greiman/SdFat.git

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

// CAN bus
#define CAN0_INT 2    // Set INT to pin 2
MCP_CAN CAN0(10);     // Set CS to pin 10

struct data_t 
{
    uint32_t time;
    long unsigned int rxId;
    unsigned char len;
    unsigned char rxBuf[8];
};

data_t data;
static uint32_t startMicros;

char msgString[128];             // Array to store serial string

// SdFat software SPI template
// SdFatSoftSpi<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> sd;
SdFat sd;

// Test file.
SdFile file;

bool toggle     = false;
bool canActive  = false;
bool sdActive   = false;

//------------------------------------------------------------------------------
bool CAN_init()
{
    bool result = true;
    
    // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
    if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK)
    {
        Serial.println("MCP2515 Initialized Successfully!");
    }
    else
    {
        Serial.println("Error Initializing MCP2515...");
        result = false;
    }
  
    CAN0.setMode(MCP_NORMAL);       // Set operation mode to normal so the MCP2515 sends acks to received data.

    pinMode(CAN0_INT, INPUT);       // Configuring pin for /INT input

    return result;
}

//------------------------------------------------------------------------------
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

    canActive = CAN_init();

    pinMode(10, OUTPUT);
    pinMode(SD_CHIP_SELECT_PIN, OUTPUT);

    if (!sd.begin(SD_CHIP_SELECT_PIN, SPI_HALF_SPEED)) 
    {
        sd.initErrorHalt();
    }

    if (!file.open("can.log", O_CREAT | O_RDWR)) 
    {
        sd.errorHalt(F("Log file create failed"));
    }
    else
    {
        Serial.println(F("Log file create success"));
        file.println(F("CAN log:"));
        file.close();
    }
  
    Serial.println(F("Setup finished."));
}

//------------------------------------------------------------------------------
void checkSDState(void)
{
    cid_t cid;
    
    if (!sd.card()->readCID(&cid)) 
    {
        if(!toggle)
        {
            Serial.println("SD readCID failed");
            toggle = true;
            sdActive = false;
            while(!sd.begin(SD_CHIP_SELECT_PIN));     // check for inserted SD card
        }
    } 
    else 
    {
        sdActive = true;
    }
}

//------------------------------------------------------------------------------
void CAN_print(data_t* data)
{
    if (startMicros == 0) 
    {
        startMicros = data->time;
    }
  
    if((data->rxId & 0x80000000) == 0x80000000)     // Determine if ID is standard (11 bits) or extended (29 bits)
    {
        sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data: ", (data->rxId & 0x1FFFFFFF), data->len);
    }
    else
    {
        sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data: ", data->rxId, data->len);
    }
    
    Serial.print(msgString);
  
    if((data->rxId & 0x40000000) == 0x40000000)   // Determine if message is a remote request frame.
    {    
        sprintf(msgString, " REMOTE REQUEST FRAME");
        Serial.print(msgString);
    } 
    else 
    {
        for(byte i = 0; i < data->len; i++)
        {
            Serial.print(data->rxBuf[i]);
            Serial.print(", ");
        }
    }

    Serial.println();
}

//------------------------------------------------------------------------------
bool CAN_acquireData(data_t* data) 
{
    data->time = micros();

    if(!digitalRead(CAN0_INT))                                  // If CAN0_INT pin is low, read receive buffer
    {
        CAN0.readMsgBuf(&data->rxId, &data->len, data->rxBuf);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
void loop() 
{
    while(1) 
    {
        checkSDState();
        
        if(sdActive && canActive)
        {
            if(CAN_acquireData(&data))
            {
                CAN_print(&data);
            }
        }
    }
}

