
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "esp_sleep.h"

BLEAdvertising *pAdvertising;

// ==== 配置开始 ====

// 设置蓝牙 MAC 地址和抓到的 raw 数据。raw 数据长度不应超过 62 个字节
uint8_t bleMac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
uint8_t bleRaw[] = {0x02, 0x01, 0x06, 0x17, 0xFF, 0x00, 0x01, 0xB5, 0x00, 0x02, 0x25, 0xEC, 0xD7, 0x44, 0x00, 0x00, 0x01, 0xAA, 0x91, 0x77, 0x67, 0xAF, 0x01, 0x10, 0x00, 0x00, 0x00, 0x03, 0x03, 0x3C, 0xFE, 0x0C, 0x09, 0x52, 0x54, 0x4B, 0x5F, 0x42, 0x54, 0x5F, 0x34, 0x2E, 0x31, 0x00};

// 设置功率。若无节能需求，可直接调至最大。
// ESP_PWR_LVL_N12 = 0,                /*!< Corresponding to -12dbm */
// ESP_PWR_LVL_N9  = 1,                /*!< Corresponding to  -9dbm */
// ESP_PWR_LVL_N6  = 2,                /*!< Corresponding to  -6dbm */
// ESP_PWR_LVL_N3  = 3,                /*!< Corresponding to  -3dbm */
// ESP_PWR_LVL_N0  = 4,                /*!< Corresponding to   0dbm */
// ESP_PWR_LVL_P3  = 5,                /*!< Corresponding to  +3dbm */
// ESP_PWR_LVL_P6  = 6,                /*!< Corresponding to  +6dbm */
// ESP_PWR_LVL_P9  = 7,                /*!< Corresponding to  +9dbm */
const esp_power_level_t ADV_POWER = ESP_PWR_LVL_P9;

// 设置广播频率。若无节能需求，可直接调至最快。
// Range: 0x0020 to 0x4000 Example: N = 0x0800 (1.28 second)
// Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec
const uint16_t ADV_FREQ = 0x0200;

// DEEP_SLEEP_SEC 不为零时，开启节能：发送广播 ADV_SEC 秒，深度睡眠 DEEP_SLEEP_SEC 秒。
const uint16_t DEEP_SLEEP_SEC = 0;
const uint16_t ADV_SEC = 1;

// 设置经过多少秒后自动关机，0 为不自动关机
const unsigned long AUTO_DEEP_SLEEP_TIME = 20 * 60; // 20 分钟

// 设置 LED 灯的 PIN，具体数字跟自己的开发板型号对应，然后把 LED_ENABLED 改成 true
const uint8_t LED_PIN = 22;
const bool LED_ENABLED = false;
const bool LED_DIGITAL_REVERSE = true; // LED_DIGITAL_REVERSE 开启时，LOW 为亮，HIGH 为灭。反之亦然。

// 设置串口波特率
const long SERIAL_RATE = 115200;

// ==== 配置结束 ====

bool criticalError = false;

void led_init()
{
  if (!LED_ENABLED)
  {
    Serial.println("LED disabled.");
    return;
  }
  pinMode(LED_PIN, OUTPUT);
  Serial.println("LED enabled on PIN " + LED_PIN);
  led_off();
}

void led_on()
{
  if (!LED_DIGITAL_REVERSE)
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);
}

void led_off()
{
  if (!LED_DIGITAL_REVERSE)
    digitalWrite(LED_PIN, LOW);
  else
    digitalWrite(LED_PIN, HIGH);
}

BLEAdvertising *pAdvertising;

