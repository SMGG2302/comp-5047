# HollowHaven — 树屋环境与声音监测

围绕大袋鼯（Greater Glider）树洞庇护环境开展的原型项目。ESP32 采集环境和距离数据，在检测到近物体时录制声音；电脑接收数据，通过本地网页查看趋势、播放录音，并按需调用音频分析 API。

## 代码结构

| 文件夹 | 内容 |
| --- | --- |
| [project/](project/README.md) | ESP32 C 固件、共享协议、原生 C 接收器与测试 |
| [website/](website/README.md) | Node.js 本地服务、HTML/CSS/JavaScript 页面、API 接口与测试 |

[Draft.md](Draft.md) 保留项目提案，[COURSE_BRIEF.md](COURSE_BRIEF.md) 保留原课程说明。网页是原型调试与研究数据查看工具。

## 快速开始

1. 按 [硬件 README](project/README.md) 接线，在 VS Code PlatformIO 中打开 project/firmware，编译并上传。
2. 安装 Node.js（最低 18，建议使用受支持的 LTS），进入 website，运行 node server.js；Windows 也可双击 website/HollowHaven.cmd。无需 npm install。
3. 电脑连接 HollowHaven-ESP32 热点，打开 http://127.0.0.1:8765，点击连接 ESP32。
4. 在声音档案中播放录音；需要识别时配置 API，切换到可上网的网络，再手动上传选中的录音。

## 当前能力与边界

- 四个传感器：SHT3x 温湿度、电容式吸水层湿度、VL53L0X 距离、SEN0487 模拟麦克风。
- 距离触发约 30 秒录音；二进制传输带 CRC32，保存成功后确认接收。
- AI 候选类别包含大袋鼯、其他负鼠/袋鼯、鸟类、昆虫、环境声和未知。尚未验证物种识别准确率；红外遮挡也不等于确认动物身份。
- 当前声音采样约 8192 Hz，不能用于完整的蝙蝠超声识别。
- 吸水层 ADC 是原始值；降温结构及自动阀门控制仍需实验验证，本版本未实现闭环水流控制。
- 网页仅本机访问，未启用局域网共享或公网部署。

## 验证和数据

网页测试：在 website 执行 node --test test.js。C 接收器和状态逻辑测试见 project/README.md。模拟测试不能替代真实传感器、网络与动物声音效果验证。

录音、运行数据、API 密钥、驱动安装包、构建缓存均不提交。API 密钥只在本地服务进程内存保存。固件包含演示热点密码，实际部署前请修改。第三方 VL53L0X 驱动保留原许可证及来源说明，见 project/firmware/components/st_tof/PROVENANCE.md。
