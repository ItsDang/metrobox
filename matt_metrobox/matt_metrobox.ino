#include "WiFiS3.h"
#include <HttpClient.h>
#include "ArduinoJson.h"
#include "arduino_secrets.h"

// template based on https://github.com/amcewen/HttpClient/blob/master/examples/SimpleHttpExample/SimpleHttpExample.ino
// and through reading https://github.com/arduino-libraries/ArduinoHttpClient/blob/master/examples/CustomHeader/CustomHeader.ino

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key index number (needed only for WEP)

char apikey[] = SECRET_WMATA_API_PRIMARY_KEY;
char stationcode[] = SECRET_STATION_CODE;

WiFiClient client;

const char kHostname[] = "api.wmata.com";
// Path to download (this is the bit after the hostname in the URL
// that you want to download
const char kPath[] = "/StationPrediction.svc/json/GetPrediction/E01";

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30*1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;
int status = WL_IDLE_STATUS;

void setup() {
/* -------------------------------------------------------------------------- */  
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
     
    // wait 10 seconds for connection:
    delay(10000);
  }
}


String groups[] = {"1", "2"};
int group_idx = 0;

void loop()
{
  int err =0;
  
  WiFiClient wc;
  HttpClient http(wc);
  Serial.println("making GET request");
  http.beginRequest();
  http.get(kHostname, kPath);
  http.sendHeader("api_key", apikey);
  http.endRequest();
  if (err == 0)
  {
    Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err);

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get

      err = http.skipResponseHeaders();
      if (err >= 0)
      {
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body returned follows:");
      
        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        String payload;
        char bodyByte;
        // Whilst we haven't timed out & haven't reached the end of the body
        while ( (http.connected() || http.available()) &&
               ((millis() - timeoutStart) < kNetworkTimeout) )
        {
            if (http.available())
            {
                bodyByte = http.read();
                // Print out this character
                // Serial.print(bodyByte);
                payload += bodyByte;
               
                bodyLen--;
                // We read something, reset the timeout counter
                timeoutStart = millis();
            }
            else
            {
                // We haven't got any data, so let's pause to allow some to
                // arrive
                delay(kNetworkDelay);
            }
        }
        // 256 kB Flash, 32 kB RAM // for now storing whole thing
        // following https://arduinojson.org/v7/example/parser/
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        // Test if parsing succeeds.
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }

        // then code from original
        int idx = 0;
        Serial.print("T:");
        for (JsonObject Train : doc["Trains"].as<JsonArray>()) {
            if (Train["Group"].as<String>() == groups[group_idx]) {
                Serial.print(Train["Line"].as<String>() + ',');
                Serial.print(Train["Car"].as<String>() + ',');
                Serial.print(Train["Destination"].as<String>() + ',');
                Serial.print(Train["Min"].as<String>() + ',');
                idx++;
                if (idx == 3) {
                    break;
                }
            }
        }

        group_idx = (group_idx + 1) % 2;

        // fill missing lines
        for (int i = idx; i < 3; i++) {
            Serial.print(",,,,");
        }
        Serial.println();

        Serial.println("Printing whole payload");
        Serial.println(payload);
      }
      else
      {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }
    else
    {    
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();

  // And just stop, now that we've tried a download
  // while(1);
  Serial.println("Wait thirty seconds");
  /*
  wait 10 seconds to refresh, there are 86,400s/day, max call is 50,000/day, so lets call every 10s? 8,640 calls
  Next train arrival information is refreshed once every 20 to 30 seconds approximately.
  */
  delay(30000); 
}