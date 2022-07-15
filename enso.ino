
#include "servo_functions.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "mbedtls/md.h"


//BLE
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
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic2 = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value2 = 14;
long randNumber;

class MyServerCallbacks: public BLEServerCallbacks {


  
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
       Serial.println("connected");
       openDoorWithServo();
       testscrolltext("VERBUNDEN");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
       Serial.println("disconnected");
       testscrolltext("GETRENNT");
    }

    void sendRandomNumber() {
      // print a random number from 0 to 999_999
      randNumber = random(1000000);
      Serial.println(randNumber);
    }
    
    void testscrolltext(String text) {
      // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
     if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
     }
  
     display.clearDisplay();

     display.setTextSize(2); // Draw 2X-scale text
     display.setTextColor(SSD1306_WHITE);
     display.setCursor(10, 0);
     display.println(text);
     display.display();      // Show initial text
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

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID2 "ecf2767c-812e-44e9-8c93-cf3042a7c7db"

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
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
  0b00000000, 0b00110000 };

#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex

RTC_DATA_ATTR int bootCount = 0;

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

bool firstLoop = true;

long generateRandomNumber() {
  // if analog input pin 0 is unconnected, random analog
   // noise will cause the call to randomSeed() to generate
   // different seed numbers each time the sketch runs.
   // randomSeed() will then shuffle the random function.
   randomSeed(analogRead(0));
      // print a random number from 0 to 999_999
      randNumber = random(1000000);
      Serial.println(randNumber);
      return randNumber;
   }

   String generateRandomString() {
  // if analog input pin 0 is unconnected, random analog
   // noise will cause the call to randomSeed() to generate
   // different seed numbers each time the sketch runs.
   // randomSeed() will then shuffle the random function.
   randomSeed(analogRead(0));
String result = "";
   char bb[] = {"QWERTYUIOPASDFGHJKLZXCVBNM"};
for(int c = 0; c <= 6; c++) {
   result += bb[random(0, 25)];
}
   Serial.println(result);
      // print a random number from 0 to 999_999
      
      return result;
   }

String encrypt(long seed){

  String s1 = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
  String s2 = "#######";
  String s3 = String(seed);
  bool b = s1.concat(s2);
  bool b2 = s1.concat(s3);  
  Serial.println(s1);

  char payload[50];
  s1.toCharArray(payload, 50);

  Serial.println(payload);
  byte shaResult[32];
 
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
 
  const size_t payloadLength = strlen(payload);
 
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char *) payload, payloadLength);
  mbedtls_md_finish(&ctx, shaResult);
  mbedtls_md_free(&ctx);
 
  String result;
  for(int i= 0; i< sizeof(shaResult); i++) {
    char str[3];
    sprintf(str, "%02x", (int)shaResult[i]);
    result += str;
  }

  return result;
}

union {
    char myByte[4];
    long mylong;
} foo;

void sendRandomNumber() {
  if (deviceConnected) {
    String nonce = generateRandomString();
    Serial.println("nonce String: " + nonce); 
    
    // myUnion.myLong = 1234L;

    pCharacteristic->setValue(nonce.c_str());
       pCharacteristic->notify();
       // value++;
       delay(5); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }
  
}
    
void setup() {
  Serial.begin(115200);
  
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  // Clear the buffer
  display.clearDisplay();
  
  Serial.print("Display width: ");
  Serial.print(display.width());
  Serial.print("Display height: " ); 
  Serial.print(display.height());

  showEnsoLogo();

  delay(1000);

if (bootCount == 0) {
  setupDeepSleep();
}
  showText("BEREIT");
  startBLEServer();
}

void loop() {
  if(firstLoop) {
    Serial.println("looping!");
    firstLoop = false;
  }

sendRandomNumber();


    if (deviceConnected) {
    pCharacteristic2->setValue((uint8_t*)&value2, 1);
   pCharacteristic2->notify();
  delay(30);
  }

  String shaResult2 = encrypt(generateRandomNumber());
  Serial.println("gotsha: " + shaResult2); 

  delay(1500);
   
}

void startBLEServer() {
  Serial.println("Starting BLE!");

  // Create the BLE Device
  BLEDevice::init("Enso Box 1");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristic->setValue("Hello World says Neil");

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Create a BLE Characteristic
  pCharacteristic2 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID2,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristic2->setValue("Hello World says Neil again");

  // Create a BLE Descriptor
  pCharacteristic2->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
  Serial.println("Waiting a client connection to notify...");

}


void showEnsoLogo(void) {
  display.clearDisplay();
  
  delay(1000); //Take some time to open up the Serial Monitor

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(display.width() / 2 - 20, display.height() / 2);
  display.println(F("ENSO"));
  
  display.fillCircle(display.width() / 2 + 45, display.height() / 2, 15, SSD1306_INVERSE);
  display.fillCircle(display.width() / 2 + 45, display.height() / 2, 11, SSD1306_INVERSE);
  display.display(); // Update screen with each newly-drawn circle

  delay(1000);
}

void showText(String text) {
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(display.width() / 2 - 30, display.height() / 2);
  display.println(text);
  
  display.display(); // Update screen with each newly-drawn circle

  delay(1000);
}

void setupDeepSleep() {
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
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
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,1); //1 = High, 0 = Low

  //If you were to use ext1, you would use it like
  //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);

  //Go to sleep now
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}
