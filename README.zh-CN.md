# HollowHaven — 树屋环境与声音监测

[English](README.md) | 简体中文

当前 v4 参考：[中文技术手册](HollowHaven_技术手册.md)及[当前配置](project/CURRENT_SETUP.md)，包括接线、路由器联网、8766网页端口、光照控阀与维护告警。

面向大袋鼯（Greater glider，*Petauroides volans*）树洞庇护环境的 ESP32 原型。采集温湿度、两路导水布湿度、光照、水位和距离；近物体进入时触发约 30 秒录音。电脑上的本地网页显示数据、保存录音，并支持手动调用音频分析 API。

## 快速开始

1. 按[硬件说明](project/README.zh-CN.md)接线，将 `project/firmware/main/wifi_credentials.example.h` 复制为 `wifi_credentials.h`，填写路由器的 2.4 GHz Wi-Fi 凭据。真实凭据文件已被 Git 忽略。
2. 用 PlatformIO 编译、上传 `project/firmware`。Windows 英文路径辅助脚本与串口设置见硬件说明。
3. 电脑与 ESP32 连接同一路由器；电脑可以使用 Wi-Fi 或网线。关闭路由器客户端隔离。
4. 安装 Node.js 18 或以上，在 `website` 目录运行 `node server.js`；Windows 可双击 `website/HollowHaven.cmd`。无需安装 npm 依赖。
5. 打开 [http://127.0.0.1:8766](http://127.0.0.1:8766)，设备 IP 保持 `auto`，点击连接。发现失败时填入 ESP32 串口打印的 IPv4 地址。
6. 页头 **English / 中文** 可随时切换语言。默认英文，浏览器记住选择；切换不改变设备连接、设置或音频播放。
7. 在声音档案播放本地录音。需要分析时先配置 API，再手动上传选中的录音；云端分析需要电脑联网。

## 目录

| 目录 | 内容 |
| --- | --- |
| [project/](project/README.zh-CN.md) | C 固件、接线、协议、原生接收器与测试 |
| [website/](website/README.zh-CN.md) | Node.js 服务、中英文网页、API 与测试 |

[Draft.md](Draft.md) 为提案，[COURSE_BRIEF.md](COURSE_BRIEF.md) 为课程说明。网页用于原型调试和研究数据查看。

接线表：[下载 Excel 接线表](接线表.xlsx)。

## 当前运行规则

- ESP32 使用路由器 STA 模式：UDP 9002 自动发现、TCP 9000 遥测、TCP 9001 音频。本地网页仅监听 `127.0.0.1:8766`。
- 光照单条件测试：有效光照 ADC <500 开阀至 1200 μs，≥500 或光照无效时关阀至 2500 μs，上电关闭。温度和水位暂不参与开关阀判断。
- 湿度 <2000 为湿润，≥2300 为干燥，中间保持此前状态；初始中间值或无效采样为未知。
- 完全开阀满一分钟后任一路仍干燥则提示可能堵塞；水位 ADC <50 提示补水。“小于50ml”是设定标签，不是连续体积测量。
- 连续三次距离 <190 mm 标记疑似占用，连续五次 ≥220 mm 解除；无效测距不代表离开。

阀门状态表示舵机输出命令，不是机械位置反馈。湿度 ADC 未标定为百分比。实际降温、水流和物种识别仍需验证；红外不能确认动物身份，录音不支持完整蝙蝠超声识别。

## 语言与分析结果

导航、状态、悬浮提示、维护告警、图表、设置、日志及应用错误均支持中英文。新 Gemini 分析按发起上传时选择的语言返回。已有 AI 结果、文件名、自定义服务标签和用户填写内容保留原文；切换界面不会重新上传或自动翻译这些数据。

## 验证与本地数据

在 `website` 运行 `node --test test.js i18n.test.js`。可选浏览器检查为 `node ui-check.js`，需要 Playwright 和 Edge。原生 C 测试见硬件说明。模拟测试不能替代实际硬件、网络与动物声音验证。

录音、运行数据、密钥、Wi-Fi 凭据和构建缓存不提交。API 密钥仅保存在服务进程内存，重启后需重新输入；录音只有手动点击上传才发送。VL53L0X 驱动保留原许可证及[来源说明](project/firmware/components/st_tof/PROVENANCE.md)。
