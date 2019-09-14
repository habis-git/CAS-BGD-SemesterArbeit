#include <SPI.h>
#include "RF24.h"
#include "DHT.h"
#include "SPIFFS.h"
#include "Settings.h"
#include "mbedtls/md.h"

#define NODEBUG

// DHT Sensor
uint8_t DHTPin = 27;
// Moisture Sensor
uint8_t MOISTSENSOR_PIN = 36;

byte address[6] = "00000";
char *key;
int keyLength;
int paLevel;

float Temperature;
float Humidity;
int Moisture;
const uint8_t packetSize = 6 + sizeof(float) + 1 + sizeof(float) + 1 + sizeof(int);
uint8_t hmacSize = 32;

// set default sleep time to one minute
int sleepTime = 60000;

uint8_t dhtType;

byte sending[32];
byte source_data[52];
int pkt, sizeArray, total_pkts;
bool confirmed, sendArray;

DHT *dht;
RF24 *radio;

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
    Serial.println("Failed to open id file for reading");
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
    Serial.println("Failed to open key file for reading");
    return;
  }

  String keyLine;
  while (keyFile.available())
  {
    String keyString = keyFile.readStringUntil ( '\n' );
    // Length (with one extra character for the null terminator)
    int key_len = keyString.length() + 1; 
    
    keyLength = key_len;
    key = (char*) malloc(key_len * sizeof(char));
    // Copy it over 
    keyString.toCharArray(key, key_len);
    Serial.print("key: "),
    Serial.println(key);
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
  radio->setPayloadSize(128);
  radio->openReadingPipe(1, address); //Open Reading pipe
  radio->openWritingPipe(address);
}

void create_hmac(char *payload, byte *hmacResult, char* key) {
  Serial.print("key in hmac: ");
  Serial.println(key);
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  const size_t payloadLength = strlen(payload);
  const size_t keyLength = strlen(key);

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *) key, keyLength);
  mbedtls_md_hmac_update(&ctx, (const unsigned char *) payload, payloadLength);
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);
}

void loop() {
  Temperature = dht->readTemperature(); // Gets the values of the temperature
  Humidity = dht->readHumidity(); // Gets the values of the humidity

  Moisture = analogRead(MOISTSENSOR_PIN);

  char packet[packetSize];
  sprintf(packet, "%s;%.1f;%.1f;%i", address, Temperature, Humidity, Moisture);
  Serial.print("packet: ");
  Serial.println(packet);
  
  byte hmacResult[hmacSize];
  create_hmac(packet, hmacResult, key);
  char authenticatedMsg[packetSize + sizeof(hmacResult)];

  char hexHmac[hmacSize * 2];
  for (int cnt = 0; cnt < hmacSize; cnt++)
  {
    // convert byte to its ascii representation
    sprintf(&hexHmac[cnt * 2], "%02X", hmacResult[cnt]);
  }

  sprintf(authenticatedMsg, "%s:%s", hexHmac, packet);

  Serial.println(F("Hashed message: "));
  Serial.println(authenticatedMsg);

  sendArray = true;
  confirmed = true;
  sizeArray = strlen(authenticatedMsg);  //total length of array
#ifdef DEBUG
  Serial.print ("Sizearray: ");
  Serial.println(sizeArray);
#endif
  total_pkts = sizeArray / 30;
  if(sizeArray % 30 != 0){
    total_pkts++;
  }

  // from https://forum.arduino.cc/index.php?topic=233551.0
  if (sendArray)
  {
    while ((pkt < total_pkts) && (confirmed)) { //packet place holder
      radio->stopListening();      //stop listening to talk
      if (confirmed) {             //if response confirming it obtained the previous packet, continue
        pkt++;
        //Header for each packet
        sending[0] = pkt;                 //Header for packet number
        sending[1] = total_pkts;   //total packets sent
#ifdef DEBUG
        Serial.print ("Packet: ");
        Serial.print(pkt);
        Serial.print(" of ");
        Serial.println(sending[1]);
#endif
        for (int i = 0; i < 30; i++) {    //Next 30 bytes to fill
          sending[i + 2] = authenticatedMsg[i + ((pkt - 1) * 30)]; //shift sending by 2 (Header) to start at 3rd position
          //IRsignal -> shift packetnumber*30 for last packet left off
#ifdef DEBUG
          Serial.print(i + 2);
          Serial.print(" = ");
          Serial.println(sending[i + 2]);
#endif
        }
        confirmed = false;
        Serial.print("Sending packet of size: ");  //make sure 32 bytes or less
        Serial.println(sizeof(sending));

        bool ok = radio->write( &sending, sizeof(sending));    //Sends first packet to radio

        if (ok) {
          Serial.print("Packet Sent: ");
          Serial.println(pkt);
        }
        else {
          printf("failed.\n\r");
        }

        byte packetReceived = 0;  //packetReceived (feedback) = 0th packet

        radio->startListening();    //start listening for a response
        delay(400);
        radio->read(&packetReceived, sizeof(packetReceived));    //save response

#ifdef DEBUG
        Serial.print("Packet sent: ");
        Serial.print(pkt);
        Serial.print(" PacketReceived: ");
        Serial.println(packetReceived);
#endif
        if (packetReceived == pkt) {  //If packet number received == packet sent
          confirmed = true;
          Serial.println("Packet was confirmed, proceed to send next packet.");
        }
        else {
          confirmed = false;
          sendArray = false;
          Serial.println("Either nothing sent/no confirmation/Mismatch of packets (resend??)");
        }
      }
    }
  }

  sendArray = true;//Reset Variables
  pkt = 0;
  confirmed = true;

  // Try again 1s later
  delay(sleepTime);
}
