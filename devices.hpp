// ============================================================================
// 广播设备定义 — 苹果 Continuity BLE + Google Fast Pair (优化版)
// ============================================================================
// 此文件定义了所有支持的BLE广播协议设备结构体和数据包生成函数声明
// 支持协议: Apple Continuity、Google Fast Pair、Samsung Easy Setup、
//           Lovespouse、Microsoft Swift Pair
#pragma once
#include <Arduino.h>
#include <cstdint>
#include <type_traits>

// ==================== 广播包类型枚举 ====================
// 定义所有支持的BLE广播包类型，每个类型对应不同的协议和数据格式
enum PacketType : uint8_t {
  APPLE_AUDIO = 0,           // 苹果音频设备（AirPods/Beats等，31字节）
  APPLE_SETUP = 1,           // 苹果设置设备（AppleTV/HomePod等，23字节）
  ANDROID_FAST_PAIR = 2,     // Google Fast Pair（Android设备快速配对，12字节）
  APPLE_ACTION_MODAL = 3,    // Apple Continuity 动作弹窗（模拟NFC效果，11字节）
  APPLE_IOS17_CRASH = 4,     // Apple Continuity iOS17崩溃（利用系统漏洞，18字节）
  APPLE_AIRTAG = 5,          // Apple Airtag（触发查找弹窗，31字节）
  APPLE_NOT_YOUR_DEVICE = 6, // Apple Continuity 非你的设备（追踪未知设备，31字节）
  SAMSUNG_EASY_SETUP_BUDS = 7,  // 三星耳机 Easy Setup（Galaxy Buds系列，30字节）
  SAMSUNG_EASY_SETUP_WATCH = 8, // 三星手表 Easy Setup（Galaxy Watch系列，15字节）
  FAST_PAIR_DEBUG = 9,       // Google Fast Pair 调试（测试用设备，12字节）
  FAST_PAIR_NON_PRODUCTION = 10, // Google Fast Pair 非生产（开发测试，12字节）
  FAST_PAIR_PHONE_SETUP = 11,    // Google Fast Pair 手机设置（手机间传输，12字节）
  LOVESPOUSE_PLAY = 12,      // Lovespouse 播放（蓝牙音箱控制，25字节）
  LOVESPOUSE_STOP = 13,      // Lovespouse 停止（停止播放，25字节）
  MICROSOFT_SWIFT_PAIR = 14  // Microsoft Swift Pair（Windows快速配对，变长）
};

// ==================== 广播包长度常量 ====================
// 各协议广播包的固定长度（字节），用于预分配缓冲区和验证数据完整性
constexpr size_t APPLE_AUDIO_PACKET_LEN = 31;         // 苹果音频设备广播包长度
constexpr size_t APPLE_SETUP_PACKET_LEN = 23;         // 苹果设置设备广播包长度
constexpr size_t FAST_PAIR_PACKET_LEN   = 7;          // Google Fast Pair 广播包长度（Service Data格式）
constexpr size_t APPLE_ACTION_MODAL_LEN = 11;         // Action Modal 广播包长度
constexpr size_t APPLE_IOS17_CRASH_LEN  = 18;         // iOS17 Crash 广播包长度
constexpr size_t APPLE_AIRTAG_PACKET_LEN = 31;        // Airtag 广播包长度
constexpr size_t APPLE_NOT_YOUR_DEVICE_LEN = 31;      // Not Your Device 广播包长度
constexpr size_t SAMSUNG_BUDS_PACKET_LEN = 30;        // 三星耳机广播包长度
constexpr size_t SAMSUNG_WATCH_PACKET_LEN = 15;       // 三星手表广播包长度
constexpr size_t LOVESPOUSE_PACKET_LEN = 25;          // Lovespouse 广播包长度（含载荷前缀 FF FF）
constexpr size_t SWIFT_PAIR_PACKET_LEN = 15;          // Swift Pair 最大广播包长度

// ==================== 苹果BLE设备结构体 ====================
// 用于 Apple Audio 和 Apple Setup 两种协议的设备定义
// 使用 __attribute__((packed)) 确保结构体紧凑存储，节省Flash空间
struct __attribute__((packed, aligned(4))) AppleDevice {
  const char* name;     // 设备名（仅用于日志输出，存储在Flash中）
  uint8_t modelId;      // 设备型号ID（决定弹窗显示的设备图标）
  PacketType type;      // 广播包类型（APPLE_AUDIO 或 APPLE_SETUP）
  uint8_t bodySeed;     // 体部数据种子（生成唯一标识，防止设备冲突）
};

