# 工地扬尘与噪音监测系统

> 基于 STM32F103C8T6 + ESP8266 的边缘监测终端，实时采集 PM2.5、噪音、温湿度并上报至 OneNet 云平台。  
> 本科毕业设计 · [项目详情见博客](https://www.eurekax.space/docs/myproject/%E9%A1%B9%E7%9B%AE1/project1)

## 📁 项目架构

- `Core/`：STM32 启动、中断、系统时钟  
- `BSP/`：最小系统板驱动（OLED、RTC、Flash、PMS7003、XM7903）  
- `Drivers/`：底层驱动（传感器、ESP8266、ST 标准外设库）  
- `Middlewares/`：OLED 显示、MQTT、cJSON、Base64、循环缓冲区等  
- `App/`：云通信、任务调度、UI、数据采集与存储  
- `Config/`：全局配置与密钥管理

> 完整目录结构见[附录](#附录完整代码结构)

## 🛠️ 编译前配置

1. 复制密钥模板：

   ```bash
   cp Config/system_config_example.h Config/system_config.h
   cp Config/system_config_example.c Config/system_config.c
   ```

2. 在 `Config/secrets.c` 中填入：

   - OneNet 凭证（PRODUCT_ID, API_KEY, DEVICE_NAME）
   - Wi-Fi SSID 与密码

3. 确保 `system_config.h/c` 已加入 `.gitignore`

**🛑 切勿提交密钥文件！**

## 附录-完整代码结构

## ▶️ 编译与烧录

- IDE：Keil MDK v5 (ARMCC)
- 打开 My_UGP/project.uvprojx
- 按`main.c`中说明接线
- 编译烧录到烧录 STM32F103C8T6

## 附录：完整代码结构

```text
My_UGP/
├── Core/ # STM32 核心启动代码
│ ├── Inc/ # 核心头文件
│ │ ├── core_cm3.h # Cortex-M3 内核寄存器定义
│ │ ├── stm32f10x.h # STM32F10x 系列寄存器定义
│ │ └── stm32f10x_conf.h # 外设库配置文件
│ └── Src/ # 核心源文件
│ ├── main.c # 主函数入口点
│ ├── startup_stm32f10x.s# 启动文件(8个型号)
│ └── system_stm32f10x.c # 系统时钟配置
│
├── BSP/ # 板级支持包
│ └── stm32f103c8t6_minidev/ # 最小系统板驱动
│  ├── bsp_oled.c/h # OLED显示屏驱动
│  ├── bsp_usart.c/h # 串口通信驱动
│  ├── bsp_rtc.c/h # 实时时钟驱动
│  ├── bsp_flash.c/h # Flash存储操作
│  ├── bsp_pms7003.c/h # PM2.5传感器驱动
│  └── bsp_xm7903.c/h # 噪音传感器驱动
│
├── Drivers/ # 驱动程序库
│ ├── Communication/ # 通信模块驱动
│ │ └── esp8266_drv.c/h # ESP8266 WiFi模块驱动
│ ├── Sensors/ # 传感器驱动
│ │ ├── dht11_drv.c/h # 温湿度传感器驱动
│ │ ├── pms7003_drv.c/h # PM2.5传感器驱动
│ │ └── xm7903_drv.c/h # 噪音传感器驱动
│ └── STM32F10xStdPeriphDriver/ # ST官方标准外设库
│
├── Middlewares/ # 中间件组件
│ ├── Display/ # 显示相关中间件
│ │ ├── Fonts/ # 字体库
│ │ │ └── OLED_Fonts.c/h # OLED显示字体
│ │ └── OLED.c/h # OLED显示核心驱动
│ ├── Protocols/ # 通信协议栈
│ │ └── mqtt/ # MQTT协议实现
│ │ ├── mqtt_kit.c/h # MQTT工具包
│ │ └── MQTTPacket.c/h # 数据包处理
│ ├── Third_Party/ # 第三方库
│ │ ├── cJSON.c/h # JSON解析库
│ │ └── base64.c/h # Base64编码解码
│ └── Utils/ # 工具类组件
│ ├── buffer/ # 缓冲区管理
│ │ └── cbuf_slot.c/h # 循环缓冲区实现
│ ├── crypto/ # 加密算法
│ │ └── CRC16.c/h # CRC16校验算法
│ └── debug/ # 调试工具
│ └── my_assert.c/h # 断言机制
│
├── App/ # 应用逻辑层
│ ├── cloud/ # 云平台通信模块
│ │ ├── onenet_mqtt.c/h # OneNet MQTT通信
│ │ └── onenet_handler.c/h # OneNet平台处理器
│ ├── log/ # 日志记录模块
│ │ └── error_warning_log.c/h # 错误警告日志
│ ├── sensors/ # 传感器管理
│ │ └── sensors_data.c/h # 传感器数据处理
│ ├── storage/ # 数据存储管理
│ │ └── storage.c/h # 存储管理实现
│ ├── system/ # 系统初始化
│ │ └── System_Init.c/h # 系统初始化配置
│ ├── task/ # 任务调度
│ │ └── task_sched.c/h # 任务调度器
│ └── ui/ # 用户界面
│ ├── ui_conten/ # UI内容管理
│ │ └── menu_data.c/h # 菜单数据定义
│ └── ui_framework/ # UI框架
│ └── oled_menu.c/h # OLED菜单实现
│
└── Config/ # 全局配置文件
  ├── bsp_config.h # BSP配置头文件
  └── debug_config.h # 调试配置

```

<!-- 注：本项目部分代码参考或复用了开源项目，包括 cJSON、OLED UI、Base64 编码实现及 MQTT 协议相关逻辑，特此致谢。 -->
