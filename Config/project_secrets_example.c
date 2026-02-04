// project_secrets_example.c
#include "project_secrets_example.h"

// ⚠️ 注意：此为示例文件，请复制为 project_secrets.c 并填入你的真实信息
// ⚠️ 确保 project_secrets.c 已加入 .gitignore，切勿提交！

// OneNet 凭证（从 OneNet 控制台获取）
const char* ONENET_PROID        = "YOUR_ONENET_PRODUCT_ID_HERE";
const char* ONENET_ACCESS_KEY   = "YOUR_ONENET_ACCESS_KEY_HERE";
const char* ONENET_DEVICE_NAME  = "YOUR_DEVICE_NAME_HERE";

// Wi-Fi 配置（替换成你的网络）
const char* WIFI_CONNECT_CMD = "AT+CWJAP=\"YOUR_WIFI_SSID_HERE\",\"YOUR_WIFI_PASSWORD_HERE\"\r\n";