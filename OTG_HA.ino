/*
Based on the OpenTherm Gateway/Monitor Example provided by Ihor Melnyk
http://ihormelnyk.com
*/

#include <Arduino.h>
#include <OpenTherm.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>// Import PubSubClient library to initialize MQTT protocol

#include "credentials.h"

#include "wifiProc.h"
#include "mqttProc.h"
#include "timeProc.h"

//macro to disable print functions (comment out to disable)
#define ENABLE_PRINT

#ifndef ENABLE_PRINT
  // disable Serial output
  #define Serial _disable
  static class {
  public:
      void begin(...) {}
      void print(...) {}
      void println(...) {}
  } Serial;
#endif

//const char* ntpServer = "pool.ntp.org";
//const long  gmtOffset_sec = 0;
//const int   daylightOffset_sec = 0; //3600;

const int mInPin = 4; //for Arduino, 4 for ESP8266 (D2), 21 for ESP32
const int mOutPin = 5; //for Arduino, 5 for ESP8266 (D1), 22 for ESP32

const int sInPin = 12; //for Arduino, 12 for ESP8266 (D6), 19 for ESP32
const int sOutPin = 13; //for Arduino, 13 for ESP8266 (D7), 23 for ESP32

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

unsigned long time_now = 0;
unsigned long pendingRequest = 0;

unsigned long timeBurnerActiveMins = 0;
unsigned long prevBATime = 0;
long timeBurnerActive_mS = 0;
long tempTime_ms = 0;

bool disable_int = false;

float boilerTemperature;
float boilerRetTemperature;
float modulation;
bool  flameState;
float targetSetpoint;
float roomSetpoint;
float roomTemperature;
float waterPressure;
float dhwSetpoint;
float dhwTemperature;
float outTemperature;

OpenTherm mOT(mInPin, mOutPin);
OpenTherm sOT(sInPin, sOutPin, true);

void IRAM_ATTR mHandleInterrupt() {
    mOT.handleInterrupt();
}

void IRAM_ATTR sHandleInterrupt() {
    sOT.handleInterrupt();
}

void ot_processRequest(unsigned long request, OpenThermResponseStatus status) {

    pendingRequest = request;
    
    //if(!disable_int)
    //{
    //  Serial.println("T" + String(request, HEX));  //master/thermostat request
    //  response = mOT.sendRequest(request);
    //  if (response) {
    //      Serial.println("B" + String(response, HEX)); //slave/boiler response
    //      sOT.sendResponse(response);
    //  }
    //}
}

void setup()
{
    Serial.begin(9600);	//9600 supported by OpenTherm Monitor App

    setupMqtt();

    //start master/slave interface
    mOT.begin(mHandleInterrupt);
    sOT.begin(sHandleInterrupt, ot_processRequest);
}

