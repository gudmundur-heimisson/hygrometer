#include <Adafruit_HTU21DF.h>
#include <bluefruit.h>

#define VBATPIN A8

// Function prototypes from battery.cpp
float readVBAT(void);
float mvToPercent(float);

BLEDis bledis;
Adafruit_HTU21DF htu;

uint8_t advData[18];

unsigned long currentMillis;
unsigned long previousMillis;
// 30 seconds
unsigned long samplingInterval = 30000;

union float_data {
  float value;
  uint8_t bytes[4];
};

union ulong_data {
  unsigned long value;
  uint8_t bytes[4];
};

union float_data temp;
union float_data humidity;
union float_data battery;
union ulong_data now;

void setup() {
  previousMillis = millis();
  htu.begin();
  Bluefruit.begin();
  // Lowest transaction power
  Bluefruit.setTxPower(-20);
  // Turn off blue LED
  Bluefruit.autoConnLed(false);
  Bluefruit.setName("Gummi's Hygrometer");
  bledis.setManufacturer("Gummi");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();

  setupAdv();
  advData[0] = 0xC3;
  advData[1] = 0xD5;
  Bluefruit.Advertising.start(0);
}

void setupAdv() {
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED);
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.addManufacturerData(advData, 18);
  // Slow sampling interval at 10.24 seconds, maximum allowed
  Bluefruit.Advertising.setInterval(100, 16384);
  // Lowest allowable fast timeout, 0.625 ms
  Bluefruit.Advertising.setFastTimeout(1);
}

void loop() {
  currentMillis = millis();
  if (currentMillis - previousMillis > samplingInterval) {
    previousMillis = currentMillis;
    now.value = currentMillis;
    temp.value = htu.readTemperature();
    humidity.value = htu.readHumidity();
    float mvbat = readVBAT();
    battery.value = mvToPercent(mvbat);
    for (int i = 0; i < 4; ++i) {
      advData[2 + i] = now.bytes[i];
      advData[6 + i] = temp.bytes[i];
      advData[10 + i] = humidity.bytes[i];
      advData[14 + i] = battery.bytes[i];
    }
    Bluefruit.Advertising.clearData();
    setupAdv();
  }
}
