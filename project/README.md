# ESP32 硬件项目

当前 v4 路由器联网、状态栏与水阀控制说明见 [CURRENT_SETUP.md](CURRENT_SETUP.md)。下方保留早期热点模式和舵机试调记录，接线与运行规则请以当前说明为准。

## 目录

- firmware/：ESP-IDF C 固件及 PlatformIO 配置。
- shared/：遥测、音频传输协议和占用判定逻辑。
- pc_receiver/：可选原生 C 遥测/音频接收器。
- tests/：C 单元测试和 Python 集成测试。

## 接线（ESP32-WROOM-32）

所有模块共地，信号须兼容 3.3 V；断电接线。

| 模块 | 引脚 | ESP32 |
| --- | --- | --- |
| SHT3x | VCC / GND / SDA / SCL | 3V3 / GND / GPIO21 / GPIO22 |
| 电容式湿度 | VCC / GND / AO | 3V3 / GND / GPIO34 |
| VL53L0X | VIN / GND / SDA / SCL | 3V3 / GND / GPIO27 / GPIO26 |
| VL53L0X | GPIO1 / XSHUT | 本原型不接 |
| SEN0487 | VCC / GND / A | 3V3 / GND / GPIO33 |

旧的 DS18B20、光敏和水位模块在当前配置中禁用。

## 编译和烧录

VS Code 安装 PlatformIO IDE，打开本目录下 firmware 文件夹，使用 Build / Upload。也可在已安装 PlatformIO Core 的终端执行：

    cd project/firmware
    pio run
    pio run -t upload --upload-port COM3
    pio device monitor -p COM3 -b 115200

COM3 仅为示例，按实际端口修改。配置固定 espressif32 6.11.0 和 ESP-IDF 包 3.50402.0；首次构建需要联网下载工具链。路径含中文导致工具链失败时，将仓库放在纯英文路径。不要删除 partitions.csv，它定义音频缓存分区。

## 运行配置

firmware/main/project_config.h 定义接线、热点凭据与阈值。默认热点 HollowHaven-ESP32，演示密码 HollowHaven5047，地址 192.168.4.1。

- 遥测 TCP 9000，每约 2 秒发送 HOLLOWHAVEN/3 数据。
- 音频 TCP 9001，HHW1 头与 16-bit 单声道 PCM；CRC32 校验后以 ACK1 确认。
- 小于 200 mm 连续 3 次为疑似占用，大于 250 mm 连续 5 次为空闲。无效测距单独标记，不能视为离开。
- 进入事件触发约 30 秒录音；实际采样率在音频头中传输。阈值为台架值，应根据空树屋距离校准。
- 当前热点仅允许一个终端连接。推荐使用 ../website 接收；不要同时运行旧 C 接收器争用通道。

## 可选 C 接收器与测试

Windows 安装 CMake 和 Visual Studio 2022 C++ Build Tools 后，在 project 目录运行：

    powershell -ExecutionPolicy Bypass -File ./build_receiver.ps1
    powershell -ExecutionPolicy Bypass -File ./run_receiver.ps1

音频接收使用 run_audio_receiver.ps1。跨平台遥测接收器可使用：

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

原生音频接收器目前仅在 Windows 构建。第三方驱动出处及许可证见 firmware/components/st_tof/PROVENANCE.md。

## 第二路湿度传感器（2026-10-27）
新增电容湿度模块：VCC→3V3，GND→GND，AO→GPIO35；原第一路仍接 GPIO34。GPIO35 不再接光敏模块。串口每约 2 秒输出 wick（第一路）、wick2（第二路）及 wick2_valid（是否采集到 ADC 样本，不代表传感器接线检测）。读数为 0–4095 原始 ADC 值，未标定百分比。为保持旧网页兼容，第二路暂不加入 v3 Wi-Fi 遥测，网页代码未修改。ADC 按麦克风、第一路湿度、麦克风、第二路湿度循环，保留麦克风一半采样时隙。


## 实测阈值与 MG90S（2026-10-28）
红外连续 3 次低于 190 mm 标记占用；连续 5 次达到 220 mm 解除，190–219 mm 保持原状态。两路湿度低于 2000 为湿，达到 2300 为干（根据干态约 2400 暂定），中间保持之前状态；初始中间值或无效采样为未知。湿度状态仅串口输出，网页未改。
MG90S 信号分配 GPIO25，使用外部稳压 5V 供电，外部 GND 与 ESP32 GND 共地。servo.c 使用 LEDC 50Hz，提供 hh_servo_set_angle；当前 HH_ENABLE_SERVO=1，上电从 HH_SERVO_START_ANGLE（默认90）开始连续往返测试，1000–2000微秒仅是待标定的有限脉宽范围，不保证对应完整物理0–180度。夹管开关角度和两路控制规则未确认，尚未实现自动滴水联动。


当前舵机测试模式：按用户要求已启用 HH_ENABLE_SERVO=1，上电从中点开始，按逻辑角度 0–180 往返循环，每 20 ms 一度，单程约 3.6 秒。使用 1000–2000 微秒的保守脉宽范围，不保证完整物理行程；暂不与湿度联动。

舵机扩大范围试调：当前配置为 500–2500 us，原先为 1000–2000 us；这是脉宽跨度两倍，不代表实际转角三倍。未验证实物机械端点，遇到卡住或持续嗡鸣应断电并缩小范围。

## 本机中文路径编译修复
本机提供 project/firmware/build-ascii.ps1，将已保存的固件和 shared 源码同步到 C:\HollowHavenBuild，再调用 C:\.platformio 工具链。VS Code 打开 project/firmware 后，先保存全部文件，选择“终端 → 运行任务 → HollowHaven: Upload (ASCII path)”。Ctrl+Shift+B 只编译。也可在 firmware 终端运行：

    powershell -ExecutionPolicy Bypass -File .\build-ascii.ps1 -Upload -Port COM3

原 PlatformIO Upload 按钮仍从中文路径构建，请使用新增任务。脚本为这台 Windows 电脑配置。
