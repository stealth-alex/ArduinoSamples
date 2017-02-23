#include <SPI.h>
#include "SdFat.h"
#include "mcp_can.h"

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

bool canActive  = false;

//------------------------------------------------------------------------------
void CAN_PrintSettings(int i, int j)
{
    switch(i)
    {
        case 0: 
            Serial.println("CAN_4K096BPS");
            break;
        case 1: 
            Serial.println("CAN_5KBPS");
            break;
        case 2: 
            Serial.println("CAN_10KBPS");
            break;
        case 3: 
            Serial.println("CAN_20KBPS");
            break;
        case 4: 
            Serial.println("CAN_31K25BPS");
            break;
        case 5: 
            Serial.println("CAN_33K3BPS");
            break;
        case 6: 
            Serial.println("CAN_40KBPS");
            break;
        case 7: 
            Serial.println("CAN_50KBPS");
            break;
        case 8: 
            Serial.println("CAN_80KBPS");
            break;
        case 9: 
            Serial.println("CAN_100KBPS");
            break;
        case 10: 
            Serial.println("CAN_125KBPS");
            break;
        case 11: 
            Serial.println("CAN_200KBPS");
            break;
        case 12: 
            Serial.println("CAN_250KBPS");
            break;
        case 13: 
            Serial.println("CAN_500KBPS");
            break;
        case 14: 
            Serial.println("CAN_1000KBPS");
            break;
    }

    switch(j)
    {
        case 0: 
            Serial.println("MCP_20MHZ");
            break;
        case 1: 
            Serial.println("MCP_16MHZ");
            break;
        case 2: 
            Serial.println("MCP_8MHZ");
            break;
    }
}
//------------------------------------------------------------------------------
bool CAN_InputDataSearch()
{
    int i, j;
    
    for (i = 14; i >= 0; i--)
    {
        for(j = 2; j >= 0; j--)
        {
            if(CAN0.begin(MCP_ANY, i, j) == CAN_OK)
            {
                Serial.println("MCP2515 Initialized Successfully!");
                CAN_PrintSettings(i, j); 
                if(CAN0.setMode(MCP_NORMAL) == MCP2515_OK)       // Set operation mode to normal so the MCP2515 sends acks to received data.
                {
                    Serial.println("CAN0 setMode Successfully!");
                }
                pinMode(CAN0_INT, INPUT);       // Configuring pin for /INT input

                for(int num = 0; num < 4; num++)
                {
                    if(CAN_acquireData(&data))
                    {
                        Serial.println("Receive data!");
                        CAN_print(&data);
                        return true;
                    }
                    delay(250);
                }
                Serial.println("NO data during 1 sec!");
            }
            else
            {
                Serial.println("Error Initializing MCP2515...");
            }
        }
    }

    return false;
}

//------------------------------------------------------------------------------
void setup() 
{
    Serial.begin(115200);

    // Wait for USB Serial 
    while (!Serial) 
    {
        SysCall::yield();
    }
  
    Serial.println(F("Setup finished."));
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

    if(CAN0.checkReceive() == CAN_MSGAVAIL)
    {
        CAN0.readMsgBuf(&data->rxId, &data->len, data->rxBuf);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
void loop() 
{
    bool findData = false;
    while(1) 
    {
        if(!findData)
        {
            findData = CAN_InputDataSearch();
        }

        if(findData)
        {
            if(CAN_acquireData(&data))
            {
                CAN_print(&data);
            }
        }
    }
}

