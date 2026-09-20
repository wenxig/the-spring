# Progress

- 2026-09-20：读取用户参考图、核对两个分支和现有改动。
- 主目录 vp install 通过。
- 原分支独立工作树已建立，准备构建仿真器。

- 用户确认保留恢复后的代码；本轮全部操作在主目录执行。
- 完成定位文字实际字宽测量，TinyTTF 13–10 px 自动选择字号，极长名称显示省略号；保留日期和高考行坐标。
- 三版布局各四种数据状态，共 12 张 400×300 单色预览。检查长地名、12/31、负温度、缺省状态和高考期间画面。
- Clang 22 / C++26 主机构建通过，CTest 7/7；UI 编译器 pytest 8/8，ruff 通过。
- main.cpp 与 ui_bindings.cpp 的 clang-format 和 clang-tidy 通过。预览程序增加 std::exception 错误报告。
- vp check 受缺失 packages/app/src/index.css 阻断；vp test 无匹配测试；vp env doctor 全部通过。
- ESP-IDF 6.0.2 / LVGL 9.5.0 构建通过，固件 1,558,496 字节，OTA app 分区余量 70%。既有 frame_packet 未使用警告和主机 LVGL 重复链接库警告保留记录。
- /dev/cu.usbmodem141101 烧录成功，Hash of data verified；读取重启日志确认 SD 挂载 ESP_OK、字体和十个 SVG 均存在、UI assets ready。
- 板端日志确认 Wi-Fi 在线与 NTP 同步，首帧 mode=full，随后分钟变化再次完成全刷。监测期间无崩溃、BUSY 超时或字库加载失败。
- C6 仍报告 host=3.0.7 / coprocessor=0.0.0 版本告警，兼容模式下 Wi-Fi 与 NTP 成功；实物画面尚待用户观察，串口验证不包含光学效果。
- 定位服务继续尝试 GPS/基站定位；天气依赖定位，属于当前已有的数据链路问题。

## 关键板端证据

```text
storage: SD card mount result: ESP_OK
storage: UI assets ready
display: present area=400x300+0+0 mode=full partial_count=0 limit=20
wifi: Wi-Fi connected, IPv4=192.168.50.127
wifi: clock synchronized from Wi-Fi NTP
display: present area=48x54+184+62 mode=full partial_count=0 limit=20
app_manager: ui route=CLOCK dirty=184x62+48+54 frame=00a36581
```