void ble_init()
{
  // 处理 bleRaw 数组
  uint8_t _bleAdvRaw[ESP_BLE_ADV_DATA_LEN_MAX];
  size_t _bleAdvRawLength;
  uint8_t _bleScanRspRaw[ESP_BLE_SCAN_RSP_DATA_LEN_MAX];
  size_t _bleScanRspRawLength;

  assert(sizeof(uint8_t) == 1);
  assert(ESP_BLE_ADV_DATA_LEN_MAX == 31 && ESP_BLE_SCAN_RSP_DATA_LEN_MAX == ESP_BLE_ADV_DATA_LEN_MAX);
  if (sizeof(bleRaw) > ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX)
  {
    criticalError = true;
    return;
  }
  if (sizeof(bleRaw) > ESP_BLE_ADV_DATA_LEN_MAX)
  {
    _bleAdvRawLength = ESP_BLE_ADV_DATA_LEN_MAX;
    _bleScanRspRawLength = sizeof(bleRaw) - ESP_BLE_ADV_DATA_LEN_MAX;

    for (size_t i = 0; i < _bleAdvRawLength; i++)
      _bleAdvRaw[i] = bleRaw[i];
    for (size_t i = 0; i < _bleScanRspRawLength; i++)
      _bleScanRspRaw[i] = bleRaw[i + ESP_BLE_ADV_DATA_LEN_MAX];
  }
  else
  {
    _bleAdvRawLength = sizeof(bleRaw);
    _bleScanRspRawLength = 0;
    for (size_t i = 0; i < _bleAdvRawLength; i++)
      _bleAdvRaw[i] = bleRaw[i];
  }

  // 设置 MAC
  uint8_t _bleMac[6];
  for (size_t i = 0; i < 6; i++)
    _bleMac[i] = bleMac[i];
  // esp32没有提供设置蓝牙mac地址的api 通过查看esp32的源代码
  // 此操作将根据蓝牙mac算出base mac
  if (UNIVERSAL_MAC_ADDR_NUM == FOUR_UNIVERSAL_MAC_ADDR)
  {
    _bleMac[5] -= 2;
  }
  else if (UNIVERSAL_MAC_ADDR_NUM == TWO_UNIVERSAL_MAC_ADDR)
  {
    _bleMac[5] -= 1;
  }
  esp_err_t ret = esp_base_mac_addr_set(_bleMac);
  if (ret != ESP_OK)
  {
    criticalError = true;
    return;
  }

  // 初始化
  BLEDevice::init("");
  BLEDevice::setPower(ADV_POWER, ESP_BLE_PWR_TYPE_DEFAULT);
  BLEDevice::setPower(ADV_POWER, ESP_BLE_PWR_TYPE_ADV);

  // Create the BLE Server
  // BLEServer *pServer = BLEDevice::createServer(); // <-- no longer required to instantiate BLEServer, less flash and ram usage

  pAdvertising = BLEDevice::getAdvertising();

  // 设置一下抓到的raw
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  oAdvertisementData.addData(std::string(reinterpret_cast<char *>(_bleAdvRaw), _bleAdvRawLength));
  pAdvertising->setAdvertisementData(oAdvertisementData);

  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();
  oScanResponseData.addData(std::string(reinterpret_cast<char *>(_bleScanRspRaw), _bleScanRspRawLength));
  pAdvertising->setScanResponseData(oScanResponseData);

  // 设置广播频率
  pAdvertising->setMinInterval(ADV_FREQ);
  pAdvertising->setMaxInterval(ADV_FREQ);
}

void ble_on()
{
  pAdvertising->start();
}

void ble_off()
{
  pAdvertising->stop();
}

void setup()
{
  Serial.begin(SERIAL_RATE);
  led_init();
  ble_init();

  // 不进行节能设置时，始终广播
  if (DEEP_SLEEP_SEC == 0)
    ble_on();
}

void loop()
{
  // 致命错误时灯常亮
  if (criticalError)
  {
    led_on();
    delay(5000);
    return;
  }

  // 自动关机检测
  if (AUTO_DEEP_SLEEP_TIME > 0 && millis() > AUTO_DEEP_SLEEP_TIME * 1000)
  {
    esp_deep_sleep_start(); // 未配置唤醒，相当于关机
    return;
  }

  if (DEEP_SLEEP_SEC == 0)
  {
    // 不进行节能设置时，始终广播，LED 灯每秒闪烁
    led_on();
    delay(1000);
    led_off();
    delay(1000);
  }
  else
  {
    // 进行节能设置时，广播开启几秒后关闭，LED 灯的亮灭表示广播状态
    led_on();
    ble_on();
    delay(ADV_SEC * 1000);
    ble_off();
    led_off();
    esp_deep_sleep(DEEP_SLEEP_SEC * 1000000);
  }
}
