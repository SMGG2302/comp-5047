# HollowHaven

[English](README.md) | 简体中文

面向大袋鼯（Greater glider，*Petauroides volans*）栖息环境研究的 ESP32 人工树洞原型，集成环境监测、本地自动滴水、距离触发录音与中英文网页。项目目前为课程功能原型，尚未确认长期野生动物适用性或物种识别准确率。

## 项目文档

更新于2026年11月6日。三套文档均提供 PDF 与可编辑的 LaTeX 源码包。

| 文档 | 内容与用途 | 中文 | 英文 | LaTeX 源码 |
| --- | --- | --- | --- | --- |
| 项目报告 | 设计依据、系统实现、评估结果和局限 | [PDF](docs/report/report-zh.pdf) | [PDF](docs/report/report-en.pdf) | [ZIP](docs/report/report-latex.zip) |
| 用户手册 | 首次使用快速指南、网页操作、维护与故障处理 | [PDF](docs/manuals/HollowHaven-User-Manual-Chinese.pdf) | [PDF](docs/manuals/HollowHaven-User-Manual-English.pdf) | [ZIP](docs/manuals/HollowHaven-User-Manual-LaTeX.zip) |
| 技术手册 | 系统架构、接线、装配、软件部署与校准 | [PDF](docs/manuals/HollowHaven-Technical-Manual-Chinese.pdf) | [PDF](docs/manuals/HollowHaven-Technical-Manual-English.pdf) | [ZIP](docs/manuals/HollowHaven-Technical-Manual-LaTeX.zip) |

实际接线请逐项核对[接线表](docs/wiring/Wiring-Table.zh-CN.md)、[当前配置](project/CURRENT_SETUP.md)与模块端子标记。技术手册的实物风格接线示意图中，少数电源短线与焊盘之间存在视觉间隙。机械尺寸和制造细节中的待填写项仍需补齐。

## 当前功能

- ESP32-WROOM-32 采集 SHT3x 温湿度、两路电容式导水层湿度、模拟光照与水位、VL53L0X 距离及 SEN0487 麦克风信号。
- MG90S 舵机控制重力供水，将水送往外部导水层；控制逻辑在 ESP32 本地运行。
- 近物体进入事件触发约30秒录音，持续停留不会重复触发进入录音。
- 本地网页提供实时状态、趋势、维护提示、会话 CSV 导出、WAV 播放／下载／导入及中英文切换。
- 可选音频分析由用户配置服务并手动发起，结果为待核实候选，不能确认动物身份。

## 快速开始

### 准备设备

1. 按技术手册和[硬件说明](project/README.zh-CN.md)完成接线。传感器信号应兼容3.3V；舵机使用外部稳压5V供电并共地。
2. 将 `project/firmware/main/wifi_credentials.example.h` 复制为同目录下的 `wifi_credentials.h`，填写本地2.4GHz Wi-Fi凭据。真实凭据文件已被 Git 忽略。
3. 使用 PlatformIO 编译并上传 `project/firmware`。安装前在地面完成电气、阀门与水路检查。固件支持主用网络及可选备用网络。

### 开始监测

1. 电脑连接设备所在局域网，可使用互通的 Wi-Fi 或有线网络；关闭路由器客户端隔离。
2. 安装 Node.js 18 或以上，在 `website` 目录运行 `node server.js`；Windows 也可双击 `website/HollowHaven.cmd`。服务运行无需 `npm install`。
3. 打开 [http://127.0.0.1:8766](http://127.0.0.1:8766)，将设备地址保存为 `auto` 后连接。发现失败时，使用设备串口输出的实际 IP。
4. 确认传感器读数和更新时间持续变化，按需切换页面语言。需要音频分析时再配置服务，选中录音并主动上传。
5. 持续接收时保持电脑唤醒、联网及后台服务运行。停止接收时，在 `website` 目录打开 PowerShell，执行 `./stop.ps1`。

关闭浏览器不会结束后台接收。停止服务或网络断开也不会停止 ESP32 自动供水。停用整机时先人工隔离供水，再断电；断电不能保证阀门关闭。

## 运行行为与边界

| 项目 | 当前行为 |
| --- | --- |
| 网络 | 路由器 STA 模式；UDP 9002发现、TCP 9000遥测、TCP 9001音频。网页服务仅监听 `127.0.0.1:8766`。 |
| 滴水 | 有效光照 ADC <500 且温度 >30°C 时请求开阀；完全开启后至少保持60秒，此后每分钟检查，仅温度 <26°C 时关闭。后续光照变化不结束周期，温度无效时保持开启至后续检查。 |
| 维护 | 低水位提示补水，不自动停水；完全开启60秒后导水层仍干燥，提示可能堵塞。 |
| 状态含义 | 阀门状态是命令，未测量实际阀位或流量。ADC 未标定为毫升、照度或含水百分比；“小于50ml”标签不代表体积测量。 |
| 数据缺口 | 断网或电脑休眠时无法接收实时遥测，完整缺失历史不会补传；设备录音缓存容量有限。 |
| 验证边界 | 控制温度是工程设定，尚非已验证的动物舒适界限。距离不能识别物种，录音不覆盖蝙蝠超声识别。 |

当前没有最大供水时长或低水位自动停水联锁，应安排补水并提供人工隔离供水的方法。“水位下降而电容传感器不响应”的联合判断目前属于人工检查，尚未实现为自动告警。完整阈值和校准方法见技术手册与源码。

## 仓库结构与评估数据

| 路径 | 内容 |
| --- | --- |
| [project/](project/README.zh-CN.md) | ESP-IDF C 固件、共享控制与协议代码、原生接收器及测试 |
| [website/](website/README.zh-CN.md) | Node.js 服务、中英文网页及测试 |
| [docs/manuals/](docs/manuals/) | 用户／技术手册 PDF 与 LaTeX 包 |
| [docs/report/](docs/report/) | 当前中英文项目报告与 LaTeX 包 |
| [modeling/](modeling/) | Blender 模型：`HollowHaven_scaled.blend`、`box2.blend`、`funnel.blend` |
| [十日数据](experiments/nest-microclimate-measurements/README.md) | 工作簿、CSV 记录与独立图表页面 |
| [Draft.md](Draft.md)、[idea/](idea/)、[COURSE_BRIEF.md](COURSE_BRIEF.md) | 提案、设计历史及课程说明 |

团队将十日数据确认为测量记录，数据 README 说明了历史模拟标签的更正。采集日期、地点、仪器、校准和各通道处理细节仍待团队提供，不能仅凭这些记录确认动物福利收益。独立数据图表页面与实时监测网页分开使用。旧版 `docs/COMP5047_Project_report.pdf` 作为历史报告保留，修订版以 `docs/report/` 为准。

## 验证与本地数据

在 `website` 目录运行网页测试：

```sh
node --test test.js i18n.test.js gemini.test.js
```

可选浏览器检查 `node ui-check.js` 需要 Playwright 和 Edge。原生 C 构建与测试步骤见[硬件说明](project/README.zh-CN.md)。软件测试不能替代硬件及现场验证。

遥测保存在 `website/data/session_*.csv`，录音保存在 `website/recordings/`，分析结果保存在 `website/data/results.json`，需定期备份。运行数据、真实 Wi-Fi 凭据、API 密钥与构建缓存不提交到 Git。API 密钥仅保存在服务进程内存，重启后需重新输入。录音仅在用户主动发起时上传，外部服务可能收费。VL53L0X 驱动保留其[许可证与来源说明](project/firmware/components/st_tof/PROVENANCE.md)。
