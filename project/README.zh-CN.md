# ESP32 硬件与固件（v4）

[English](README.md) | 简体中文

当前使用 ESP32-WROOM-32、路由器 Wi-Fi、两路导水布湿度、光照控阀和实时维护状态。旧热点与舵机往返测试不代表当前配置。

## 接线

断电接线，所有模块共地，模拟输入电压限于 0–3.3 V。

| 模块 | ESP32 接口 |
| --- | --- |
| SHT3x | VCC 3V3，GND，SDA GPIO21，SCL GPIO22 |
| 左一电容湿度 | VCC 3V3，GND，AO GPIO34 |
| 左二电容湿度 | VCC 3V3，GND，AO GPIO36（VP） |
| 光照 | 兼容3.3V的供电与输出，GND，AO GPIO39（VN/SVN） |
| 水位 | 兼容3.3V的供电与输出，GND，AO GPIO32 |
| SEN0487 | VCC 3V3，GND，A GPIO33 |
| VL53L0X | VIN 3V3，GND，SDA GPIO27，SCL GPIO26；GPIO1/XSHUT 不接 |
| MG90S 水阀舵机 | 信号 GPIO13；外部稳压5V供电并与 ESP32 共地 |

DS18B20 禁用。以实物模块引脚及供电要求为准。

## 联网与烧录

将 `firmware/main/wifi_credentials.example.h` 复制为 `wifi_credentials.h`，填写路由器 2.4GHz 凭据；真实凭据不提交。其他参数在 `firmware/main/project_config.h`。

PlatformIO 打开 `firmware` 编译、上传；命令行为 `pio run`、`pio run -t upload --upload-port COM3`、`pio device monitor -p COM3 -b 115200`。COM3 改为实际 USB 串口。首次构建需下载工具链，不要删除音频缓存分区表 `partitions.csv`。

本机英文路径辅助脚本 `firmware/build-ascii.ps1 -Upload -Port COM3` 同步已保存源码到 `C:\HollowHavenBuild` 并使用 `C:\.platformio`。其他电脑使用前需检查路径。当前配置固定 espressif32 6.11.0、ESP-IDF 包 3.50402.0。

[本地网页](../website/README.zh-CN.md)地址为 `http://127.0.0.1:8766`；设备 IP 填 `auto` 自动发现，失败时填串口打印的 DHCP IP。电脑与 ESP32 在同一局域网，关闭客户端隔离。发现 UDP9002，遥测 TCP9000，音频 TCP9001。

## 当前控制规则

- 光照 ADC <500 开阀至1200μs，≥500或光照无效时关阀至2500μs，上电关闭；温度、水位暂不参与开关阀。
- 每100ms改变10μs，完整动作约13秒，50Hz PWM持续输出；状态表示命令，没有机械位置反馈。
- 湿度<2000为湿润、≥2300为干燥，中间保持此前状态；初始中间值或无效采样为未知。
- 完全开阀满60000ms后任一路仍干燥则提示可能堵塞；关闭重置计时，未知不算干燥。
- 水位 ADC <50 提示设定标签“小于50ml”和补水维护，不是连续体积换算。
- 连续3次距离<190mm标记占用，连续5次≥220mm解除；无效测距不代表离开。进入触发约30秒录音。

## 目录与验证

`firmware/` 为固件，`shared/` 为协议与状态逻辑，`pc_receiver/` 为可选C接收器，`tests/` 为C和Python测试。

在 `project` 运行 `cmake -S . -B build`、`cmake --build build`、`ctest --test-dir build --output-on-failure`。Windows 的 `build_receiver.ps1` 需要 CMake 和 Visual Studio C++ Build Tools。原生音频接收器仅支持 Windows；不要与网页同时争用设备连接。VL53L0X 保留[来源与许可证](firmware/components/st_tof/PROVENANCE.md)。
