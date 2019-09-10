#include <SPI.h>
#include "RF24.h"
#include "DHT.h"
#include "SPIFFS.h"
#include "Settings.h"
#include "Encryption.h"

// DHT Sensor
uint8_t DHTPin = 27;
// Moisture Sensor
uint8_t MOISTSENSOR_PIN = 36;

byte address[6] = "00000";
int paLevel;

float Temperature;
float Humidity;
int Moisture;
const uint8_t packetSize = 6 + sizeof(float) + 1 + sizeof(float) + 1 + sizeof(int);

// set default sleep time to one minute
int sleepTime = 60000;

uint8_t dhtType;

DHT *dht;
RF24 *radio;
Encryption *encrypter;

void setSettingsValue(String argument, String value) {
  if (argument.indexOf ( "sensorType" ) >= 0) {
    if (value == "DHT11") {
      dhtType = DHT11;
      Serial.println("DHT Sensor Type is DHT11");
    } else if (value == "DHT21") {
      dhtType = DHT21;
      Serial.println("DHT Sensor Type is DHT21");
    } else if (value == "DHT22") {
      dhtType = DHT22;
      Serial.println("DHT Sensor Type is DHT22");
    }
  }

  if (argument.indexOf("sleepTime") >= 0) {
    sleepTime = value.toInt();
    Serial.print("Sleep Time is ");
    Serial.println(value);
  }

  if (argument.indexOf ( "paLevel" ) >= 0) {
    if (value == "low") {
      paLevel = RF24_PA_LOW;
      Serial.println("PALevel is LOW");
    } else if (value == "med") {
      paLevel = RF24_PA_HIGH;
      Serial.println("PALevel is HIGH");
    } else if (value == "high") {
      paLevel = RF24_PA_MAX;
      Serial.println("PALevel is MAX");
    }
  }
}

// Used to control whether this node is sending or receiving
void setup() {
  Serial.begin(115200);

  // read settings
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  File idFile = SPIFFS.open("/id.txt");
  if (!idFile) {
    Serial.println("Failed to open file for reading");
    return;
  }

  String line;
  while (idFile.available())
  {
    line = idFile.readStringUntil ( '\n' ) ;
    line.getBytes(address, sizeof(address));
    Serial.print("Address of this device is ");
    Serial.println(line.c_str());
  }
  idFile.close() ;

  File keyFile = SPIFFS.open("/key");
  if (!keyFile) {
    Serial.println("Failed to open keyFile for reading");
    return;
  }

  String keyLine;
  while (keyFile.available())
  {
    keyLine = keyFile.readStringUntil ( '\n' ) ;
    byte key[16];
    keyLine.getBytes(key, sizeof(key));
    encrypter = new Encryption(key, sizeof(key));
    Serial.print("key of this device is ");
    Serial.println(keyLine.c_str());
  }
  keyFile.close() ;

  readSettings();

  /* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 25 & 26 */
  radio = new RF24(25, 26); //CE, CSN

  // Initialize DHT sensor.
  dht = new DHT(DHTPin, dhtType);

  pinMode(DHTPin, INPUT);
  pinMode(MOISTSENSOR_PIN, INPUT);
  dht->begin();

  Serial.println(F("Temperature/Moisture measurement started"));

  if (radio->begin()) {
    Serial.println("Radio device ready");
  }
  else {
    Serial.println("Radio device not ready!");
  }
  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio->setPALevel(paLevel);
  radio->openWritingPipe(address);
}
void loop() {
  Temperature = dht->readTemperature(); // Gets the values of the temperature
  Humidity = dht->readHumidity(); // Gets the values of the humidity

  Moisture = analogRead(MOISTSENSOR_PIN);

  char packet[packetSize];
  sprintf(packet, "%s;%.1f;%.1f;%i", address, Temperature, Humidity, Moisture);

  String msg = encrypter->encrypt(packet);

  Serial.println(F("Now sending: "));
  Serial.println(packet);

  if (!radio->write( &packet, packetSize )) {
    Serial.println(F("failed"));
  }
  // Try again 1s later
  delay(sleepTime);
}
