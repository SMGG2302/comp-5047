# ESP32 硬件项目

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
