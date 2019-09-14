#include <SPI.h>
#include <RF24.h>
#include <Ethernet.h>
#include <PubSubClient.h> // MQTT Bibliothek
#include "sha256.h"
#include "keyFile.h"

#define NODEBUG

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

uint8_t hmacSize = 32;

IPAddress ip(192, 168, 1, 130);
IPAddress server(192, 168, 1, 5);

EthernetClient ethClient;
PubSubClient client(ethClient);

RF24 radio(7, 8); // CE, CSN
const byte address[6] = "00001";

byte received[90];             //data holder for received data
bool allReceived;

char *key;
int keyLength;

char* convert_hmac_to_string(uint8_t* hash, int hmacSize) {
  char resultArray[hmacSize * 2];
  for (int cnt = 0; cnt < hmacSize; cnt++)
  {
    sprintf(&resultArray[cnt * 2], "%02X", hash[cnt]);
  }
  return resultArray;
}

void setup() {
  Serial.begin(115200);

  //Serial.print("hmac key: ");
  //Serial.println(key);

  int key_len = key_str.length() + 1; 
    
  keyLength = key_len;
  key = (char*) malloc(key_len * sizeof(char));
  // Copy it over 
  key_str.toCharArray(key, key_len);

  Serial.println("Init Ethernet");
  client.setServer(server, 1883);
  Ethernet.begin(mac, ip);

  Serial.println("Start Receiver");
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.setRetries(15, 15);
  radio.startListening();

  radio.printDetails(); //Debug

  //Variable Initialization
  allReceived = false;
}

void loop() {
  //Variable Initialization
  allReceived = false;

  if (radio.available()) {
    bool done = false;
    while (!done && !allReceived) //If everything isnt received and not doen receiving
    {
      byte data[32];
      if (!allReceived && radio.available()) {
        radio.read( &data, sizeof(data) );//place received data in byte structure
        byte pkt = data[0];  //part of header, packet number received

#ifdef DEBUG
        Serial.print("Packet: ");
        Serial.print(pkt);
        Serial.print(" of ");
        Serial.println(data[1]);
#endif
        for (int i = 2; i < 32; i++) {
#ifdef DEBUG
          Serial.print(i);
          Serial.print(" = ");
          Serial.println(data[i]);
          Serial.println((pkt - 1) * 30 + (i - 2));
#endif
          received[(pkt - 1) * 30 + (i - 2)] = data[i];
        }
        delay(20);
        radio.stopListening();    //stop listening to transmit response of packet received.
        bool ok = radio.write(&pkt, sizeof(pkt));  //send packet number back to confirm
        if (ok) {
          Serial.print("Done Sending Response of packet:");
          Serial.println(pkt);
        }
        if (pkt == data[1]) { //if packet received == total number of packets
          allReceived = true;
          Serial.println("got all the packets\n");
        }
        Serial.println("Done Receiving");
        radio.startListening();
      }
    }

    Serial.print("Received values from sensor: ");
    Serial.println((char*)received);

    char* hmac = strtok(received, ":");
    char* data = strtok(NULL, ":");
    Serial.print("data: ");
    Serial.println(data);
    Serial.print("Received   hmac: ");
    Serial.println(hmac);

    Sha256.initHmac(key, keyLength);
    Sha256.print(data);
    uint8_t* calculatedHmac = Sha256.resultHmac();
    Serial.print("Calculated hmac: ");
    char calculatedHmac_char[hmacSize * 2];
    for (int cnt = 0; cnt < hmacSize; cnt++)
    {
      // convert byte to its ascii representation
      sprintf(&calculatedHmac_char[cnt * 2], "%02X", calculatedHmac[cnt]);
    }
    Serial.println(calculatedHmac_char);

    uint8_t authorizationCheck = strcmp(calculatedHmac_char, hmac);
    Serial.print("Authorization Check: ");
    Serial.println(authorizationCheck);

    if (authorizationCheck != 0) {
      Serial.println("Hmac not matching, not authorized or tampered message! Message rejected");
    } else {
      char* SensorId = strtok(data, ";");
      char* Temperature = strtok(NULL, ";");
      char* Humidity = strtok(NULL, ";");
      char* SoilHumidity = strtok(NULL, ";");

      char json[100];
      sprintf(json, "{\"ID\": \"%s\", \"Temp.Air\": %s, \"Hum.Air\": %s, \"Hum.Soil\": %s}", SensorId, Temperature, Humidity, SoilHumidity);
      if (!client.connected()) {
        reconnect();
      }
      if (client.connected()) {
        Serial.print("Send data to topic '/weather/' ");
        client.publish("/weather/", json);
        Serial.println(json);
      }
    }
  }
}

void reconnect() {
  // Solange wiederholen bis Verbindung wiederhergestellt ist
  int i = 0;
  while (!client.connected() && i < 5) {
    Serial.print("Connecting to MQTT Server");
    //Versuch die Verbindung aufzunehmen
    if (client.connect("arduinoClient")) {
      Serial.println("Successful connected!");
    } else {
      Serial.print("Error, rc=");
      Serial.print(client.state());
      Serial.println(" Next try in 5 seconds");
      // 5 Sekunden Pause vor dem nächsten Versuch
      delay(5000);
      i++;
    }
  }
}
