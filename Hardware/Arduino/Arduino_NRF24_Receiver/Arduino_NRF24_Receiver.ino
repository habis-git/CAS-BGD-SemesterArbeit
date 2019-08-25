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
const uint8_t packetSize = 9;

char temperatureTopicPrefix[11] = "temperature";
char temperatureTopicName[sizeof(temperatureTopicPrefix) + sizeof(address) + 4];

char humidityTopicPrefix[11] = "humidity";
char humidityTopicName[sizeof(humidityTopicPrefix) + sizeof(address) + 4];

void setup() {
  Serial.begin(115200);

  Serial.println("Initialisiere Ethernet");
  client.setServer(server, 1883);
  Ethernet.begin(mac, ip);

  Serial.println("Starte Funkempfänger");
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  sprintf(temperatureTopicName, "/%s/%s/", temperatureTopicPrefix, address);
  Serial.print("Sende Temperatur Daten an das Topic ");
  Serial.println(temperatureTopicName);

  sprintf(humidityTopicName, "/%s/%s/", humidityTopicPrefix, address);
  Serial.print("Sende Luftfeuchtigkeit Daten an das Topic ");
  Serial.println(humidityTopicName);
}
void loop() {
  if (radio.available()) {
    char packet[7];
    radio.read(&packet, packetSize);

    Serial.print("Werte empfangen: ");
    Serial.println(packet);

    if (!client.connected()) {
      reconnect();
    }
    char* Temperature = strtok(packet, ";");
    if (client.connected()) {
      client.publish(temperatureTopicName, Temperature);
    }
    char* Humidity = strtok(NULL, ";");
    if (client.connected()) {
      client.publish(humidityTopicName, Humidity);
    }
  }
}

void reconnect() {
  // Solange wiederholen bis Verbindung wiederhergestellt ist
  while (!client.connected()) {
    Serial.print("Versuch des MQTT Verbindungsaufbaus...");
    //Versuch die Verbindung aufzunehmen
    if (client.connect("arduinoClient")) {
      Serial.println("Erfolgreich verbunden!");
      // Nun versendet der Arduino eine Nachricht in outTopic ...
      client.publish("test", "Arduino nach Haue telefonieren");
    } else {
      Serial.print("Fehler, rc=");
      Serial.print(client.state());
      Serial.println(" Nächster Versuch in 5 Sekunden");
      // 5 Sekunden Pause vor dem nächsten Versuch
      delay(5000);
    }
  }
}
