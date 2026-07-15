# EvilESP32 – 全品牌 BLE 广播轮播器（多协议弹窗触发）

> **安全研究 & 蓝牙协议学习工具** – 用于演示 BLE 广播数据包的构造与解析，测试设备对各类广播协议的响应。

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange)](https://platformio.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)
[![Arduino](https://img.shields.io/badge/Arduino-2.0.17-blue)](https://www.arduino.cc/)

---

## 📖 项目简介

`EvilESP32` 是一个基于 ESP32 的 BLE 广播模拟器，能够生成 **Apple Continuity**、**Google Fast Pair**、**Samsung Easy Setup**、**Microsoft Swift Pair** 以及 **Lovespouse** 等主流厂商的专属广播数据包。当手机/电脑等终端设备扫描到这些广播时，会触发系统级弹窗（如 AirPods 连接提示、Fast Pair 配对通知、Windows 快速配对等）。

本项目**仅供安全研究、蓝牙协议教学和渗透测试授权场景使用**。通过剖析商业 BLE 广播协议，开发者可以深入了解各厂商的弹窗触发机制，并为设备兼容性测试、企业安全演练提供工具支持。

---

## 🚀 主要特性

- **全协议支持**  
  内置 15 种设备类别，覆盖苹果、安卓、Windows 生态的主流弹窗。

- **大规模设备库**  
  预置 500+ 真实设备型号（含配色变体），可无限扩展。

- **智能广播调度**  
  交错轮询四种设备大类（苹果 → 安卓 → 电脑 → 其他），实现多品牌弹窗交替出现。

- **动态 MAC 地址**  
  每包随机生成符合 BLE 规范的静态随机 MAC（64 种合法前缀），避免被黑名单识别。

- **真实设备名伪装**  
  内置 100+ 常见耳机/手环名称池，每包随机更换 GATT 设备名，对抗 iOS 反欺骗机制。

- **功率分层控制**  
  根据不同协议动态调整发射功率（苹果低功耗、Fast Pair 高功率），兼顾覆盖与隐蔽性。

- **LED 状态指示**  
  左右两颗 LED 分别指示当前广播的设备类别（苹果/安卓/其他），便于现场调试。

- **轻量高效**  
  使用 Xorshift32 快速随机数生成器，所有数据包预计算并缓存，O(1) 查表调度，满足嵌入式实时性要求。

---

## 📦 支持的广播协议

| 协议类别               | 触发设备                              | 弹窗效果                         |
|-----------------------|--------------------------------------|----------------------------------|
| **Apple Audio**       | AirPods、Beats 全系列                 | “连接此 AirPods” 弹窗            |
| **Apple Setup**       | AppleTV、HomePod、iPhone 设置         | “设置新的 AppleTV” / “迁移号码”   |
| **Apple Action Modal**| 多种 Continuity 动作                  | NFC 扫描风格弹窗（AutoFill 等）   |
| **Apple iOS17 Crash** | 特定 actionId 组合                    | 可能导致 iOS 17 设备短暂卡顿      |
| **Apple Airtag**      | Airtag / Hermes Airtag                | “查找” 弹窗                       |
| **Apple Not Your Device** | 80+ 颜色型号变体                   | “这不是您的设备” 追踪提示         |
| **Google Fast Pair**  | 467 款主流耳机/音箱（Bose/Sony/JBL） | Android 快速配对弹窗              |
| **Fast Pair Debug**   | 测试用虚拟设备（Flipper Zero 等）     | 调试弹窗                          |
| **Fast Pair Phone Setup** | 手机间数据传输                    | “设置新手机” 弹窗                 |
| **Samsung Easy Setup**| Galaxy Buds / Watch 系列              | 三星手机快速配对弹窗              |
| **Lovespouse**        | 蓝牙音箱（播放/停止指令）              | 音箱控制弹出                      |
| **Microsoft Swift Pair** | Surface / Xbox / 主流耳机           | Windows 10/11 快速配对通知        |

> 详细设备列表请参阅 [`devices.cpp`](devices.cpp) 中的数组定义。

---

## 🧪 适用场景

- **安全测试**：在授权环境中测试员工设备对仿冒弹窗的警觉性。
- **协议分析**：学习 BLE 广播数据包的构造与厂商私有协议逆向。
- **兼容性验证**：验证移动设备是否能正确解析特定厂商的广播格式。
- **教学演示**：无线通信课程中演示 BLE 广播层的工作原理。

---

## 🛠️ 硬件要求

- **开发板**：任意 ESP32 系列（推荐 ESP32-C3 / ESP32-S3）
- **LED**：两个 GPIO 引脚（代码默认 GPIO12、GPIO13），可禁用
- **USB 供电**：5V/500mA 以上

---

## ⚙️ 构建与烧录

### 1. 安装 PlatformIO

本项目使用 [PlatformIO](https://platformio.org/) 管理依赖和编译，请先安装 VSCode 的 PlatformIO 扩展。

### 2. 克隆仓库

```bash
git clone https://github.com/yourusername/EvilESP32.git
cd EvilESP32
```

### 3. 配置平台

在 [`platformio.ini`](platformio.ini) 中选择目标板（默认 `airm2m_core_esp32c3`）：

```ini
[platformio]
default_envs = airm2m_core_esp32c3   # 改为你的板子
```

### 4. 编译 & 上传

```bash
pio run -t upload
```

首次上传可能会自动安装依赖，请确保网络畅通。

### 5. 串口监视（可选）

```bash
pio device monitor -b 115200
```

> **注意**：若使用 USB-CDC 虚拟串口，需在 `build_flags` 中添加 `-D ARDUINO_USB_CDC_ON_BOOT=1`（已默认包含）。

---

## 🔧 配置选项

所有核心参数集中在 [`main.cpp`](main.cpp) 头部，可根据需求调整：

```cpp
#define ENABLE_LOG 1                     // 串口日志开关
#define BROADCAST_DELAY_MIN_MS 100       // 设备间最小延时 (ms)
#define BROADCAST_DELAY_MAX_MS 200       // 设备间最大延时 (ms)
#define ROUND_DELAY_MS 3000              // 每轮广播完成后的休息时间
#define MAX_DEVICES 1000                 // 设备列表最大容量
#define LED_ENABLE 0                     // LED 功能开关
```

- **延时调节**：增大 `BROADCAST_DELAY_MIN/MAX_MS` 可降低广播频率，减少功耗。
- **设备数量**：若需加载超过 1000 个设备，请修改 `MAX_DEVICES` 并确保 Flash 足够。
- **LED 指示**：设为 `0` 可禁用 LED 以节省 GPIO 或降低功耗。

---

## 🧠 工作原理简析

1. **初始化**：随机数种子、BLE 控制器、GATT 服务（用于 Fast Pair）。
2. **设备加载**：将所有内置设备统一转换为 `DeviceDesc` 结构，并预计算广播包。
3. **轮询调度**：按“苹果 → 安卓 → 电脑 → 其他”顺序逐大类轮询，每类内部索引递增。
4. **广播执行**：
   - 生成随机 MAC 和设备名
   - 填充广播数据（Service Data / Manufacturer Data）
   - 设置 PDU 类型（可连接/非连接）
   - 按协议类型执行不同发送策略（单次/多次）
   - 插入设备间随机延时
5. **轮次重置**：所有设备发送完成后，LED 闪烁提示，休息 `ROUND_DELAY_MS` 后进入下一轮。

---

## ⚠️ 法律与道德声明

> **本工具仅限合法授权用途**。使用前请务必阅读并理解以下条款：

1. **教育目的**：本项目旨在帮助安全研究员和开发人员理解 BLE 广播协议，不得用于任何非法活动。
2. **授权测试**：在他人设备上触发弹窗必须事先获得明确许可，否则可能被视为干扰他人正常使用或侵犯隐私。
3. **风险自负**：作者不对因使用本项目导致的任何直接或间接损失承担责任。使用者应自行评估法律风险，并遵守所在地法律法规。
4. **勿用于恶意目的**：禁止用于诈骗、钓鱼、骚扰等行为。如有违反，一切后果由使用者承担。

**使用即视为同意以上声明。**

---

## 📄 许可证

本项目采用 [MIT 许可证](LICENSE)，允许自由修改和分发，但需保留版权声明和免责条款。

---

## 🙏 致谢

- 苹果 Continuity 协议分析参考了 [AppleJuice](https://github.com/ECTO-1A/AppleJuice) 等开源项目
- Google Fast Pair 数据来自官方设备数据库
- 部分设备名池收集自社区贡献

---

**⚠️ 再次提醒**：请在合规前提下使用，让我们共同维护安全的无线环境。
