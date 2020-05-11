#include <bluefruit.h>

BLEDis bledis;

uint8_t advData[5] = {
  0xE2, 0xC5, 0x00, 0xC3, 0x42
};

unsigned long currentMillis;
unsigned long previousMillis;
unsigned long samplingInterval = 5000;

void setup() {
  Serial.begin(115200);
  while ( !Serial ) delay(10);
  Serial.println("Started\n");
  previousMillis = millis();
  Bluefruit.begin();
  Bluefruit.setTxPower(8);
  Bluefruit.setName("Gummi's Hygrometer");
  bledis.setManufacturer("Gummi");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();

  setupAdv();
  Bluefruit.Advertising.start(0);
}

void setupAdv() {
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED);
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.addManufacturerData(advData, 5);
  Bluefruit.Advertising.setFastTimeout(1);
}

void loop() {
  currentMillis = millis();
  if (currentMillis - previousMillis > samplingInterval) {
    previousMillis = currentMillis;
    Serial.println("New sample\n");
    advData[2]++;
    Bluefruit.Advertising.clearData();
    setupAdv();
  }
}
