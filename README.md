# Metrobox
Based off of https://github.com/TokyoExpress/metrobox.
Source code and build for a miniature WMATA Metrorail train arrivals board for an Arduino Uno R4 Wifi and a ILI9341 chipset display instead.

![494856235_698844399746148_1401936910194309229_n 1](https://github.com/user-attachments/assets/a64b9ca4-966c-4bbc-8b6c-76393950ed5d)

---

## Parts and Build

To make this yourself, you will need the following
- [Arduino Uno R4 Wifi](https://store-usa.arduino.cc/products/uno-r4-wifi)
- [2.8" TFT Touch Shield for Arduino with Resistive Touch Screen v2 - STEMMA QT / Qwiic](https://www.adafruit.com/product/1651)
- USB-C cable(optionally right-angled)
- 3D printer or access to 3D printing services

Altogether the total cost should come out to around $50-55.

![components](https://github.com/user-attachments/assets/339163df-8bea-435f-b5e7-586ea0b838e8)

### Uploading Software

1. Install the [Arduino IDE](https://www.arduino.cc/en/software/).
2. Install the Arduino Uno R4 Boards platform into [Arduino IDE Boards Manager](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-board-manager/)
3. [Add libraries](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-installing-a-library/) WiFiManager, ArduinoJson, MCUFRIEND_kbv, and Adafruit GFX Library.
4. You are now ready to begin [uploading sketches](https://docs.arduino.cc/software/ide-v2/tutorials/getting-started/ide-v2-uploading-a-sketch/) to the board. To start, ensure that the Arduino UNO R4 Wifi is recognized by Arduino IDE when plugged into the computer.

Make sure for the following steps that your Serial Monitor baud rate is set to 9600.

### Configuring the Metrobox
#### Obtain a WMATA API key

1. Sign up for an account at https://developer.wmata.com/signup/ and verify your email.
Once you have an account, go to Products -> Default Tier and subscribe a new product in the section shown below (the name does not matter).

<img src="https://github.com/user-attachments/assets/0f91c9cd-96ff-4a55-b4f0-fac1dbf8b521" width="1000">

2. You should be taken to the Profile page, where you can access your new API key. Keep this page open.

<img src="https://github.com/user-attachments/assets/53431f79-78f3-45a3-9787-44904617d616" width="800">

#### Find Your Station Code

Find your station name's corresponding 3-digit code in [wmata_station_codes.csv](wmata_station_codes.csv). Some stations have multiple platforms, so be sure you choose the correct platform with the lines you typically take. Keep this page open as well.

#### Update arduino_secrets.h
Open Arduino IDE and open matt_metrobox.ino.
Switch to tab named arduino_secrets_template.h.
Update the Wifi Credentials, the API key and the Station Code and rename the file to arduino_secrets.h.
<img width="1078" alt="Screenshot 2025-07-04 at 6 14 50 PM" src="https://github.com/user-attachments/assets/8d3ee4d9-4c31-4fd9-8e57-b99af88732a4" />
<img width="480" alt="Screenshot 2025-07-04 at 6 15 00 PM" src="https://github.com/user-attachments/assets/0f298b04-0317-4753-b1d7-cccbae687df1" />
<img width="585" alt="Screenshot 2025-07-04 at 6 10 12 PM" src="https://github.com/user-attachments/assets/27fb52f2-dcea-498d-aac6-8fbf1fdec3da" />

#### Upload matt_metrobox.ino
Open matt_metrobox.ino and [upload the sketch](https://docs.arduino.cc/software/ide-v2/tutorials/getting-started/ide-v2-uploading-a-sketch/).
