#include <SPI.h>
#include "RF24.h"
#include "DHT.h"
/****************** User Config ***************************/
// Uncomment one of the lines below for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// DHT Sensor
uint8_t DHTPin = 27;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(25, 26); //CE, CSN

const byte address[6] = "00001";

float Temperature;
float Humidity;
const uint8_t packetSize = sizeof(float) + 1 + sizeof(float);

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

/**********************************************************/

// Used to control whether this node is sending or receiving
void setup() {
  pinMode(DHTPin, INPUT);
  dht.begin();

  Serial.begin(115200);
  Serial.println(F("RF24/examples/GettingStarted"));

  if (radio.begin()) {
    Serial.println("Alles Super");
  }
  else {
    Serial.println("Nicht gut");
  }
  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(address);
}
void loop() {
  Temperature = dht.readTemperature(); // Gets the values of the temperature
  Humidity = dht.readHumidity(); // Gets the values of the humidity

  char packet[packetSize];
  sprintf(packet, "%.1f;%.1f", Temperature, Humidity);

  Serial.println(F("Now sending: "));
  Serial.println(packet);

  if (!radio.write( &packet, packetSize )) {
    Serial.println(F("failed"));
  }
 // Try again 1s later
  delay(5000);
}
