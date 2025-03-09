#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>// Import PubSubClient library to initialize MQTT protocol
#include "OTG_HA.h"
#include "wifiProc.h"

#include "credentials.h"

/* Variables */
PubSubClient mqttclient(espClient);
bool mqttInitComplete = false;
unsigned long mqttinit_timenow = 0;
unsigned long mqttrefresh_timenow = 0;
int regMQTTvars = false;
unsigned long time_now_mqtt = 0;

/* Prototypes */
void mqtt_callback(char* topic, byte* payload, unsigned int length);

/* Functions */
void setupMqtt(void)
{
    //setup mqtt
    mqttclient.setServer(MQTT_SERVER, 1883);
    mqttclient.setCallback(mqtt_callback);
    //mqttclient.setKeepAlive(3);
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

void mqttProcessing(void)
{
  char buf[10];

  //check if we have connected to mqtt
  if(false == mqttInitComplete)
  {
    if(millis() > mqttinit_timenow)
    {
      //process mqtt
      if (!mqttclient.connected() || (wifiResetComplete == true)) {
        reconnect_mqtt();
        regMQTTvars = false;
        wifiResetComplete = false;
      }

      //check if a connection has been established
      if(mqttclient.connected())
      {
          mqttInitComplete = true;
      }

      //set the timer for the next attempt
      mqttinit_timenow = (millis() + 5000);
    }
  }
  //Process MQTT messages if we have established a connection
  else
  {
    //check if we need to reconnect
    if (!mqttclient.connected() || (wifiResetComplete == true)) {
      mqttInitComplete = false;
      //set the timer for the next attempt
      mqttinit_timenow = (millis() + 5000);
      Serial.println("MQTT Connection Lost. Reconnecting...");
    }
    else
    {
      mqttclient.loop();

      //if(!regMQTTvars)
      //this is a hack as the connected() function never reports anything other than connected...?
      if(millis() > mqttrefresh_timenow)
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
          delay(20);
          mqttclient.publish("homeassistant/sensor/outtemperature/config", "{\"name\": \"OUT Temperature\", \"device_class\": \"temperature\", \"state_topic\": \"homeassistant/sensor/outtemperature/state\", \"unique_id\": \"11\"}");
          delay(20);
          mqttclient.publish("homeassistant/sensor/burneractivetime/config", "{\"name\": \"Burner Active (Time)\", \"device_class\": \"duration\", \"state_topic\": \"homeassistant/sensor/burneractivetime/state\", \"unique_id\": \"12\"}");          
          regMQTTvars = true;
          mqttrefresh_timenow = millis() + 60000;
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

        sprintf(buf,"%f", outTemperature);
        mqttclient.publish("homeassistant/sensor/outtemperature/state", buf);

        sprintf(buf,"%u", timeBurnerActiveMins);
        mqttclient.publish("homeassistant/sensor/burneractivetime/state", buf);

        time_now_mqtt = millis();
      }
    }
  }
}

void mqttProcMain(void)
{
    if(wifiInitComplete)
    {
        mqttProcessing();
    }
}