// ==================== Google Fast Pair 设备结构体 ====================
// Google Fast Pair 协议使用3字节modelId来标识设备
struct __attribute__((packed, aligned(4))) FastPairDevice {
  const char* name;       // 设备名（仅用于日志输出）
  const uint8_t modelId[3]; // 3字节型号ID（大端序，从Google官方数据库获取）
};

// ==================== 用户自定义广播设备 ====================
// 复用 AppleDevice 结构，方便用户添加自定义设备
typedef AppleDevice CustomDevice;

// ==================== Apple Continuity Action Modal 设备结构体 ====================
// Action Modal 协议使用 actionId 决定弹窗内容（如支付、共享等）
struct __attribute__((packed, aligned(4))) AppleActionModalDevice {
  const char* name;     // 设备名（仅用于日志输出）
  const uint8_t actionId[2]; // 动作ID（决定弹窗类型和内容）
};

// ==================== Apple Continuity iOS17 Crash 设备结构体 ====================
// iOS17 Crash 协议，利用苹果系统漏洞导致设备崩溃
struct __attribute__((packed, aligned(4))) AppleIos17CrashDevice {
  const char* name;     // 设备名（仅用于日志输出）
  const uint8_t actionId[2]; // 动作ID（触发崩溃的具体动作）
};

// ==================== Apple Airtag 设备结构体 ====================
// Airtag 协议，触发 iPhone 的"查找"弹窗
struct __attribute__((packed, aligned(4))) AppleAirtagDevice {
  const char* name;     // 设备名（仅用于日志输出）
  const uint8_t modelId[4]; // 4字节型号ID（包含Airtag型号信息）
};

// ==================== Apple Not Your Device 设备结构体 ====================
// Not Your Device 协议，触发 iPhone 的"这不是你的设备"弹窗
struct __attribute__((packed, aligned(4))) AppleNotYourDevice {
  const char* name;     // 设备名（仅用于日志输出）
  const uint8_t modelId[4]; // 4字节型号ID（包含颜色和型号信息）
};

// ==================== Samsung Easy Setup Buds 设备结构体 ====================
// 三星耳机 Easy Setup 协议
struct __attribute__((packed, aligned(4))) SamsungBudsDevice {
  const char* name;       // 设备名（仅用于日志输出）
  const uint8_t modelId[3]; // 3字节型号ID（三星设备唯一标识）
};

// ==================== Samsung Easy Setup Watch 设备结构体 ====================
// 三星手表 Easy Setup 协议
struct __attribute__((packed, aligned(4))) SamsungWatchDevice {
  const char* name;       // 设备名（仅用于日志输出）
  const uint8_t modelId[2]; // 2字节型号ID
};

// ==================== Lovespouse 设备结构体 ====================
// Lovespouse 协议，用于蓝牙音箱的播放控制
struct __attribute__((packed, aligned(4))) LovespouseDevice {
  const char* name;       // 设备名（仅用于日志输出）
  const uint8_t commandId[6]; // 6字节命令ID（决定播放内容）
};

// ==================== Microsoft Swift Pair 设备结构体 ====================
// Microsoft Swift Pair 协议，用于 Windows 设备快速配对
struct __attribute__((packed, aligned(4))) SwiftPairDevice {
  const char* name;       // 设备名（直接用于广播包内容）
};

// ==================== 编译期类型安全检查 ====================
// 在编译时验证结构体是否满足内存布局要求，防止运行时错误
static_assert(std::is_trivial<AppleDevice>::value,       "AppleDevice must be trivial");
static_assert(std::is_standard_layout<AppleDevice>::value, "AppleDevice must be standard layout");
static_assert(std::is_trivial<FastPairDevice>::value,    "FastPairDevice must be trivial");
static_assert(std::is_standard_layout<FastPairDevice>::value, "FastPairDevice must be standard layout");
static_assert(sizeof(AppleDevice) == sizeof(CustomDevice), "CustomDevice must match AppleDevice");

// ==================== 外部数组声明 ====================
// 设备列表数组声明，实际定义在 devices.cpp 中
// 苹果设备
extern const AppleDevice    ALL_DEVICES[];                  // 苹果设备总列表（音频+设置）
extern const int            NUM_APPLE_DEVICES;              // 苹果设备数量
extern const FastPairDevice FAST_PAIR_DEVICES[];            // Google Fast Pair 设备列表
extern const int            NUM_FASTPAIR_DEVICES;           // Fast Pair 设备数量
// extern const CustomDevice   CUSTOM_DEVICES[];               // 用户自定义设备列表（已迁移到 ALL_DEVICES）
// extern const int            NUM_CUSTOM_DEVICES;             // 自定义设备数量（已迁移）

