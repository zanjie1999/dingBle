#include "BluetoothSerial.h"
#include "SimpleBLE.h"

RTC_DATA_ATTR int addrIndex = 0;
int macAddrsSize = 7;
uint8_t macAddrs[][6] = {
    {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}, 
    {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}};
String bleName = "";

void setup() {
    pinMode(0, INPUT);
    Serial.begin(115200);
    Serial.println("\r\n");
    Serial.println(addrIndex);
}

void loop() {
    uint8_t* addr = macAddrs[addrIndex];
    // esp32没有提供设置蓝牙mac地址的api 通过查看esp32的源代码
    // 此操作将根据蓝牙mac算出base mac
    if (UNIVERSAL_MAC_ADDR_NUM == FOUR_UNIVERSAL_MAC_ADDR) {
        addr[5] -= 2;
    } else if (UNIVERSAL_MAC_ADDR_NUM == TWO_UNIVERSAL_MAC_ADDR) {
        addr[5] -= 1;
    }
    esp_base_mac_addr_set(addr);
    // SimpleBLE ble;
    // ble.begin(bleName);
    BluetoothSerial SerialBT;
    SerialBT.begin(bleName);
    while (digitalRead(0)) {
        if (Serial.available()) {
            SerialBT.write(Serial.read());
        }
        if (SerialBT.available()) {
            Serial.write(SerialBT.read());
        }
    }
    addrIndex++;
    if (addrIndex >= macAddrsSize) {
        addrIndex = 0;
    }
    while (!digitalRead(0)) {
        Serial.println(addrIndex);
        delay(100);
    }

    //由于无法重新初始化蓝牙，因此直接重启
    esp_sleep_enable_timer_wakeup(0);
    esp_deep_sleep_start();
}