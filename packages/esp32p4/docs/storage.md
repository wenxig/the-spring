# 存储方案

板载 MicroSD 通过 4-bit SDMMC 常驻挂载，使用 GPIO43/44/39/40/41/42（CLK/CMD/D0-D3）。应用启动时初始化卡并挂载到 `/sdcard`；卡被视为固定介质，首版不实现热拔插恢复。

StorageService 使用 cJSON 解析与序列化 JSON，事件保存在 `/sdcard/data/kv/events.json`。启动时创建目录和初始事件数组，应用通过服务接口追加事件。

分区表采用 `otadata + ota_0 + ota_1` 双槽回滚布局，内置 FAT 分区约 6MB，用于配置和故障转储。事件数据位于 SD 卡。