// Apple Continuity 协议扩展设备
extern const AppleActionModalDevice APPLE_ACTION_MODAL_DEVICES[];  // Action Modal 设备列表
extern const int                   NUM_APPLE_ACTION_MODAL_DEVICES; // Action Modal 设备数量
extern const AppleIos17CrashDevice APPLE_IOS17_CRASH_DEVICES[];    // iOS17 Crash 设备列表
extern const int                   NUM_APPLE_IOS17_CRASH_DEVICES;   // iOS17 Crash 设备数量
extern const AppleAirtagDevice     APPLE_AIRTAG_DEVICES[];          // Airtag 设备列表
extern const int                   NUM_APPLE_AIRTAG_DEVICES;        // Airtag 设备数量
extern const AppleNotYourDevice    APPLE_NOT_YOUR_DEVICE_DEVICES[]; // Not Your Device 设备列表
extern const int                   NUM_APPLE_NOT_YOUR_DEVICE_DEVICES; // Not Your Device 设备数量

// 三星设备
extern const SamsungBudsDevice     SAMSUNG_BUDS_DEVICES[];          // 三星耳机设备列表
extern const int                   NUM_SAMSUNG_BUDS_DEVICES;        // 三星耳机设备数量
extern const SamsungWatchDevice    SAMSUNG_WATCH_DEVICES[];         // 三星手表设备列表
extern const int                   NUM_SAMSUNG_WATCH_DEVICES;       // 三星手表设备数量

// Google Fast Pair 扩展设备
extern const FastPairDevice        FAST_PAIR_DEBUG_DEVICES[];       // Fast Pair 调试设备列表
extern const int                   NUM_FAST_PAIR_DEBUG_DEVICES;     // Fast Pair 调试设备数量
extern const FastPairDevice        FAST_PAIR_NON_PRODUCTION_DEVICES[]; // Fast Pair 非生产设备列表
extern const int                   NUM_FAST_PAIR_NON_PRODUCTION_DEVICES; // Fast Pair 非生产设备数量
extern const FastPairDevice        FAST_PAIR_PHONE_SETUP_DEVICES[];    // Fast Pair 手机设置设备列表
extern const int                   NUM_FAST_PAIR_PHONE_SETUP_DEVICES;   // Fast Pair 手机设置设备数量

// Lovespouse 设备
extern const LovespouseDevice      LOVESPOUSE_PLAY_DEVICES[];       // Lovespouse 播放设备列表
extern const int                   NUM_LOVESPOUSE_PLAY_DEVICES;     // Lovespouse 播放设备数量
extern const LovespouseDevice      LOVESPOUSE_STOP_DEVICES[];       // Lovespouse 停止设备列表
extern const int                   NUM_LOVESPOUSE_STOP_DEVICES;     // Lovespouse 停止设备数量

// Microsoft Swift Pair 设备
extern const SwiftPairDevice       SWIFT_PAIR_DEVICES[];            // Swift Pair 设备列表
extern const int                   NUM_SWIFT_PAIR_DEVICES;          // Swift Pair 设备数量

// ==================== 数据包生成函数声明 ====================
// 根据设备信息生成对应的BLE广播包数据
// 参数:
//   device - 设备信息结构体引用
//   buffer - 输出缓冲区（调用方负责分配足够空间）
//   outLength - 输出生成的包长度

// 苹果设备广播包生成
void generatePacket(const AppleDevice& device, uint8_t* buffer, size_t& outLength);
// Google Fast Pair 广播包生成
void generateFastPairPacket(const FastPairDevice& device, uint8_t* buffer, size_t& outLength);
// 用户自定义设备广播包生成（复用AppleDevice逻辑）
void generateCustomPacket(const CustomDevice& device, uint8_t* buffer, size_t& outLength);

// Apple Continuity 协议扩展广播包生成
void generateAppleActionModalPacket(const AppleActionModalDevice& device, uint8_t* buffer, size_t& outLength);
void generateAppleIos17CrashPacket(const AppleIos17CrashDevice& device, uint8_t* buffer, size_t& outLength);
void generateAppleAirtagPacket(const AppleAirtagDevice& device, uint8_t* buffer, size_t& outLength);
void generateAppleNotYourDevicePacket(const AppleNotYourDevice& device, uint8_t* buffer, size_t& outLength);

// 三星设备广播包生成
void generateSamsungBudsPacket(const SamsungBudsDevice& device, uint8_t* buffer, size_t& outLength);
void generateSamsungWatchPacket(const SamsungWatchDevice& device, uint8_t* buffer, size_t& outLength);

// Lovespouse 和 Swift Pair 广播包生成
void generateLovespousePacket(const LovespouseDevice& device, uint8_t* buffer, size_t& outLength);
void generateSwiftPairPacket(const SwiftPairDevice& device, uint8_t* buffer, size_t& outLength);