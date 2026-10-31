# HollowHaven 本地观察站

[English](README.md) | 简体中文

Node.js 本地网页显示 ESP32 数据、阀门状态、维护告警和录音，并提供手动声音分析。网页默认英文；页头 **English / 中文** 可切换，浏览器记住选择。动态状态、提示、告警、图表、日志和应用错误均支持中英文，切换不打断连接或播放。

## 启动与连接

需要 Node.js 18 或以上，无需安装 npm 依赖。在 `website` 运行 `node server.js`，或双击 Windows 启动器 `HollowHaven.cmd`。打开 [http://127.0.0.1:8766](http://127.0.0.1:8766)。关闭页面不会停止后台接收；启动器创建的服务可用 `stop.ps1` 停止。

电脑与 ESP32 连接同一路由器，ESP32 使用 2.4 GHz；电脑可使用同一路由器的 5 GHz 或网线。关闭客户端隔离。设备 IP 填 `auto` 通过 UDP 9002 发现；失败时使用串口打印的 IP。遥测 TCP 9000，音频 TCP 9001。不要同时运行争用连接的其他接收器。

## 实时监测

状态栏显示补水、直射光、阀门和两路导水布状态，悬停或键盘聚焦可查看 ADC 和规则。另显示温湿度、距离及声音统计。[当前硬件规则](../project/README.zh-CN.md)提供接线和阈值。

水位 ADC <50 显示设定的“小于50ml”提示，不作连续体积测量。完全开阀满一分钟仍干燥提示可能堵塞，需现场检查。无效或过期数据保持未知，不能推定动物离开。

## 声音分析

选择 WAV，在设置填写 Gemini 或自定义 API，再手动上传。接收、播放、导入不会自动上传。密钥仅在服务进程内存保存，重启需重新输入；更换服务或接口且不填新密钥会清除旧密钥。API 可能产生服务商费用。

新 Gemini 分析按发起上传时的界面语言返回；已有分析和自定义服务返回内容保留原文，文件名及用户设置也不自动翻译。通用模型尚未经过本项目目标物种验证；分数不是校准后的识别概率，不能识别录音未采集到的超声。

自定义服务接收 POST 原始 WAV，`Content-Type: audio/wav`，可选 Bearer 密钥；外部接口必须使用 HTTPS，本机测试允许 HTTP。返回 `{label,evidence,uncertainty}`、`[{label,score}]` 或 `{predictions:[...]}`，示例见[英文说明](README.md)。

## 本地文件与测试

`recordings/` 保存录音，`data/session_*.csv` 保存会话遥测，`data/results.json` 保存分析，`data/settings.json` 保存不含密钥的设置。语言仅记录在浏览器 `hollowhaven.language`。支持 v4 和旧 v3 遥测；音频 CRC32 校验并持久化后才回复 ACK1。

HTTP 仅监听 `127.0.0.1:8766`。在本目录运行 `node --test test.js i18n.test.js`。可选 `node ui-check.js` 需要 Playwright 和 Edge；可通过 `HH_PLAYWRIGHT_PATH` 指定模块，`HH_UI_URL` 指定测试地址。模拟测试不替代实物与物种识别验证。
