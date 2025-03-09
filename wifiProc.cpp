#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "credentials.h"

#include "wifiProc.h"

/* Variables */
WiFiClient   espClient;
bool wifiInitComplete = false;
bool wifiResetComplete = false;
unsigned long wifi_timenow = 0;

/* Prototypes */


/* Functions */
void wifiProcessing(void)
{
  static uint8 state_u8 = 0;
  static uint8 showInitState = false;

  // Check if we have connected to wifi
  if(true == wifiInitComplete) 
  {
    if(false == showInitState)
    {
      // Connected to WiFi
      Serial.println();
      Serial.print("Connected! IP address: ");
      Serial.println(WiFi.localIP());
      showInitState = true;
    }
  }
  else
  {
    showInitState = false;
    
    switch(state_u8)
    {
      /* Attempt to connect */
      case 0:
      {
        //Serial.println("1");
        if (WiFi.status() != WL_CONNECTED)
        {
          // Begin WiFi
          WiFi.begin(WIFI_SSID, WIFI_PASS);

          // Connecting to WiFi...
          Serial.print("Connecting to ");
          Serial.print(WIFI_SSID);

          wifi_timenow = (millis() + 30000);

          wifiInitComplete = false;

          state_u8++;
        }
      }
      break;

      /* Validate the connection */
      case 1:
      {
        //Serial.println("2");
        /* Check if we have are ready to check the status */
        if(millis() > wifi_timenow)
        {
          if( WiFi.status() == WL_CONNECTED )
          {
            wifiInitComplete = true;
            wifiResetComplete = true;
          }

          state_u8 = 0;
        }
      }
      break;

      default:
      {
        /* do nothing */
      }
    }

  }
}

bool wifiConnectedStatus(void)
{
    bool retval_b = false;

    if( WiFi.status() == WL_CONNECTED )
    {
        retval_b = true;
    }

    return(retval_b);
}