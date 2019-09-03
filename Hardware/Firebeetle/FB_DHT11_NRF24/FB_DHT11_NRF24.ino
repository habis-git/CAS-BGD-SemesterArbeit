#include <SPI.h>
#include "RF24.h"
#include "DHT.h"
#include "SPIFFS.h"
#include "Settings.h"

// DHT Sensor
uint8_t DHTPin = 27;
// Moisture Sensor
uint8_t MOISTSENSOR_PIN = 36;

byte address[6] = "00000";
int paLevel;

float Temperature;
float Humidity;
int Moisture;
const uint8_t packetSize = sizeof(float) + 1 + sizeof(float) + 1 + sizeof(int);

// set default sleep time to one minute
int sleepTime = 60000;

uint8_t dhtType;

DHT *dht;
RF24 *radio;

void setSettingsValue(String argument, String value) {
  if (argument.indexOf ( "sensorType" ) >= 0){ 
    if(value == "DHT11"){
      dhtType = DHT11;
    }else if(value == "DHT21"){
      dhtType = DHT21;
    }else if(value == "DHT22"){
      dhtType = DHT22;
    }
  }

  if (argument.indexOf("sleepTime") >= 0){
    sleepTime = argument.toInt();
  }

  if (argument.indexOf ( "paLevel" ) >= 0){ 
    if(value == "low"){
      paLevel = RF24_PA_LOW;
    }else if(value == "med"){
      paLevel = RF24_PA_HIGH;
    }else if(value == "high"){
      paLevel = RF24_PA_MAX;
    }
  }
}

// Used to control whether this node is sending or receiving
void setup() {
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
    Serial.print("Address of this device is:");
    Serial.println(line.c_str());
  }
  idFile.close() ;
  
  /* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 25 & 26 */
  radio = new RF24(25, 26); //CE, CSN

  // Initialize DHT sensor.
  dht = new DHT(DHTPin, dhtType);

  pinMode(DHTPin, INPUT);
  pinMode(MOISTSENSOR_PIN, INPUT);
  dht->begin();

  Serial.begin(115200);
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
  sprintf(packet, "%.1f;%.1f;%i", Temperature, Humidity, Moisture);

  Serial.println(F("Now sending: "));
  Serial.println(packet);

  if (!radio->write( &packet, packetSize )) {
    Serial.println(F("failed"));
  }
  // Try again 1s later
  delay(sleepTime);
}
