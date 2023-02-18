
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "esp_sleep.h"
  
BLEAdvertising *pAdvertising;

// 设置蓝牙 MAC 地址和抓到的 raw 数据。raw 数据长度不应超过 62 个字节
uint8_t bleMac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
uint8_t bleRaw[] = {0x02, 0x01, 0x06, 0x17, 0xFF, 0x00, 0x01, 0xB5, 0x00, 0x02, 0x25, 0xEC, 0xD7, 0x44, 0x00, 0x00, 0x01, 0xAA, 0x91, 0x77, 0x67, 0xAF, 0x01, 0x10, 0x00, 0x00, 0x00, 0x03, 0x03, 0x3C, 0xFE, 0x0C, 0x09, 0x52, 0x54, 0x4B, 0x5F, 0x42, 0x54, 0x5F, 0x34, 0x2E, 0x31, 0x00};

// 设置 LED 灯的 PIN，具体数字跟自己的开发板型号对应，然后把 enableLed 改成 true
#define LED_PIN 22
boolean enableLed = false;

// 设置经过多少毫秒后自动休眠，0 为不自动休眠
unsigned long autoDeepSleepTime = 20 * 60 * 1000; // 20 分钟

// 设置串口波特率
#define SERIAL_RATE 115200

void led_init()
{
  if (!enableLed)
    return;
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

boolean criticalError = false;

void ble_init() {
  BLEAdvertising *pAdvertising;

  // esp32没有提供设置蓝牙mac地址的api 通过查看esp32的源代码
  // 此操作将根据蓝牙mac算出base mac
  if (UNIVERSAL_MAC_ADDR_NUM == FOUR_UNIVERSAL_MAC_ADDR)
  {
    bleMac[5] -= 2;
  }
  else if (UNIVERSAL_MAC_ADDR_NUM == TWO_UNIVERSAL_MAC_ADDR)
  {
    bleMac[5] -= 1;
  }
  esp_base_mac_addr_set(bleMac);

  // 初始化
  BLEDevice::init("");

  // Create the BLE Server
  // BLEServer *pServer = BLEDevice::createServer(); // <-- no longer required to instantiate BLEServer, less flash and ram usage

  pAdvertising = BLEDevice::getAdvertising();

  // 设备信息设置成空白的
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();
  pAdvertising->setScanResponseData(oScanResponseData);

  // 里面有个 m_customScanResponseData = true; 和 m_customScanResponseData = true; 所以只能先随便设置一下
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  pAdvertising->setAdvertisementData(oAdvertisementData);

  // 处理 bleRaw 数组
  uint8_t bleAdvRaw[31];
  size_t bleAdvRawLength;
  uint8_t bleScanRspRaw[31];
  size_t bleScanRspRawLength;

  if (sizeof(bleRaw) > 31 + 31)
  {
    criticalError = true;
    return;
  }

  if (sizeof(bleRaw) > 31)
  {
    bleAdvRawLength = 31;
    bleScanRspRawLength = sizeof(bleRaw) - 31;

    for (size_t i = 0; i < bleAdvRawLength; i++)
      bleAdvRaw[i] = bleRaw[i];
    for (size_t i = 0; i < bleScanRspRawLength; i++)
      bleScanRspRaw[i] = bleRaw[i + 31];
  }
  else
  {
    bleAdvRawLength = sizeof(bleRaw);
    bleScanRspRawLength = 0;
    for (size_t i = 0; i < bleAdvRawLength; i++)
      bleAdvRaw[i] = bleRaw[i];
  }

  // api 设置一下抓到的raw

  esp_err_t errRc = ::esp_ble_gap_config_adv_data_raw(bleAdvRaw, bleAdvRawLength);
  if (errRc != ESP_OK)
  {
    Serial.printf("esp_ble_gap_config_adv_data_raw: %d\n", errRc);
    criticalError = true;
    return;
  }
  if (bleScanRspRawLength > 0)
  {
    errRc = ::esp_ble_gap_config_scan_rsp_data_raw(bleScanRspRaw, bleScanRspRawLength);
    if (errRc != ESP_OK)
    {
      Serial.printf("esp_ble_gap_config_scan_rsp_data_raw: %d\n", errRc);
      criticalError = true;
      return;
    }
  }

  pAdvertising->start();
}

void setup()
{
  Serial.begin(SERIAL_RATE);
  ble_init();
  led_init();
}

void loop() {
  // 致命错误时灯常亮
  if (criticalError) {
    digitalWrite(LED_PIN, HIGH);
    delay(5000);
    return;
  }

  // 闪灯
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  delay(1000);

  // 自动休眠
  if (autoDeepSleepTime > 0 && millis() > autoDeepSleepTime)
  {
    esp_deep_sleep_start();
  }
}
