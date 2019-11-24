#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp-knx-ip.h>
#include "WifiConfig.h"
extern "C"
{
#include "user_interface.h"
}

#define SCL_PIN SCL // SCL is Pin D1 on the board
#define SDA_PIN SDA // SDA/D4 is Pin D2 on the board
#define LED_PIN D2
Adafruit_BME280 bme280;

float bme280_temperature;
float bme280_humidity;
bool bme280_status;
// only for testing. in production the ESP will go to sleep
unsigned long lastGetI2CSensorTime = -5000ul;

// storage that survives deepsleep
// 1 byte:readCounter, 3 bytes:last temperature, 3 bytes:last humidity
#define rtcStorageLength 3
byte rtcStore[rtcStorageLength];

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Starting..."));

  Wire.begin(SDA_PIN, SCL_PIN);
  bme280_status = bme280.begin(0x76);
  if (!bme280_status)
  {
    Serial.println("Initial BME280...Error");
  }
  Serial.println("Initial BME280...Complete");
  Serial.println("Reading last values from RTC...");
  system_rtc_mem_read(64, rtcStore, rtcStorageLength);
}

void sendValues()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  Serial.println(F("Connected to WiFi"));
  // make TCP connections
}

bool valuesChanged()
{
  byte newValues[rtcStorageLength];
  bool valuesChanged = false;

  // increase read value
  int oldCount = int(rtcStore[0]);
  // send a value at least every hour to see the device is alive (15 min read interval * 4 = one hour)
  if (oldCount >= (int)4)
  {
    Serial.println("Values did not change for an hour. Enforcing sending values.");
    valuesChanged = true;
  }
  else
  {
    Serial.print("Read values counter: ");
    oldCount = oldCount + 1;
    newValues[0] = char(oldCount);
    Serial.println(oldCount);
  }

  // set current temperature with one decimal space cutting of the last digit
  unsigned int oldTemp = (int)rtcStore[1];
  unsigned int newTemp = (int)(bme280_temperature * 10);
  if (abs(oldTemp - newTemp) >= 2)
  {
    valuesChanged = true;
    Serial.println(String("  temperature changed from ") + String(oldTemp) + String(" to ") + String(newTemp));
    newValues[1] = char(newTemp);
  }
  else
  {
    newValues[1] = char(oldTemp);
  }

  // set current humidity with zero decimal space cutting of the last two digits
  unsigned int oldHum = (int)rtcStore[2];
  unsigned int newHum = (int)(bme280_humidity);
  if (abs(oldHum - newHum) >= 2)
  {
    valuesChanged = true;
    Serial.println(String("  humidity changed from ") + String(oldHum) + String(" to ") + String(newHum));
    newValues[2] = char(newHum);
  }
  else
  {
    newValues[2] = char(oldHum);
  }

  if (valuesChanged)
  {
    newValues[0] = char(0);
  }

  // always store values, as they contain the counter that changes every time
  for (size_t i = 0; i < rtcStorageLength; i++)
  {
    rtcStore[i] = newValues[i];
  }
  Serial.println("Storing values in RTC memory");
  system_rtc_mem_write(64, rtcStore, rtcStorageLength);

  return valuesChanged;
}

void loop()
{
  if (millis() - lastGetI2CSensorTime > 5000ul) // 5-Second
  {
    bme280_temperature = bme280.readTemperature(); // *C
    bme280_humidity = bme280.readHumidity();       // %RH

    Serial.print("BME280 Temperature = ");
    Serial.print(bme280_temperature);
    Serial.println(" *C");

    Serial.print("BME280 Humidity = ");
    Serial.print(bme280_humidity);
    Serial.println(" %");

    bool newValues = valuesChanged();
    if (newValues)
    {
      Serial.print("Values changed: ");
      Serial.println(newValues);
      Serial.println();
    }
    lastGetI2CSensorTime = millis();
  }

  delay(1);
  //Serial.println(F("Sleep......"));
  //ESP.deepSleep(10 * 60 * 1000000); //10 minutes
}
