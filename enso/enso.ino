
// #include "servo_functions.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "mbedtls/md.h"
// #include <Servo.h>
#include <Servo.h>

// Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

// BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLECharacteristic *pCharacteristic2 = NULL;
BLECharacteristic *pCharacteristic3 = NULL;
BLECharacteristic *pCharacteristic4 = NULL;
BLECharacteristic *pCharacteristic5 = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value2 = 14;
long randNumber;
String initialStatus = "waiting_for_rental_init";
static const int servoPin = 4;
//static const int servoPin = 14;

Servo servo1;

class MyServerCallbacks : public BLEServerCallbacks
{

  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.println("connected");
    testscrolltext("VERBUNDEN");
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("disconnected");
    testscrolltext("GETRENNT");
    setup(); // restart to a fresh state, so that the user can connect again
  }

  void testscrolltext(String text)
  {
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
      Serial.println(F("SSD1306 allocation failed"));
      for (;;)
        ; // Don't proceed, loop forever
    }

    display.clearDisplay();

    display.setTextSize(2); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.println(text);
    display.display(); // Show initial text
    delay(100);

    // Scroll in various directions, pausing in-between:
    display.startscrollright(0x00, 0x0F);
    delay(2000);
    display.stopscroll();
    delay(1000);
    display.startscrollleft(0x00, 0x0F);
    delay(2000);
    display.stopscroll();
    delay(1000);
    display.startscrolldiagright(0x00, 0x07);
    delay(2000);
    display.startscrolldiagleft(0x00, 0x07);
    delay(2000);
    display.stopscroll();
    delay(1000);
  }
};

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "ad03f23c-bf5b-47ba-946d-f468fcdc7ce6"
#define characteristic_uuid_status "ecf2767c-812e-44e9-8c93-cf3042a7c7db"
#define characteristic_uuid_user_id "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define characteristic_uuid_item_id "aa69fb3c-f28e-4ed0-a000-9ddba3358c42"
#define characteristic_uuid_code_verifier "fa49d57b-aadf-4ab2-96f2-97cb5e2f35ff"
#define characteristic_uuid_hash "ad03f23c-bf5b-47ba-946d-f468fcdc7ce6"

#define NUMFLAKES 10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT 16
#define LOGO_WIDTH 16
static const unsigned char PROGMEM logo_bmp[] =
    {0b00000000, 0b11000000,
     0b00000001, 0b11000000,
     0b00000001, 0b11000000,
     0b00000011, 0b11100000,
     0b11110011, 0b11100000,
     0b11111110, 0b11111000,
     0b01111110, 0b11111111,
     0b00110011, 0b10011111,
     0b00011111, 0b11111100,
     0b00001101, 0b01110000,
     0b00011011, 0b10100000,
     0b00111111, 0b11100000,
     0b00111111, 0b11110000,
     0b01111100, 0b11110000,
     0b01110000, 0b01110000,
     0b00000000, 0b00110000};

#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex

RTC_DATA_ATTR int bootCount = 0;

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

bool firstLoop = true;

long generateRandomNumber()
{
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));
  // print a random number from 0 to 999_999
  randNumber = random(1000000);
  // Serial.println(randNumber);
  return randNumber;
}

String generateRandomString()
{
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));
  String result = "";
  char bb[] = {"QWERTYUIOPASDFGHJKLZXCVBNM"};
  for (int c = 0; c <= 6; c++)
  {
    result += bb[random(0, 25)];
  }
  Serial.println(result);
  // print a random number from 0 to 999_999

  return result;
}

String encrypt(String nonce)
{

  String s1 = "elPalitoBoxUniquePasswort";
  bool b = s1.concat(nonce);
  // Serial.println(s1);

  char payload[50];
  s1.toCharArray(payload, 50);

  // Serial.println(payload);
  byte shaResult[32];

  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  const size_t payloadLength = strlen(payload);

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char *)payload, payloadLength);
  mbedtls_md_finish(&ctx, shaResult);
  mbedtls_md_free(&ctx);

  String result;
  for (int i = 0; i < sizeof(shaResult); i++)
  {
    char str[3];
    sprintf(str, "%02x", (int)shaResult[i]);
    result += str;
  }

  return result;
}

String sendNonce()
{
  String nonce = generateRandomString();

  Serial.println("Going to send the nonce: " + nonce);

  pCharacteristic->setValue(nonce.c_str());
  pCharacteristic->notify();
  delay(4); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms

  return nonce;
}

void setup()
{
  Serial.begin(115200);
  Serial.print("HELLLLLLLLLLLLLLLLOOOOOOOOOOOOOOOOOOOOO");
  pinMode(servoPin, OUTPUT);
  pinMode(5, INPUT);  // Setzt den PIN mit der Nummer PIN_NUMBER als Eingang
  pinMode(16, INPUT);
  
  // openDoorWithServo();
  //  SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();

  Serial.print("Display width: ");
  Serial.print(display.width());
  Serial.print("Display height: ");
  Serial.print(display.height());

  showEnsoLogo();

  delay(1000);

  if (bootCount == 0)
  {
    // setupDeepSleep();
  }
  showText("BEREIT");
  startBLEServer();
}

bool nonceSend = false;
String nonce = "";
String shaResult = "";
bool boxIsOpen = false;

void openDoor()
{

  digitalWrite(servoPin, HIGH); // turn the LED on (HIGH is the voltage level)
  Serial.println("Wakeup caused by external signal using RTC_IO");
  delay(200);                  // wait for a second
  digitalWrite(servoPin, LOW); // turn the LED off by making the voltage LOW
  delay(500);
}

