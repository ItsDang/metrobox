// get information libs
#include "WiFiS3.h"
#include "HttpClient.h"
#include "ArduinoJson.h"

// display libs
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// wifi and api key
#include "arduino_secrets.h"

// template based on https://github.com/amcewen/HttpClient/blob/master/examples/SimpleHttpExample/SimpleHttpExample.ino
// and through reading https://github.com/arduino-libraries/ArduinoHttpClient/blob/master/examples/CustomHeader/CustomHeader.ino

// longest station name is E03
// what is the longest destination?

char ssid[] = SECRET_SSID;  // your network SSID (name)
char pass[] = SECRET_PASS;  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;           // your network key index number (needed only for WEP)

char apikey[] = SECRET_WMATA_API_PRIMARY_KEY;
const char stationcode[] = SECRET_STATION_CODE;

WiFiClient client;

const char kHostname[] = "api.wmata.com";
// Path to download (this is the bit after the hostname in the URL
// that you want to download
// E01
char stationInfoPath[] = "/Rail.svc/json/jStationInfo?StationCode=E01";
char stationPredictionPath[] = "/StationPrediction.svc/json/GetPrediction/E01";

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30 * 1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;
int status = WL_IDLE_STATUS;

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {
  /* -------------------------------------------------------------------------- */
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
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

  Serial.print("Station Code is ");
  Serial.println(stationcode);
  Serial.println("PATH BEFORE");
  Serial.println(stationInfoPath);
  Serial.println(stationPredictionPath);
  char * pstationInfoBlank = strstr(stationInfoPath, "E01");
  Serial.println("Pointer position");
  Serial.println(pstationInfoBlank);
  memcpy(pstationInfoBlank, stationcode, 3);
  char * pstationPredictionBlank = strstr(stationPredictionPath, "E01");
  memcpy(pstationPredictionBlank, stationcode, 3);
  Serial.println("AFTER");
  Serial.println(stationInfoPath);
  Serial.println(stationPredictionPath);

  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(3);  // so cable is low

  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x");
  Serial.println(x, HEX);

  getStation();
}

