#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

// My cool ambilight sketch

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
    if (!topic || !payload)
    {
        Serial.println("Invalid argument (nullpointer) given!");
    }
    else
    {
        Serial.printf("Message arrived on channel: %s\n", topic);

        /*
         * Example payload:
         * 0AABBCCDDEEFF
         *
         * Scene effect: 0
         * Color 1 (as hexadecimal RGB value): AABBCC
         * Color 2 (as hexadecimal RGB value): DDEEFF
         * Brightness (as hexadecimal RGB value): GG
         *
         */
        const char* payloadAsCharPointer = (char*) payload;

        
        const char lightScene = payloadAsCharPointer[0];
        const uint32_t color1 = _getRGBColorFromPayload(payloadAsCharPointer, 1);
        const uint32_t color2 = _getRGBColorFromPayload(payloadAsCharPointer, 7);
        const uint8_t brightness = _getBrightnessFromPayload(payloadAsCharPointer, 13);
        
        Serial.printf("Light scene: %c\n", lightScene);
        Serial.println();
        Serial.printf("Color 1: %06X\n", color1);
        Serial.println();
        Serial.printf("Color 2: %06X\n", color2);
        Serial.println();
        Serial.printf("Brightness: %02X\n", brightness);
        Serial.println();
        
        showScene(lightScene, color1, color2, brightness);
    }
}

void neopixelFullOn() {
  for (uint16_t i = 0; i < neopixelStrip.numPixels(); i++)
    {
        neopixelStrip.setPixelColor(i, 0xFFFFFF);
    }
    neopixelStrip.setBrightness(255);
    neopixelStrip.show();
}


/*
 * Neopixel effects
 *
 */

void neopixel_off()
{
    for (uint16_t i = 0; i < neopixelStrip.numPixels(); i++)
    {
        neopixelStrip.setPixelColor(i, 0);
    }

    neopixelStrip.show();
}


void neopixel_setBrightness(uint8_t brightness)
{
    for (uint16_t i = 0; i < neopixelStrip.numPixels(); i++)
    {
        neopixelStrip.setPixelColor(i, brightness);
    }
}

void neopixel_showSingleColorScene(const uint32_t color, const uint8_t brightness)
{
    neopixelStrip.setBrightness(brightness);
    
    for (uint16_t i = 0; i < neopixelStrip.numPixels(); i++)
    {
        neopixelStrip.setPixelColor(i, color);
    }

    neopixelStrip.show();
}

void neopixel_showMixedColorScene(const uint32_t color1, const uint32_t color2)
{
    const uint16_t neopixelCount = neopixelStrip.numPixels();
    const uint16_t neopixelCountCenter = neopixelCount / 2;

    for (uint16_t i = 0; i < neopixelCountCenter; i++)
    {
        neopixelStrip.setPixelColor(i, color1);
    }

    for (uint16_t i = neopixelCountCenter; i < neopixelCount; i++)
    {
        neopixelStrip.setPixelColor(i, color2);
    }

    neopixelStrip.show();
}

void neopixel_showGradientScene(const uint32_t color1, const uint32_t color2)
{
    const uint16_t neopixelCount = neopixelStrip.numPixels();

    // Split first color to R, B, G parts
    const uint8_t color1_r = (color1 >> 16) & 0xFF;
    const uint8_t color1_g = (color1 >>  8) & 0xFF;
    const uint8_t color1_b = (color1 >>  0) & 0xFF;

    // Split second color to R, B, G parts
    const uint8_t color2_r = (color2 >> 16) & 0xFF;
    const uint8_t color2_g = (color2 >>  8) & 0xFF;
    const uint8_t color2_b = (color2 >>  0) & 0xFF;

    for(uint16_t i = 0; i < neopixelCount; i++)
    {
        float percentage = _mapPixelCountToPercentage(i, neopixelCount);

        // Calculate the color of this iteration
        // see: https://stackoverflow.com/questions/27532/ and https://stackoverflow.com/questions/22218140/
        const uint8_t r = (color1_r * percentage) + (color2_r * (1 - percentage));
        const uint8_t g = (color1_g * percentage) + (color2_g * (1 - percentage));
        const uint8_t b = (color1_b * percentage) + (color2_b * (1 - percentage));

        const uint32_t currentColor = neopixelStrip.Color(r, g, b);
        neopixelStrip.setPixelColor(i, currentColor);
    }

    neopixelStrip.show();
}

float _mapPixelCountToPercentage(uint16_t i, float count)
{
    const float currentPixel = (float) i;
    const float neopixelCount = (float) count;

    const float min = 0.0f;
    const float max = 1.0f;

    return (currentPixel - 0.0f) * (max - min) / (neopixelCount - 0.0f) + min;
}

void showScene(const char lightScene, const uint32_t color1, const uint32_t color2, const uint8_t brightness)
{
    switch(lightScene)
    {
        case '0':
        {
            neopixel_off();
        }
        break;

        case '1':
        {
            neopixel_showSingleColorScene(color1, brightness);
        }
        break;

        case '2':
        {
            neopixel_showMixedColorScene(color1, color2);
        }
        break;

        case '3':
        {
            neopixel_showGradientScene(color1, color2);
        }
        break;
    }
}


uint8_t _getBrightnessFromPayload(const char* payload, const uint16_t startPosition)
{
    uint8_t brightness = 0x000000;

    if (!payload)
    {
        Serial.println("Invalid argument (nullpointer) given!");
    }
    else
    {
        // Pre-initialized char array (length = 7) with terminating null character:
        char brightnessString[3] = { '0', '0', '\0' };
      
        strncpy(brightnessString, payload + startPosition, 2);

        // Convert hexadecimal RGB color strings to decimal integer
        const uint8_t convertedBrightness = strtol(brightnessString, NULL, 16);

        // Verify that the given color values are in a valid range
        if ( convertedBrightness >= 0x00 && convertedBrightness <= 0xFF )
        {
            brightness = convertedBrightness;
        } else {
            Serial.println("Wrong convertedBrightness range");
        }
    }

    return brightness;
}

uint32_t _getRGBColorFromPayload(const char* payload, const uint8_t startPosition)
{
    uint32_t color = 0x000000;

    if (!payload)
    {
        Serial.println("Invalid argument (nullpointer) given!");
    }
    else
    {
        // Pre-initialized char array (length = 7) with terminating null character:
        char rbgColorString[7] = { '0', '0', '0', '0', '0', '0', '\0' };
        strncpy(rbgColorString, payload + startPosition, 6);

        // Convert hexadecimal RGB color strings to decimal integer
        const uint32_t convertedRGBColor = strtol(rbgColorString, NULL, 16);

        // Verify that the given color values are in a valid range
        if ( convertedRGBColor >= 0x000000 && convertedRGBColor <= 0xFFFFFF )
        {
            color = convertedRGBColor;
        }
    }

    return color;
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
    neopixel_showSingleColorScene(0, 0);
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
