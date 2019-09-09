#include <SPI.h>
#include <RF24.h>
#include <Ethernet.h>
#include <PubSubClient.h> // MQTT Bibliothek

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

IPAddress ip(192, 168, 1, 130);
IPAddress server(192, 168, 1, 5);

EthernetClient ethClient;
PubSubClient client(ethClient);

RF24 radio(7, 8); // CE, CSN
const byte address[6] = "00001";

void setup() {
  Serial.begin(115200);

  Serial.println("Init Ethernet");
  client.setServer(server, 1883);
  Ethernet.begin(mac, ip);

  Serial.println("Start Receiver");
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
}
void loop() {
  if (radio.available()) {
    char packet[17];
    radio.read(&packet, sizeof(packet));

    Serial.print("Received values from sensor: ");
    Serial.println(packet);
    
    char* SensorId = strtok(packet, ";");
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

void reconnect() {
  // Solange wiederholen bis Verbindung wiederhergestellt ist
  while (!client.connected()) {
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
    }
  }
}
