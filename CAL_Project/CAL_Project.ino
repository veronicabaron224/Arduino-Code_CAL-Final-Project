#include <WiFi.h>
#include "time.h"
#include "sntp.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2); // Change this to 0x27 if needed

const char *ssid = "LuvFam";
const char *password = "Qu13sc3nt@38!";

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 28800;
const int daylightOffset_sec = 3600;

const int buzzerPin = 32;

float temperature;

bool userTimeSet = false;
struct tm userTime;

static time_t lastDisplayTime = 0;

void printLocalTime()
{
  struct tm timeinfo;
  if (userTimeSet)
  {
    timeinfo = userTime;
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(&timeinfo, "%b %d %Y"); // Short month name, day, year

    lcd.setCursor(0, 1);
    lcd.print(&timeinfo, "%H:%M:%S"); 
  }
  else if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }

  if (timeinfo.tm_sec != lastDisplayTime)
  {
    lastDisplayTime = timeinfo.tm_sec;

    temperature = dht.readTemperature();

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(&timeinfo, "%b %d %Y"); // Short month name, day, year

    lcd.setCursor(0, 1);
    lcd.print(&timeinfo, "%H:%M:%S"); // Hour, minute, second

    lcd.setCursor(9, 1);
    lcd.print(temperature);
    lcd.print(" C");

    // Send temperature over serial
    Serial.print("T:");
    Serial.println(temperature);
  }
}

void timeAvailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void beepAlarm()
{
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

void setup()
{
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  dht.begin();

  // Setup LCD with backlight and initialize
  lcd.init();
  lcd.backlight();

  // Set notification callback function
  sntp_set_time_sync_notification_cb(timeAvailable);

  sntp_servermode_dhcp(1);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  // Connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  lcd.clear();
  lcd.print("Connecting to ");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  delay(500);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("CONNECTED");
  delay(500);
}

void loop()
{
  printLocalTime();

  if (Serial.available() > 0)
  {
    String receivedData = Serial.readStringUntil('\n');

    if (receivedData.startsWith("T:"))
    {
      receivedData.remove(0, 2);
      setTimeFromSerial(receivedData);
    }
  }

  if (Serial.available() > 0)
  {
    char receivedChar = Serial.read();
    if (receivedChar == 'A')
    {
      digitalWrite(buzzerPin, HIGH);
    }
    if (receivedChar == 'B')
    {
      digitalWrite(buzzerPin, LOW);
      delay(60000); // 1 minute
      digitalWrite(buzzerPin, HIGH);
    }
    if (receivedChar == 'C')
    {
      digitalWrite(buzzerPin, LOW);
    }
  }

  // if (temperature >= 40 || temperature <= 20)
  // {
  //   beepAlarm();
  // }

  delay(100); // Adjust the delay as needed
}

void setTimeFromSerial(String timeString)
{
  int day, month, year, hours, minutes, seconds;
  if (sscanf(timeString.c_str(), "%d/%d/%d %d:%d:%d", &day, &month, &year, &hours, &minutes, &seconds) == 6)
  {
    userTime.tm_year = year - 1900;  // Year offset from 1900
    userTime.tm_mon = month - 1;    // Month (0 - 11)
    userTime.tm_mday = day;         // Day of the month (1 - 31)
    userTime.tm_hour = hours;
    userTime.tm_min = minutes;
    userTime.tm_sec = seconds;

    // Set the time
    struct timeval tv = {.tv_sec = mktime(&userTime), .tv_usec = 0};
    settimeofday(&tv, NULL);

    // Print the updated time
    printLocalTime();
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed to parse time from the received string");
  }

  delay(100);
}