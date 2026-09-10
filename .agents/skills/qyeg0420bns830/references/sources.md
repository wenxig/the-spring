# 资料来源与证据范围

网页与控制器 PDF 查阅日期：2026-09-09。

## 奇耘原厂页面及附件

- [QYEG0420BNS830 商品页](https://www.qiyun-display.cn/Products_1/72.html)：已读取正文，确认型号、黑白颜色、尺寸、分辨率、SPI、电压、温度、刷新标称值及 SSD1683。
- [QYEG0420BNS830F0_V2.0.pdf](https://omo-oss-file110.thefastfile.com/portal-saas/pg2025011709540898814/cms/file/38d45920-189f-446f-8e9c-ab5a7bf10cc9.pdf)。
- [Raspberry Pi 示例 ZIP](https://omo-oss-file110.thefastfile.com/portal-saas/pg2025011709540898814/cms/file/8e612d81-fd83-4985-82b7-49ee3e84217f.zip)。
- [STM32 驱动资料 ZIP](https://omo-oss-file110.thefastfile.com/portal-saas/pg2025011709540898814/cms/file/qyeg0420bns830-stm32驱动资料.zip)。

三个附件链接来自商品页，下载返回 HTTP 403 / Forbidden Access，内容尚未核验。附件名中的 F0 与实物后缀对应关系也需确认。重新获取时保留版本与校验摘要，核对针序、时序、波形和电源参数后更新技能。

## SSD1683 控制器手册

- [Crystalfontz 索引](https://www.crystalfontz.com/controllers/SolomonSystech/SSD1683/)。
- [SSD1683 PDF 镜像](https://www.crystalfontz.com/controllers/uploaded/SSD1683.pdf)：已读取 Solomon Systech Rev 1.0，Jan 2021，共 49 页。

| 页码 | 核验内容 |
| --- | --- |
| 5 | 分辨率、两块 RAM、SPI、供电和内部温度传感器 |
| 7–8 | 供电及接口信号，RES# 和 BUSY 极性 |
| 10–12 | 三线/四线 SPI、位序及采样关系 |
| 26–37 | 复位、RAM、温度、刷新、休眠与窗口命令 |
| 39–42 | 地址模式与运行流程 |
| 43–45 | 额定值、电流条件、SPI 时序 |

该镜像的 PDF 元数据标题为 SSD1780，正文页眉与内容标识为 SSD1683 Rev 1.0；本技能按正文核验。芯片电气范围与面板规格分别引用。

## 补充工程资料

- [Waveshare 4.2inch e-Paper Module Manual](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module_Manual)：已读取接口、刷新维护、休眠和模块版本说明。
- [Waveshare epd4in2_V2.py](https://github.com/waveshareteam/e-Paper/blob/master/RaspberryPi_JetsonNano/python/lib/waveshare_epd/epd4in2_V2.py)：已读取公开实现，核对 400x300 位打包、RAM 写入、BUSY 等待和刷新命令组织；链接指向可变分支，实际移植时应固定提交版本。

这些同类模块资料用于形成工程假设与验证方案。奇耘面板的针序、转接板电压支持、LUT 和刷新周期需采用本型号资料；商品页之外的本屏原厂附件内容均保持未核验状态。
