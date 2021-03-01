# dingBle
修改esp32的蓝牙mac地址，模拟各种蓝牙设备  
并且还可以按照指定的raw内容来返回扫描结果  
比如模拟某钉的打卡机，实现蓝牙打卡

### 如何使用
首先，装一个叫 nRF Connect 的app，走到打卡机旁边，找到型号最强那个，记下他的mac地址，复制raw  
（当然貌似可以直接点clone就能用手机发射一个一样的出来）
然后修改 dingBle.ino 中的 bleMac 和 bleRaw ，两个一组，前面加上0x，是不是很直观？  
用arduino开发环境编译上传到esp32就可以用了
