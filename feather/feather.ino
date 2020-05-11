#include <Adafruit_HTU21DF.h>
#include <bluefruit.h>

#define VBATPIN A6

BLEDis bledis;
Adafruit_HTU21DF htu;

uint8_t advData[18];

unsigned long currentMillis;
unsigned long previousMillis;
unsigned long samplingInterval = 5000;

union {
  float temp;
  uint8_t temp_data[4];
} temp;

union {
  float humidity;
  uint8_t humidity_data[4];
} humidity;

union {
  long millis;
  uint8_t millis_data[4];
} now;

union {
  float battery;
  uint8_t battery_data[4];
} battery;

void setup() {
  previousMillis = millis();

  htu.begin();
  Bluefruit.begin();
  Bluefruit.setTxPower(8);
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
  Bluefruit.Advertising.setInterval(100, 16384);
  Bluefruit.Advertising.setFastTimeout(1);
}

float readBatteryVoltage() {
  return (2.0 * 3.3 * analogRead(VBATPIN)) / 1024.0;
}

float readBatteryPercentage() {
  return (readBatteryVoltage() - 3.7) / 4.2;
}

void loop() {
  currentMillis = millis();
  if (currentMillis - previousMillis > samplingInterval) {
    previousMillis = currentMillis;
    battery.battery = readBatteryPercentage();
    temp.temp = htu.readTemperature();
    humidity.humidity = htu.readHumidity();
    for (int i = 0; i < 4; ++i) {
      now.millis = currentMillis;
      advData[2 + i] = now.millis_data[i];
      advData[6 + i] = temp.temp_data[i];
      advData[10 + i] = humidity.humidity_data[i];
      advData[14 + i] = battery.battery_data[i];
    }
    Bluefruit.Advertising.clearData();
    setupAdv();
  }
}
