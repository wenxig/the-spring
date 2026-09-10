# EC600MCNLE AT 引擎

## 分层

1. `UartTransport`：UART1 GPIO0/GPIO1、DMA、超时和线路恢复。
2. `AtParser`：按行解析最终响应、带数据响应和 URC；处理回显、空行、超长行和二进制边界。
3. `AtTransactionQueue`：单写者事务队列，命令带 token、超时、重试策略和取消；任何时刻只允许一个会改变 modem 状态的事务。
4. `UrcRouter`：将来电、短信、注册、PDP、定位和通话 URC 路由到领域状态机。
5. 领域服务：`RegistrationService`、`DataService`、`LocationService`、`VoiceService`。

## 统一事实模型

所有服务写入同一个 `ModemSnapshot`，包括 SIM、注册状态、CSQ、PDP、IP、定位 fix、通话状态和最后错误。查询接口返回带单调递增 revision 和采样时间的快照；事件同时携带 revision。启动、串口异常、复位和关键命令失败时执行重同步查询，避免本地状态与 modem 实际状态漂移。

## VoLTE

`VoiceService` 封装号码校验、`ATD<number>;`、`ATA`、`ATH`、来电 URC、通话开始/结束和超时回滚。首版使用 EC600X-EVB 板载麦克风和扬声器；ESP32 只控制呼叫状态，不假设 PCM 已引出。

## 定位

`LocationService` 抽象 GNSS/基站定位来源，按固件 AT 手册选择命令，统一输出经纬度、精度、时间、来源和 fix 有效性。定位查询使用独立事务，不能阻塞呼叫和数据事务。

## 数据承载

PDP 激活、网络注册和 socket/HTTP 由 `DataService` 管理；向上只暴露网络客户端接口。Wi-Fi 与蜂窝状态由链路管理器汇总，所有切换和失败原因写入同一快照。
