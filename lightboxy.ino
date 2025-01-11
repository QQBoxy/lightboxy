#include <ESP8266WiFi.h>
#include "PubSubClient.h"

const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
const char *mqtt_server = "YOUR_MQTT_SERVER";
const char *mqtt_name = "YOUR_MQTT_SERVER_NAME";
const char *mqtt_password = "YOUR_MQTT_SERVER_PASSWORD";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg_power[MSG_BUFFER_SIZE];
char msg_brightness[MSG_BUFFER_SIZE];
int power = 0;
int brightness = 255;

void setup_wifi()
{

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if (strcmp(topic, "lightboxy/switch/inTopic") == 0)
  {
    // Power
    power = atoi((char *)payload);
    Serial.print("Power: ");
    if (power == 1)
    {
      analogWrite(0, brightness);
      analogWrite(LED_BUILTIN, 255 - brightness);
      Serial.print("1 (ON)");
    }
    else
    {
      digitalWrite(0, LOW);
      digitalWrite(LED_BUILTIN, HIGH);
      power = 0;
      Serial.print("0 (OFF)");
    }
    Serial.println();
    snprintf(msg_power, MSG_BUFFER_SIZE, "%ld", power);
  }
  else if (strcmp(topic, "lightboxy/brightness/inTopic") == 0)
  {
    // Brightness
    // Convert payload to number
    if (length >= 128)
    {
      return;
    }
    char buffer[129];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    char *endptr = nullptr;
    long num = strtol(buffer, &endptr, 10);
    if (*endptr != '\0')
    {
      return;
    }
    // Check if number is between 0 and 255
    if (num >= 0 && num <= 255)
    {
      brightness = (int)num;
      Serial.print("Brightness: ");
      Serial.print(brightness);
      Serial.print(" (");
      Serial.print(brightness * 100 / 255);
      Serial.print(" %)");
      Serial.println();
    }
    else
    {
      Serial.print("Brightness out of range!");
      Serial.println();
    }

    if (power == 1)
    {
      analogWrite(0, brightness);
      analogWrite(LED_BUILTIN, 255 - brightness);
    }
    snprintf(msg_brightness, MSG_BUFFER_SIZE, "%ld", brightness);
  }
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_name, mqtt_password))
    {
      Serial.println("connected");
      client.publish("lightboxy/switch/outTopic", "0");
      client.publish("lightboxy/brightness/outTopic", "255");
      client.subscribe("lightboxy/switch/inTopic");
      client.subscribe("lightboxy/brightness/inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup()
{
  pinMode(0, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop()
{

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000)
  {
    lastMsg = now;
    client.publish("lightboxy/switch/outTopic", msg_power);
    client.publish("lightboxy/brightness/outTopic", msg_brightness);
  }
}