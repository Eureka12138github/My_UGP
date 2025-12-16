# 工地扬尘与噪音监测系统

> 本科毕业设计 · [项目详情见博客](https://www.eurekax.space/docs/myproject/%E9%A1%B9%E7%9B%AE1/project1)

## 🛠️ 编译前配置

1. 复制配置模板：
   ```bash
   cp project_secrets.example.h project_secrets.h
   cp project_secrets.example.c project_secrets.c
   ```
2. 在 `project_secrets.c` 中填入你的：
- OneNet 凭证（PROID / ACCESS_KEY / DEVICE_NAME）
- Wi-Fi 账号密码

3. 确保 `project_secrets.c` 和 `.h` 已加入 `.gitignore`

## ⚙️ 开发环境  
- Keil MDK v5 (ARMCC)  
- STM32F103C8T6 + ESP8266-01S 

> 敏感信息请勿提交到仓库！

<!-- 注：本项目部分代码参考或复用了开源项目，包括 cJSON、OLED UI、Base64 编码实现及 MQTT 协议相关逻辑，特此致谢。 -->   