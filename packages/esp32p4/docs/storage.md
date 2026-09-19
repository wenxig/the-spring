# 存储方案

板载 MicroSD 通过 4-bit SDMMC 常驻挂载，使用 GPIO43/44/39/40/41/42（CLK/CMD/D0-D3）。应用启动时初始化卡并挂载到 `/sdcard`；卡被视为固定介质，首版不实现热拔插恢复。

SD 卡占用 SDMMC slot 0，C6 ESP-Hosted 占用 slot 1。ESP-IDF 6 的共享控制器由 ESP-Hosted 初始化；主任务等待传输层就绪后挂载 SD 卡。挂载使用 slot 级释放接口，保留 C6 所在 slot；文件句柄上限为 16，覆盖 TinyTTF 按字号缓存和图标读取。

StorageService 使用 cJSON 解析与序列化 JSON，事件保存在 `/sdcard/db/events.json`。启动时创建目录和初始事件数组，应用通过服务接口追加事件。

声明式 UI 的大资源放在 SD 卡，启动挂载后由 LVGL 文件系统按需解析：

```text
/fonts/HYWenHei-65W-3.ttf
/icons/sunny-outline.svg
/icons/partly-sunny-outline.svg
/icons/cloudy-outline.svg
/icons/rainy-outline.svg
/icons/thunderstorm-outline.svg
/icons/snow-outline.svg
/icons/moon-outline.svg
/icons/cloudy-night-outline.svg
/icons/help-circle-outline.svg
/icons/location-outline.svg
```

准备卡片时执行：

```sh
mkdir -p /Volumes/<SD卡卷名>/fonts /Volumes/<SD卡卷名>/icons
cp assets/HYWenHei-65W-3.ttf /Volumes/<SD卡卷名>/fonts/
cp assets/ionicons5-weather/*.svg /Volumes/<SD卡卷名>/icons/
```

启动日志会输出每个资源的存在性以及 `UI assets ready`。资源缺失时固件仍可启动，但中文字体和对应图标无法正常渲染。

分区表采用 `otadata + ota_0 + ota_1` 双槽回滚布局，内置 FAT 分区约 6MB，用于配置和故障转储。事件数据位于 SD 卡。
