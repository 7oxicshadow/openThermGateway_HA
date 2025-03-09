#include <Arduino.h>
#include "time.h"
#include "wifiProc.h"

/* Variables */
struct tm timeinfo;
int prevMday = 0;

/* Prototypes */


/* Functions */
void initTime(void)
{
    // Init and get the time
    configTime(0, 0, "pool.ntp.org"); // (int timezone_sec, int daylightOffset_sec, String server1)
    setenv("TZ","GMT0BST,M3.5.0/1,M10.5.0",1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
    tzset();
    Serial.println("here"); 
}

void updateTime(void)
{
    if(!getLocalTime(&timeinfo))
    {
      Serial.println("Failed to obtain time");
      return;
    }
}

bool newDayCheck(void)
{
    bool retval_b = false;

    /* Check if we have rolled over into a new day */
    if(timeinfo.tm_mday != prevMday)
    {
        prevMday = timeinfo.tm_mday;
        retval_b = true;
    }

    return(retval_b);
}

void timeProcMain(void)
{
    static bool initComplete_b = false;
    
    if(false == initComplete_b)
    {
        if( false != wifiConnectedStatus())
        {
            initTime();
            initComplete_b = true;
        }
    }
    else
    {
        if(wifiInitComplete)
        {
          updateTime();
        }
    }
}
