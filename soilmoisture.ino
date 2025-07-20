#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

// RS485 control pins
#define RE 8
#define DE 7

// SD card chip select pin
#define chipSelect 4

// RS485 request frame for soil sensor
const byte soilSensorRequest[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};
byte soilSensorResponse[9];

// RS485 communication via SoftwareSerial
SoftwareSerial mod(2, 3); // RO = D2, DI = D3

RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  mod.begin(4800);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  pinMode(10, OUTPUT); // Required for SD library

  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }

  File dataFile = SD.open("moisturenew.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println("Date,Time,Moisture (%)");
    dataFile.close();
  }
}

void loop() {
  // Enable RS485 transmission
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);

  // Send request to soil sensor
  mod.write(soilSensorRequest, sizeof(soilSensorRequest));

  // Switch to receive mode
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);
  delay(10);

  // Wait for response
  unsigned long startTime = millis();
  while (mod.available() < 9 && millis() - startTime < 1000) {
    delay(1);
  }

  if (mod.available() >= 9) {
    byte index = 0;
    while (mod.available() && index < 9) {
      soilSensorResponse[index] = mod.read();
      index++;
    }

    // Parse moisture value
    int Moisture_Int = int(soilSensorResponse[3] << 8 | soilSensorResponse[4]);
    float Moisture_Percent = Moisture_Int / 10.0;

    // Get current time
    DateTime now = rtc.now();

    // Print to Serial
    Serial.print("Time: ");
    Serial.print(now.timestamp(DateTime::TIMESTAMP_TIME));
    Serial.print(" | Moisture: ");
    Serial.print(Moisture_Percent);
    Serial.println(" %");

    // Log to SD card
    File dataFile = SD.open("moisture.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.print(now.year(), DEC);
      dataFile.print('/');
      dataFile.print(now.month(), DEC);
      dataFile.print('/');
      dataFile.print(now.day(), DEC);
      dataFile.print(',');
      dataFile.print(now.hour(), DEC);
      dataFile.print(':');
      dataFile.print(now.minute(), DEC);
      dataFile.print(':');
      dataFile.print(now.second(), DEC);
      dataFile.print(',');
      dataFile.println(Moisture_Percent);
      dataFile.close();
    }
  } else {
    Serial.println("Sensor timeout or incomplete frame");
  }

  delay(1000); // Log every second
}