void loop()
{
  #define STATUS_REQ_MISS_CNT    (10)

  unsigned long response;
  float temp_f;

  static unsigned char reqTracker = 0;
  static unsigned char count = 0;
  static unsigned char statusReqMissing = STATUS_REQ_MISS_CNT;
  static unsigned long lastStatusResponse = 0;

  // must be non-blocking
  wifiProcessing();

  /* Call main tasks for submodules */
  mqttProcMain();
  timeProcMain();

  //process slave node
  sOT.process();

  /* Have we had an interrupt for a thermostat message instead
      of requesting */
  if(pendingRequest != 0)
  {
    /* pull the room temp from the thermostat message instead
        of requesting */
    if((pendingRequest & 0x00FF0000) == 0x00180000)
    {
      roomTemperature = mOT.getFloat(pendingRequest);
    }
    /* pull the room setpoint from the thermostat */
    else if((pendingRequest & 0x00FF0000) == 0x00100000)
    {
      roomSetpoint = mOT.getFloat(pendingRequest);
    }
    else
    {
      /* do nothing */
    }

    //Serial.println("I");

    //Serial.println("T" + String(pendingRequest, HEX));  //master/thermostat request
    response = mOT.sendRequest(pendingRequest);
    
    if (response) {

        //check if the thermostat has requested a status messsage
        if(pendingRequest == 0x300)
        {
          //record the response to the status request
          lastStatusResponse = response;
          statusReqMissing = STATUS_REQ_MISS_CNT;
        }
        else
        {
            //this is a hack to detect that we have no status message 
            if(statusReqMissing == 0)
            {
              lastStatusResponse = 0;
            }
            else
            {
              statusReqMissing--;
            }
        }

        /* Get water pressure when the thermostat requests it */
        if(pendingRequest == 0x00120000)
        {
            waterPressure = mOT.getFloat(response);
        }
        /* pull the DHW temperature from the boiler response */
        else if((pendingRequest & 0x00FF0000) == 0x001A0000)
        {
          dhwTemperature = mOT.getFloat(response);
        }
        /* pull the outside temperature from the boiler response */
        else if((pendingRequest & 0x00FF0000) == 0x001B0000)
        {
          outTemperature = mOT.getFloat(response);
        }
        /* pull the DHW setpoint from the boiler response */
        else if((pendingRequest & 0x00FF0000) == 0x00380000)
        {
          dhwSetpoint = mOT.getFloat(response);
        }
        else
        {
          /* do nothing */
        }

        //Serial.println("B" + String(response, HEX)); //slave/boiler response
        sOT.sendResponse(response);
    }

    //clear the pending request
    pendingRequest = 0;

    //spec says 100ms delay before next message can be transmitted
    delay(100);
  }
  else
  {
    //process on a timer
    if(millis() > time_now + 200)
    //if(0)
    {

      //Serial.println("L");

      switch(reqTracker)
      {
        case 0:
        {
            temp_f = mOT.getBoilerTemperature();

            if(temp_f != 0)
            {
              boilerTemperature = temp_f;
            }

            //Serial.print("0: ");
            //Serial.println(buf);
            reqTracker++;
        }
        break;

        case 1:
        {
            modulation = mOT.getModulation();

            //Serial.print("1: ");
            //Serial.println(buf);
            reqTracker++;
        }
        break;

        case 2:
        {
            temp_f = mOT.getReturnTemperature();

            if(temp_f != 0)
            {
                boilerRetTemperature = temp_f;
            }

            //Serial.print("2: ");
            //Serial.println(buf);
            reqTracker++;
        }
        break;

        case 3:
        {
            response = mOT.sendRequest(mOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TSet, 0));
            temp_f = mOT.isValidResponse(response) ? mOT.getFloat(response) : 0;

            if(temp_f != 0)
            {
                targetSetpoint = temp_f;
            }

            //Serial.print("3: ");
            //Serial.println(buf);

            reqTracker++;
        }
        break;

        case 4:
        {
            //response = mOT.sendRequest(mOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Toutside, 0));
            //temp_f = mOT.isValidResponse(response) ? mOT.getFloat(response) : 0;

            //if(temp_f != 0)
            //{
            //    outTemperature = temp_f;
            //}

            //Serial.print("3: ");
            //Serial.println(buf);

            reqTracker++;
        }
        break;

        case 5:
        {
            //response = mOT.sendRequest(mOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tr, 0));
            //temp_f = mOT.isValidResponse(response) ? mOT.getFloat(response) : 0;

            //if(temp_f != 0)
            //{
            //   roomTemperature = temp_f;
            //}

            //Serial.print("3: ");
            //Serial.println(buf);

            reqTracker++;
        }
        break;

        case 6:
        {
            
            //we cannot request the status from the boiler because the thermostat is
            //doing it. use the last received status instead
            if(lastStatusResponse != 0U)
            {
              flameState = mOT.isValidResponse(lastStatusResponse) ? mOT.isFlameOn(lastStatusResponse) : 2;
            }
            else
            {
              flameState = 0U;
            }

            reqTracker = 0;
        }
        break;

        default:
        {

        }
        break;

      }
      
      time_now = millis();

    }
  }

  /* Calcualte how much time the burner has been active for */
  if( false != flameState )
  {
      /* we need a sample before we work out a delta */
      if(prevBATime == 0U)
      {
          prevBATime = millis();
      }
      else
      {
          timeBurnerActive_mS += (long)(millis() - prevBATime);
          prevBATime = millis();
      }

      tempTime_ms = (timeBurnerActive_mS - 60000);

      if( tempTime_ms >= 0 )
      {
          timeBurnerActiveMins++;
          timeBurnerActive_mS = tempTime_ms;
      }
  }
  else
  {
      prevBATime = 0U;
  }

  /* Check if we need to reset the burner active time
     based on a new day */
  if(false != newDayCheck())
  {
      timeBurnerActiveMins = 0U;
  }
}
