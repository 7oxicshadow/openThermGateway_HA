/*
Based on the OpenTherm Gateway/Monitor Example provided by Ihor Melnyk
http://ihormelnyk.com
*/

#include <Arduino.h>
#include <OpenTherm.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>// Import PubSubClient library to initialize MQTT protocol

#include "credentials.h"

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

const int mInPin = 4; //for Arduino, 4 for ESP8266 (D2), 21 for ESP32
const int mOutPin = 5; //for Arduino, 5 for ESP8266 (D1), 22 for ESP32

const int sInPin = 12; //for Arduino, 12 for ESP8266 (D6), 19 for ESP32
const int sOutPin = 13; //for Arduino, 13 for ESP8266 (D7), 23 for ESP32

WiFiClient   espClient;
PubSubClient mqttclient(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
int regMQTTvars = false;

bool wifiInitComplete = false;
bool mqttInitComplete = false;

unsigned long mqttinit_timenow = 0;

unsigned long time_now = 0;
unsigned long time_now_mqtt = 0;
unsigned long pendingRequest = 0;

bool disable_int = false;

static float boilerTemperature;
static float boilerRetTemperature;
static float modulation;
static bool  flameState;
static float targetSetpoint;
static float roomSetpoint;
static float roomTemperature;
static float waterPressure;
static float dhwSetpoint;
static float dhwTemperature;

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

// Check for Message received on define topic for MQTT Broker
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  //if ((char)payload[0] == '1') {
  //  digitalWrite(D2, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  //} else {
  //  digitalWrite(D2, HIGH);  // Turn the LED off by making the voltage HIGH
  //}

}

void reconnect_mqtt() {
  // Loop until we're reconnected
  if (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    
    if (mqttclient.connect(clientId.c_str(), "mqttuser", "mqttuser")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //mqttclient.publish("LEDTEST2", "hello world");
      // ... and resubscribe
      //mqttclient.subscribe("LEDTEST1");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
    }
  }
}

void setup()
{
    Serial.begin(9600);	//9600 supported by OpenTherm Monitor App

    // Begin WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Connecting to WiFi...
    Serial.print("Connecting to ");
    Serial.print(WIFI_SSID);

    //setup mqtt
    mqttclient.setServer(MQTT_SERVER, 1883);
    mqttclient.setCallback(mqtt_callback);

    //start master/slave interface
    mOT.begin(mHandleInterrupt);
    sOT.begin(sHandleInterrupt, ot_processRequest);
}

void wifiProcessing(void)
{
  // Check if we have connected to wifi
  if ( (WiFi.status() == WL_CONNECTED) && (false == wifiInitComplete) )
  {
    // Connected to WiFi
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    wifiInitComplete = true;
  }
}