void getStation() {
  int err = 0;

  WiFiClient wc;
  HttpClient http(wc);
  Serial.println("making GET request");
  http.beginRequest();
  http.get(kHostname, stationInfoPath);
  http.sendHeader("api_key", apikey);
  http.endRequest();
  if (err == 0) {
    Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 0) {
      Serial.print("Got status code: ");
      Serial.println(err);

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get

      err = http.skipResponseHeaders();
      if (err >= 0) {
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
        while ((http.connected() || http.available()) && ((millis() - timeoutStart) < kNetworkTimeout)) {
          if (http.available()) {
            bodyByte = http.read();
            // Print out this character
            // Serial.print(bodyByte);
            payload += bodyByte;

            bodyLen--;
            // We read something, reset the timeout counter
            timeoutStart = millis();
          } else {
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
        Serial.println("PAYLOAD");
        Serial.println(payload);
        Serial.println("PAYLOAD JSON");
        Serial.println(doc["Name"].as<String>());
        JsonObject Station = doc.as<JsonObject>();
        showStation(doc["Name"].as<String>(), doc["LineCode1"].as<String>(), doc["LineCode2"].as<String>(), doc["LineCode3"].as<String>(), doc["LineCode4"].as<String>());
      } else {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    } else {
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  } else {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();
}

void showStation(String stationName, String LineCode1, String LineCode2, String LineCode3, String LineCode4) {
  // first clear the station space
  tft.fillRect(0, 0, ILI9341_TFTHEIGHT, ILI9341_TFTWIDTH / 5, ILI9341_BLACK);
  // then draw the line rectangles
  int lines = 0;
  if (LineCode1 != "null") {
    tft.fillRect(ILI9341_TFTHEIGHT / 40 * lines, 0, ILI9341_TFTHEIGHT / 40, ILI9341_TFTWIDTH / 10 * 3, getColorFromLine(LineCode1, 0));
    lines++;
  }
  if (LineCode2 != "null") {
    tft.fillRect(ILI9341_TFTHEIGHT / 40 * lines, 0, ILI9341_TFTHEIGHT / 40, ILI9341_TFTWIDTH / 10 * 3, getColorFromLine(LineCode2, 0));
    lines++;
  }
  if (LineCode3 != "null") {
    tft.fillRect(ILI9341_TFTHEIGHT / 40 * lines, 0, ILI9341_TFTHEIGHT / 40, ILI9341_TFTWIDTH / 10 * 3, getColorFromLine(LineCode3, 0));
    lines++;
  }
  if (LineCode4 != "null") {
    tft.fillRect(ILI9341_TFTHEIGHT / 40 * lines, 0, ILI9341_TFTHEIGHT / 40, ILI9341_TFTWIDTH / 10 * 3, getColorFromLine(LineCode4, 0));
    lines++;
  }

  tft.setFont(NULL);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);  // 3 fits

  printStationNameFormatted(stationName, lines);
  // tft.setCursor(ILI9341_TFTHEIGHT / 40 * lines, 0);
  // String stationNameLine1 = stationName.substring(0, 16);
  // String stationNameLine2 = stationName.substring(16, 32);
  // String stationNameLine3 = stationName.substring(32);
  // Serial.println(stationNameLine1);
  // Serial.println(stationNameLine2);
  // Serial.println(stationNameLine3);
  // tft.println(stationNameLine1);
  // tft.setCursor(ILI9341_TFTHEIGHT / 40 * lines, ILI9341_TFTWIDTH / 10 * 1);
  // tft.println(stationNameLine2);
  // tft.setCursor(ILI9341_TFTHEIGHT / 40 * lines, ILI9341_TFTWIDTH / 10 * 2);
  // tft.println(stationNameLine3);
}

void printStationNameFormatted(String stationName, int lines) {
  // the special cases
  if(stationName == " ") {
    Serial.println("HUH");
  } else { // if can be split based on the last index of ' ' ',' '/' '-' that is less
    Serial.println("Splitting: "+stationName);
    int startIndex = 0;
    for(int line = 0; line < 3; line++) {
      Serial.println("LINE: " + String(line));
      tft.setCursor(ILI9341_TFTHEIGHT / 40 * lines, ILI9341_TFTWIDTH / 10 * line);
      int searchBackFrom = 16;
      if(lines == 1) {
        searchBackFrom += 1;
      }
      Serial.println("Searching:");
      String stringToSearch=stationName.substring(startIndex, startIndex+searchBackFrom+1);
      Serial.println(stringToSearch);
      int splitIndex = max(stringToSearch.lastIndexOf(' ',searchBackFrom),
      max(stringToSearch.lastIndexOf(',',searchBackFrom),
      max(stringToSearch.lastIndexOf('/',searchBackFrom),stringToSearch.lastIndexOf('-',searchBackFrom))));
      Serial.println("Found " + String(stringToSearch.charAt(splitIndex)) + " at: " + String(splitIndex));
      Serial.println(stringToSearch.substring(0, splitIndex+1));
      if(splitIndex == -1 || splitIndex == 0) { // no split so just print out the line and stop
        tft.println(stringToSearch);
        break;
      } else {
        tft.println(stringToSearch.substring(0, splitIndex));
        startIndex += splitIndex; // not a ' ' so have to keep on next line, need to check not 1
        if(stringToSearch.charAt(splitIndex) == ' ') { // ' ' space at end of line, can "delete"
          startIndex += 1;
        }
      }
    }
  }
}

// useful reference https://github.com/newdigate/rgb565_colors
// these are defined in Adafruit_ILI9341.h
// see https://rgbcolorpicker.com/ and convert directly from the SVGs?
#define BLACK 0x0000   // #define ILI9341_BLACK 0x0000
#define RED 0xf0e3     // #define ILI9341_RED 0xF800
#define ORANGE 0xfbc0  // #define ILI9341_ORANGE 0xFD20
#define BLUE 0x1af8    // #define ILI9341_BLUE 0x001F
#define GREEN 0x0520   // #define ILI9341_GREEN 0x07E0
#define YELLOW 0xef61  // #define ILI9341_YELLOW 0xFFE0
#define WHITE 0xFFFF   // #define ILI9341_WHITE 0xFFFF
#define GREY 0x8c51    // #define ILI9341_LIGHTGREY 0xC618 #define ILI9341_DARKGREY 0x7BEF
#define TRAIN 0xFD20   // like a yellow orange, is actually the same as ILI9341_ORANGE
// from the svgs
#define SVG_BLUE 0x03d8    //"#0079C5"
#define SVG_GREEN 0x1569   //"#13AE4E"
#define SVG_ORANGE 0xfc85  //"#FF9127"
#define SVG_RED 0xf165     //"#FA2C2D"
#define SVG_SILVER 0x9d34  //"#9BA6A3"
#define SVG_YELLOW 0xfe67  //"#FFCE36"

uint16_t getColorFromLine(String line, int colorScheme) {
  if (colorScheme == 0) {  // original metro box colors, though this is with different display
    if (line == "WH") return WHITE;
    if (line == "RD") return RED;
    if (line == "OR") return ORANGE;
    if (line == "BL") return BLUE;
    if (line == "GR") return GREEN;
    if (line == "YL") return YELLOW;
    if (line == "SV") return GREY;
  } else if (colorScheme == 1) {  // ILI9341 colors
    if (line == "WH") return ILI9341_WHITE;
    if (line == "RD") return ILI9341_RED;
    if (line == "OR") return ILI9341_ORANGE;
    if (line == "BL") return ILI9341_BLUE;
    if (line == "GR") return ILI9341_GREEN;
    if (line == "YL") return ILI9341_YELLOW;
    if (line == "SV") return GREY;
  } else {  // colors from the SVGs
    if (line == "WH") return ILI9341_WHITE;
    if (line == "RD") return SVG_RED;
    if (line == "OR") return SVG_ORANGE;
    if (line == "BL") return SVG_BLUE;
    if (line == "GR") return SVG_GREEN;
    if (line == "YL") return SVG_YELLOW;
    if (line == "SV") return SVG_SILVER;
  }
}

String groups[] = { "1", "2" };
int group_idx = 0;

void loop() {
  // testLayout();  
  real();
  // And just stop, now that we've tried a download
  // while(1);
  /*
  wait 10 seconds to refresh, there are 86,400s/day, max call is 50,000/day, so lets call every 10s? 8,640 calls
  Next train arrival information is refreshed once every 20 to 30 seconds approximately.
  */
  Serial.println("Wait thirty seconds");
  delay(30000);
}

// max chars per line with the station color lines
// 1 max 17
// 2 max 16
// 3 max 16
// 4 max 16
// lazy way first, cut into strings of 16
// stations with -, have - then \n
// any outliers?
// stations with " "
// the stations to split,
// if less that max don't worry about splitting
// count to 16, if see any ' ', '-', '/' then keep on that line then new line

  // station info

  // header

  // train info, ln is two currently, car is single digit,
  // dest is up to "MT VERNON-CNV CT" AFAIK, so 16 chars,
  //  min is up to 2 digits or 3 chars for ARR or BRD

  // probably best to save space by either removing a char from line and ARR or BRD
  // and tighten the spacing via cursor shifting instead of spaces
void testLayout(){
  tft.setTextSize(3);
  // station line colors
  tft.fillRect(ILI9341_TFTHEIGHT / 40 * 0, 0, ILI9341_TFTHEIGHT / 40, ILI9341_TFTWIDTH / 10 * 3, ILI9341_RED);
  tft.fillRect(ILI9341_TFTHEIGHT / 40 * 1, 0, ILI9341_TFTHEIGHT / 40, ILI9341_TFTWIDTH / 10 * 3, ILI9341_BLUE);
  tft.fillRect(ILI9341_TFTHEIGHT / 40 * 2, 0, ILI9341_TFTHEIGHT / 40, ILI9341_TFTWIDTH / 10 * 3, GREY);
  tft.fillRect(ILI9341_TFTHEIGHT / 40 * 3, 0, ILI9341_TFTHEIGHT / 40, ILI9341_TFTWIDTH / 10 * 3, ILI9341_GREEN);
  // station
  tft.fillRect(ILI9341_TFTHEIGHT / 40 * 4, 0, ILI9341_TFTHEIGHT / 40 * 36, ILI9341_TFTWIDTH / 10 * 3, ILI9341_PINK); 
  tft.setCursor(ILI9341_TFTHEIGHT / 40 * 4,0);
  tft.println("U Street/African1");
  tft.setCursor(ILI9341_TFTHEIGHT / 40 * 4,ILI9341_TFTWIDTH / 10);
  tft.println("-Amer Civil War 1");
  tft.setCursor(ILI9341_TFTHEIGHT / 40 * 4,ILI9341_TFTWIDTH / 10 * 2);
  tft.println("Memorial/Cardozo1");
  // header
  tft.fillRect(0, ILI9341_TFTWIDTH / 10 * 3, ILI9341_TFTHEIGHT, ILI9341_TFTWIDTH / 10 * 1, ILI9341_RED);
  tft.setCursor(0,ILI9341_TFTWIDTH / 10 * 3);
  tft.println("LN CAR DEST   MIN");
  // lines
  tft.fillRect(0, ILI9341_TFTWIDTH / 10 * 4, ILI9341_TFTHEIGHT, ILI9341_TFTWIDTH / 10 * 6, ILI9341_NAVY); 
  tft.setCursor(0,ILI9341_TFTWIDTH / 10 * 4);
  tft.println("GR 8   MT VERNON-CNV CT ARR");
  tft.println("GR 8   MT VERNON-CNV CT ARR");
  tft.println("GR 8   MT VERNON-CNV CT ARR");
}

void real() {
  int err = 0;

  WiFiClient wc;
  HttpClient http(wc);
  Serial.println("making GET request");
  http.beginRequest();
  http.get(kHostname, stationPredictionPath);
  http.sendHeader("api_key", apikey);
  http.endRequest();
  if (err == 0) {
    Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 0) {
      Serial.print("Got status code: ");
      Serial.println(err);

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get

      err = http.skipResponseHeaders();
      if (err >= 0) {
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
        while ((http.connected() || http.available()) && ((millis() - timeoutStart) < kNetworkTimeout)) {
          if (http.available()) {
            bodyByte = http.read();
            // Print out this character
            // Serial.print(bodyByte);
            payload += bodyByte;

            bodyLen--;
            // We read something, reset the timeout counter
            timeoutStart = millis();
          } else {
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
        showHeader();
        // was in showTrains earlier so only showed one line
        tft.setCursor(0,ILI9341_TFTWIDTH / 10 * 4);
        tft.fillRect(0, ILI9341_TFTWIDTH / 10 * 4, ILI9341_TFTHEIGHT, ILI9341_TFTWIDTH / 10 * 6, ILI9341_BLACK);    // reset screen
        // need to understand what this does, check agaisnt terminal stations to see if it removes the dups there
        for (JsonObject Train : doc["Trains"].as<JsonArray>()) {
          if (Train["Group"].as<String>() == groups[group_idx]) {
            Serial.print(Train["Line"].as<String>() + ',');
            Serial.print(Train["Car"].as<String>() + ',');
            Serial.print(Train["Destination"].as<String>() + ',');
            Serial.print(Train["Min"].as<String>() + ',');
            showTrains(Train["Line"].as<String>(), Train["Car"].as<String>(), Train["Destination"].as<String>(), Train["Min"].as<String>());
            idx++;
            if (idx == 3) {
              break;
            }
          }
        }

        group_idx = (group_idx + 1) % 2;

        // fill missing lines // don't need because I'm not reading serial
        for (int i = idx; i < 3; i++) {
          Serial.print(",,,,");
        }
        Serial.println();

        Serial.println("Printing whole payload");
        Serial.println(payload);
      } else {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    } else {
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  } else {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();
}

void showHeader() {
  tft.setFont(NULL);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(3);  // 3 fits
  tft.setCursor(0, ILI9341_TFTWIDTH / 10 * 3);
  tft.print("LN CAR DEST   MIN");
}

void showTrains(String line, String car, String destination, String minutes) {
  // I think I need to swap height and width here because of the rotation
  tft.setFont(NULL);
  tft.setTextColor(ILI9341_ORANGE);
  tft.setTextColor(TRAIN);
  tft.print(line);
  // tft.setCursor(tft.getCursorX()+5, tft.getCursorY());
  tft.print(" ");  // to be replace with tft.setCursor(0, 0);
  tft.print(car);
  // tft.setCursor(tft.getCursorX()+5, tft.getCursorY());
  tft.print(" ");  // to be replace with tft.setCursor(0, 0);
  tft.print(destination);
  // tft.setCursor(tft.getCursorX()+5, tft.getCursorY());
  tft.print(" ");  // to be replace with tft.setCursor(0, 0);
  tft.println(minutes);
}