void openDoorWithServo()
{
  servo1.attach(servoPin);

  for (int posDegrees = -10; posDegrees <= 40; posDegrees++)
  {
    servo1.write(posDegrees);
    // Serial.println(posDegrees);
    delay(200);
  }

  delay(2000);

  for (int posDegrees = 40; posDegrees >= -10; posDegrees--)
  {
    servo1.write(posDegrees);
    Serial.println(posDegrees);
    delay(200);
  }
}

String currentStatus = initialStatus;
// ##########################################################################################################
// ##########################################################################################################
// ############################### LOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOP ########################################
// ##########################################################################################################
// ##########################################################################################################
void loop()
{
  int feedbackLock1 = digitalRead(5);  // Liest den aktuellen Wert des Pins aus
  Serial.print("current feedbackLock1: ");
  Serial.println(feedbackLock1);
  int feedbackLock2 = digitalRead(16);
  Serial.print("current feedbackLock2: ");
  Serial.println(feedbackLock2);
  if (firstLoop)
  {
    Serial.println("looping!");
    firstLoop = false;
  }

  // if (!nonceSend && deviceConnected)
  // {
  //   nonce = sendNonce();
  //   nonceSend = true;
  //   shaResult = encrypt(nonce);
  //   Serial.println("gotsha: " + shaResult);
  // }

  if (deviceConnected)
  {
    std::string statusRxValue = pCharacteristic->getValue();
    Serial.print("characteristic !!!!!!!!!!! value read in loop = ");
    Serial.println(statusRxValue.c_str());
    // showText(String(statusRxValue.c_str()));

    if (String(statusRxValue.c_str()) == "init_rent_out")
    {
      showText("init rent");
      //TODO: generate code
      currentStatus = "sending_code";
      pCharacteristic->setValue(currentStatus.c_str());
      pCharacteristic->notify();
      delay(4);
    }
    else if (String(statusRxValue.c_str()) == "sending_hash")
    {
      showText("got hash");

      std::string hashRxValue = pCharacteristic5->getValue();
      Serial.print("hashRxValue value read in loop = ");
      Serial.println(hashRxValue.c_str());
      if (String(hashRxValue.c_str()) == "testHash")
      {
        Serial.print("hash is correct");
        if (!boxIsOpen)
        {
          openDoor();
          //   openDoorWithServo();
          showText("GEÖFFNET");
          boxIsOpen = true;
        }
      }
    }
  }
  delay(1000);
}
// ##########################################################################################################
// ##########################################################################################################
// ############################### LOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOP ########################################
// ##########################################################################################################
// ##########################################################################################################

void startBLEServer()
{
  Serial.println("Starting BLE!");

  // Create the BLE Device
  BLEDevice::init("Enso Box 2");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create the BLE Characteristic
  // createBleCharacteristics();

  pCharacteristic = pService->createCharacteristic(
      characteristic_uuid_status,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic->setValue(initialStatus.c_str());

  // // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic2 = pService->createCharacteristic(
      characteristic_uuid_user_id,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic2->setValue("Hello World says Neil");

  // Create a BLE Descriptor
  pCharacteristic2->addDescriptor(new BLE2902());

  pCharacteristic3 = pService->createCharacteristic(
      characteristic_uuid_item_id,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic3->setValue("Hello World says Neil");

  // Create a BLE Descriptor
  pCharacteristic3->addDescriptor(new BLE2902());

  pCharacteristic4 = pService->createCharacteristic(
      characteristic_uuid_code_verifier,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic4->setValue("Hello World says Neil");

  // Create a BLE Descriptor
  pCharacteristic4->addDescriptor(new BLE2902());

  pCharacteristic5 = pService->createCharacteristic(
      characteristic_uuid_hash,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic5->setValue("Hello World says Neil");

  // Create a BLE Descriptor
  pCharacteristic5->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
  Serial.println("Waiting a client connection to notify...");
}

void createBleCharacteristics()
{
}

void showEnsoLogo(void)
{
  display.clearDisplay();

  delay(1000); // Take some time to open up the Serial Monitor

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(display.width() / 2 - 20, display.height() / 2);
  display.println(F("ENSO"));

  display.fillCircle(display.width() / 2 + 45, display.height() / 2, 15, SSD1306_INVERSE);
  display.fillCircle(display.width() / 2 + 45, display.height() / 2, 11, SSD1306_INVERSE);
  display.display(); // Update screen with each newly-drawn circle

  delay(1000);
}

void showText(String text)
{
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(display.width() / 2 - 30, display.height() / 2);
  display.println(text);

  display.display(); // Update screen with each newly-drawn circle

  delay(1000);
}

void setupDeepSleep()
{
  Serial.begin(115200);
  delay(1000); // Take some time to open up the Serial Monitor

  // Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  // Print the wakeup reason for ESP32
  print_wakeup_reason();

  /*
  First we configure the wake up source
  We set our ESP32 to wake up for an external trigger.
  There are two types for ESP32, ext0 and ext1 .
  ext0 uses RTC_IO to wakeup thus requires RTC peripherals
  to be on while ext1 uses RTC Controller so doesnt need
  peripherals to be powered on.
  Note that using internal pullups/pulldowns also requires
  RTC peripherals to be turned on.
  */
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1); // 1 = High, 0 = Low

  // If you were to use ext1, you would use it like
  // esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);

  // Go to sleep now
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}
