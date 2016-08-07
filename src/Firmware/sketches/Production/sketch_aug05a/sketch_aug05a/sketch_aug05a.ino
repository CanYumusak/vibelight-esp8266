#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

const char* ssid = "KabelSalat";
const char* password = "DerMitDemLangenKabel";

char* out_topic = "lightcontrol/in_channel";
char* in_topic = "lightcontrol/out_channel";

char* server = "iot.eclipse.org";


#define PIN_NEOPIXELS               D1       // GPIO5 = D1
#define NEOPIXELS_COUNT             40
#define NEOPIXELS_BRIGHTNESS        255     // [0..255]


#define PIN_STATUSLED               LED_BUILTIN

void callback(char* topic, byte* payload, unsigned int length) {
  handleIncomingMessage(topic, payload, length);
}


WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);
Adafruit_NeoPixel neopixelStrip = Adafruit_NeoPixel(NEOPIXELS_COUNT, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);


void handleIncomingMessage(char* topic, byte* payload, unsigned int length) {
  client.publish(out_topic, "Got a callback with payload");
  neopixel_showSingleColorScene(0x0000FF);
}

void neopixel_showSingleColorScene(const uint32_t color)
{
    for (uint16_t i = 0; i < neopixelStrip.numPixels(); i++)
    {
        neopixelStrip.setPixelColor(i, color);
    }

    neopixelStrip.show();
    Serial.println("Shown neopixel");
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Generate client name based on MAC address and last 8 bits of microsecond counter
  String clientName = "bedlights";
  Serial.print("Connecting to ");
  Serial.print(server);
  Serial.print(" as ");
  Serial.println(clientName);
  neopixelStrip.begin();
  neopixelStrip.setBrightness(0xFF);
  if (client.connect((char*) clientName.c_str())) {
    Serial.println("Connected to MQTT broker");
    
    if (client.publish(out_topic, "hello from ESP8266")) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }

    if (client.subscribe(in_topic)) {
      Serial.println("Subscribe ok");
    } else {
      Serial.println("Subscribe failed");
    }
    neopixel_showSingleColorScene(0);
  }
  else {
    Serial.println("MQTT connect failed");
    Serial.println("Will reset and try again...");
    abort();
  }
}


void blinkStatusLED(const int times)
{
    for (int i = 0; i < times; i++)
    {
        // Enable LED
        digitalWrite(PIN_STATUSLED, LOW);
        delay(100);

        // Disable LED
        digitalWrite(PIN_STATUSLED, HIGH);
        delay(100);
    }
}

void loop() {
  client.loop();
}
