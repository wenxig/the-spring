# 设备状态一致性

设备状态采用事件溯源的快照模型：每次硬件确认、URC 或重同步查询生成 revision；UI、SQLite、网络上报都消费同一快照。SQLite 保存带 revision 的状态事件和周期快照，网络上报携带 revision 与采样时间，服务端可识别重复、乱序和过期数据。

状态更新遵循“命令发出、响应确认、快照提交”顺序。预测状态只能作为 pending 标记，不能覆盖 confirmed 值。串口断线、modem 重启、SIM 变化和 PDP 断开会生成 invalidation 事件，并触发有限重试和完整重同步。
