#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define PULSE_SENSOR_PIN 35
#define LANDMINE_SENSOR_PIN 32
#define VIBRATION_MOTOR_PIN 26

#define ss 5
#define rst 14
#define dio0 2

char ssid[] = "Varad";
char pass[] = "varad@3456";
const char* serverURL = "http://192.168.89.172:8080/insert";

int heartRate = 0;
float temperature = 0.0;
bool landmine = false;
bool loraReady = false;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  dht.begin();

  LoRa.setPins(ss, rst, dio0);

  int retryCount = 0;
  while (!LoRa.begin(433E6) && retryCount < 1) {
    Serial.println("LoRa init failed. Retrying...");
    delay(2000);
    retryCount++;
  }

  if (retryCount >= 5) {
    Serial.println("LoRa not available, continuing without it.");
    loraReady = false;
  } else {
    LoRa.setSyncWord(0xA5);
    Serial.println("LoRa Initialized Successfully!");
    loraReady = true;
  }

  pinMode(VIBRATION_MOTOR_PIN, OUTPUT);
  digitalWrite(VIBRATION_MOTOR_PIN, LOW);
}

void receiveLoRaMessage() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("Received Message: ");
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    Serial.println();

    digitalWrite(VIBRATION_MOTOR_PIN, HIGH);
    delay(1000);
    digitalWrite(VIBRATION_MOTOR_PIN, LOW);
  }
}

void sendDataOverWiFi(float temp, int hr, bool landmine) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverURL) + "?temperature=" + String(temp) + "&pulse=" + String(hr) + "&landmine=" + String(landmine) + "&key=123@";
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("HTTP Response: ");
      Serial.println(response);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void loop() {
  if (loraReady) {
    receiveLoRaMessage();
  }

  int landmineValue = touchRead(LANDMINE_SENSOR_PIN);
  if (landmineValue < 20) {
    Serial.println("⚠️ ALERT: Possible Landmine Detected!");
    landmine = true;
    if (loraReady) {
      LoRa.beginPacket();
      LoRa.print("⚠️ Landmine Detected!");
      LoRa.endPacket();
    }
    digitalWrite(VIBRATION_MOTOR_PIN, HIGH);
    delay(2000);
    digitalWrite(VIBRATION_MOTOR_PIN, LOW);
  } else {
    landmine = false;
  }

  temperature = dht.readTemperature();
  heartRate = analogRead(PULSE_SENSOR_PIN);
  heartRate = map(heartRate, 0, 4095, 60, 120);

  if (isnan(temperature)) {
    Serial.println("Failed to read temperature!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Heart Rate: ");
  Serial.print(heartRate);
  Serial.println(" BPM");

  if (loraReady) {
    LoRa.beginPacket();
    LoRa.print("Temp:");
    LoRa.print(temperature);
    LoRa.print(", HR:");
    LoRa.print(heartRate);
    LoRa.endPacket();
  }

  if (temperature > 38.0 || heartRate > 110 || heartRate < 50) {
    Serial.println("⚠️ ALERT: Abnormal Health Readings!");
    if (loraReady) {
      LoRa.beginPacket();
      LoRa.print("ALERT: Temp:");
      LoRa.print(temperature);
      LoRa.print(", HR:");
      LoRa.print(heartRate);
      LoRa.endPacket();
    }
    digitalWrite(VIBRATION_MOTOR_PIN, HIGH);
    delay(1000);
    digitalWrite(VIBRATION_MOTOR_PIN, LOW);
  }

  sendDataOverWiFi(temperature, heartRate, landmine);
  delay(5000);
}
