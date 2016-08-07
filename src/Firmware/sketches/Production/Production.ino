#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
    #include <avr/power.h>
#endif


/*
 * Connection configuration
 *
 */
const char* wifiSsid = "KabelSalat";
const char* wifiPassword = "DerMitDemLangenKabel";

char* mqttClientId = "Vibelight Device 1.0 xxxxxxxxxxxxx";
const char* mqttServer = "iot.eclipse.org";
const uint8_t mqttPort = 1883;

char* out_topic = "lightcontrol/in_channel";
char* in_topic = "lightcontrol/out_channel";


// Try to connect N times and reset chip if limit is exceeded
// #define CONNECTION_RETRIES          3

#define PIN_STATUSLED               LED_BUILTIN

#define PIN_NEOPIXELS               D1       // GPIO5 = D1
#define NEOPIXELS_COUNT             40
#define NEOPIXELS_BRIGHTNESS        255     // [0..255]


#define EEPROM_ADDRESS_LIGHTSCENE   0
#define EEPROM_ADDRESS_COLOR1       1
#define EEPROM_ADDRESS_COLOR2       4

void callback(char* topic, byte* payload, unsigned int length) {
  _MQTTRequestCallback(topic, payload, length);
}


WiFiClient wifiClient;
PubSubClient client(mqttServer, mqttPort, callback, wifiClient);
Adafruit_NeoPixel neopixelStrip = Adafruit_NeoPixel(NEOPIXELS_COUNT, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

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

void neopixel_showSingleColorScene(const uint32_t color)
{
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

void showScene(const char lightScene, const uint32_t color1, const uint32_t color2)
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
            neopixel_showSingleColorScene(color1);
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

void saveCurrentScene(const char lightScene, const uint32_t color1, const uint32_t color2)
{
    _setSceneEffectToEEPROM(lightScene);

    _setRGBColorToEEPROM(color1, EEPROM_ADDRESS_COLOR1);
    _setRGBColorToEEPROM(color2, EEPROM_ADDRESS_COLOR2);
}

void showLastScene()
{
    const char lightScene = _getSceneEffectFromEEPROM();

    const uint32_t color1 = _getRGBColorFromEEPROM(EEPROM_ADDRESS_COLOR1);
    const uint32_t color2 = _getRGBColorFromEEPROM(EEPROM_ADDRESS_COLOR2);

    showScene(lightScene, color1, color2);
}

char _getSceneEffectFromEEPROM()
{
    const char lightScene = (char) EEPROM.read(EEPROM_ADDRESS_LIGHTSCENE);
    return lightScene;
}

void _setSceneEffectToEEPROM(const char lightScene)
{
    EEPROM.write(EEPROM_ADDRESS_LIGHTSCENE, (uint8_t) lightScene);
    EEPROM.commit();
}

uint32_t _getRGBColorFromEEPROM(const uint16_t startAddress)
{
    uint32_t color = 0x000000;

    const uint8_t r = EEPROM.read(startAddress);
    const uint8_t g = EEPROM.read(startAddress + 1);
    const uint8_t b = EEPROM.read(startAddress + 2);

    color = neopixelStrip.Color(r, g, b);
    return color;
}

void _setRGBColorToEEPROM(const uint32_t color, const uint16_t startAddress)
{
    // Split color to R, B, G parts
    const uint8_t r = (color >> 16) & 0xFF;
    const uint8_t g = (color >>  8) & 0xFF;
    const uint8_t b = (color >>  0) & 0xFF;

    EEPROM.write(startAddress, r);
    EEPROM.write(startAddress + 1, g);
    EEPROM.write(startAddress + 2, b);

    EEPROM.commit();
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

void setupPins()
{
 //   pinMode(PIN_STATUSLED, OUTPUT);
}

void setupEEPROM()
{
    Serial.println("Setup EEPROM...");

    EEPROM.begin(512);
}

void setupNeopixels()
{
    Serial.println("Setup Neopixels...");

    neopixelStrip.begin();
    neopixelStrip.setBrightness(NEOPIXELS_BRIGHTNESS);
}

void setupWifi()
{
    Serial.printf("Connecting to %s\n", wifiSsid);
    
    WiFi.begin(wifiSsid, wifiPassword);

    while (WiFi.status() != WL_CONNECTED)
    {
        // Blink 2 times when connecting
        blinkStatusLED(1);

        delay(500);
        Serial.print(".");
    }


    Serial.println("WiFi connected.");

    Serial.print("Obtained IP address: ");
    Serial.println(WiFi.localIP());
}

void neopixelFullOn() {
  for (uint16_t i = 0; i < neopixelStrip.numPixels(); i++)
    {
        neopixelStrip.setPixelColor(i, 0xFFFFFF);
    }
    neopixelStrip.setBrightness(255);
    neopixelStrip.show();
}

void _MQTTRequestCallback(char* topic, byte* payload, unsigned int length)
{
  neopixelFullOn();
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
         *
         */
        const char* payloadAsCharPointer = (char*) payload;

        const char lightScene = payloadAsCharPointer[0];
        const uint32_t color1 = _getRGBColorFromPayload(payloadAsCharPointer, 1);
        const uint32_t color2 = _getRGBColorFromPayload(payloadAsCharPointer, 1 + 6);

        Serial.printf("Light scene: %c\n", lightScene);
        Serial.printf("Color 1: %06X\n", color1);
        Serial.printf("Color 2: %06X\n", color2);

        showScene(lightScene, color1, color2);
        saveCurrentScene(lightScene, color1, color2);
    }
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

void setupMQTT()
{
 
}

void connectMQTT()
{
// Generate client name based on MAC address and last 8 bits of microsecond counter
  String clientName = "bedlights";
//  Serial.print("Connecting to ");
//  Serial.print(server);
//  Serial.print(" as ");
//  Serial.println(clientName);
  
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

void setup()
{
    Serial.begin(115200);
    delay(10);

//    setupPins();
    //setupEEPROM();

//    setupNeopixels();
    //showLastScene();

    setupWifi();
    setupMQTT();
    connectMQTT();
}

void loop()
{   
  client.loop();
}
