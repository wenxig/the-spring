# 存储方案

板载 MicroSD 通过 4-bit SDMMC 常驻挂载，使用 GPIO43/44/39/40/41/42（CLK/CMD/D0-D3）。应用启动时初始化卡并挂载到 `/sdcard`；卡被视为固定介质，首版不实现热拔插恢复。

SQLite 数据库放在 `/sdcard/data/spring.sqlite3`，通过 `espressif/esp-sqlite` 集成。数据库访问集中在 Storage 任务，其他任务通过队列或服务接口提交事务，避免多个 FreeRTOS 任务直接共享连接。启动时创建目录、数据库和 schema；写入采用短事务并定期 checkpoint。

分区表采用 `otadata + ota_0 + ota_1` 双槽回滚布局，内置 FAT 分区约 6MB，用于配置和故障转储。SQLite 主数据位于 SD 卡。