void mqttProcessing(void)
{
  char buf[10];

  //check if we have connected to mqtt
  if(false == mqttInitComplete)
  {
    if(millis() > mqttinit_timenow)
    {
      //process mqtt
      if (!mqttclient.connected()) {
        reconnect_mqtt();
        regMQTTvars = false;
      }

      //check if a connection has been established
      if(mqttclient.connected())
      {
          mqttInitComplete = true;
      }
    }

    //set the timer for the next attempt
    mqttinit_timenow = (millis() + 5000);
  }
  //Process MQTT messages if we have established a connection
  else
  {
    mqttclient.loop();
  
    if(!regMQTTvars)
    {
        Serial.println("MQTT Sensor Config");
        mqttclient.publish("homeassistant/binary_sensor/flame_state/config", "{\"name\": \"Flame State\", \"device_class\": \"heat\", \"state_topic\": \"homeassistant/binary_sensor/flame_state/state\", \"unique_id\": \"1\"}");
        delay(20);
        mqttclient.publish("homeassistant/sensor/temperature/config", "{\"name\": \"Temperature\", \"device_class\": \"temperature\", \"state_topic\": \"homeassistant/sensor/temperature/state\", \"unique_id\": \"2\"}");
        delay(20);
        mqttclient.publish("homeassistant/sensor/modulation/config", "{\"name\": \"Modulation\", \"device_class\": \"moisture\", \"state_topic\": \"homeassistant/sensor/modulation/state\", \"unique_id\": \"3\"}");
        delay(20);
        mqttclient.publish("homeassistant/sensor/returntemp/config", "{\"name\": \"Return Temperature\", \"device_class\": \"temperature\", \"state_topic\": \"homeassistant/sensor/returntemp/state\", \"unique_id\": \"4\"}");
        delay(20);
        mqttclient.publish("homeassistant/sensor/targetsetpoint/config", "{\"name\": \"Target Setpoint\", \"device_class\": \"temperature\", \"state_topic\": \"homeassistant/sensor/targetsetpoint/state\", \"unique_id\": \"5\"}");
        delay(20);
        mqttclient.publish("homeassistant/sensor/roomsetpoint/config", "{\"name\": \"Room Setpoint\", \"device_class\": \"temperature\", \"state_topic\": \"homeassistant/sensor/roomsetpoint/state\", \"unique_id\": \"6\"}");
        delay(20);
        mqttclient.publish("homeassistant/sensor/roomtemperature/config", "{\"name\": \"Room Temperature\", \"device_class\": \"temperature\", \"state_topic\": \"homeassistant/sensor/roomtemperature/state\", \"unique_id\": \"7\"}");
        delay(20);        
        mqttclient.publish("homeassistant/sensor/waterpressure/config", "{\"name\": \"Water Pressure\", \"device_class\": \"pressure\", \"state_topic\": \"homeassistant/sensor/waterpressure/state\", \"unique_id\": \"8\"}");
        delay(20);
        mqttclient.publish("homeassistant/sensor/dhwsetpoint/config", "{\"name\": \"DHW Setpoint\", \"device_class\": \"temperature\", \"state_topic\": \"homeassistant/sensor/dhwsetpoint/state\", \"unique_id\": \"9\"}");
        delay(20);
        mqttclient.publish("homeassistant/sensor/dhwtemperature/config", "{\"name\": \"DHW Temperature\", \"device_class\": \"temperature\", \"state_topic\": \"homeassistant/sensor/dhwtemperature/state\", \"unique_id\": \"10\"}");
        regMQTTvars = true;
    }

    //process mqtt on a timer
    if(millis() > time_now_mqtt + 10000)
    {

      sprintf(buf,"%f", boilerTemperature);
      mqttclient.publish("homeassistant/sensor/temperature/state", buf);

      sprintf(buf,"%f", boilerRetTemperature);
      mqttclient.publish("homeassistant/sensor/returntemp/state", buf);

      sprintf(buf,"%f", modulation);
      mqttclient.publish("homeassistant/sensor/modulation/state", buf);

      if(flameState > 0)
          sprintf(buf,"ON");
      else
          sprintf(buf,"OFF");

      mqttclient.publish("homeassistant/binary_sensor/flame_state/state", buf);

      sprintf(buf,"%f", targetSetpoint);
      mqttclient.publish("homeassistant/sensor/targetsetpoint/state", buf);

      sprintf(buf,"%f", roomSetpoint);
      mqttclient.publish("homeassistant/sensor/roomsetpoint/state", buf);   

      sprintf(buf,"%f", roomTemperature);
      mqttclient.publish("homeassistant/sensor/roomtemperature/state", buf);    

      sprintf(buf,"%f", waterPressure);
      mqttclient.publish("homeassistant/sensor/waterpressure/state", buf);   

      sprintf(buf,"%f", dhwSetpoint);
      mqttclient.publish("homeassistant/sensor/dhwsetpoint/state", buf);    

      sprintf(buf,"%f", dhwTemperature);
      mqttclient.publish("homeassistant/sensor/dhwtemperature/state", buf);  

      time_now_mqtt = millis();
    }
  }
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

  // must be non-blocking
  if(wifiInitComplete)
    mqttProcessing();

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
            //response = mOT.sendRequest(mOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::TrSet, 0));
            //temp_f = mOT.isValidResponse(response) ? mOT.getFloat(response) : 0;

            //if(temp_f != 0)
            //{
            //    roomSetpoint = temp_f;
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
}
