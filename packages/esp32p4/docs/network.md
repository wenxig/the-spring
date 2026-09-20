# 网络抽象与链路选择

业务组件只依赖 `spring::network` 的拥有型 `Request`、`Response` 和异步便捷入口 `get`、`post`、`put`、`patch`、`del`。请求头保留顺序和重复字段，请求体与响应体使用 `std::vector<std::byte>`，回调在网络任务中执行，每个请求只完成一次。

`network_service` 维护请求队列、取消令牌、超时和 transport 选择。Wi-Fi transport 使用 ESP-Hosted C6 STA 与 `esp_http_client`；蜂窝 transport 使用 EC600MCNLE 的 USB Host CDC-ACM DTE、`esp_modem` PPP 和相同的 HTTP 适配层。请求通过 `esp_netif` 接口名绑定实际承载，业务层不感知链路。

链路顺序固定为 Wi-Fi、蜂窝。Wi-Fi 在连接建立前失败时切换一次蜂窝；请求已连接或开始写入后发生错误时直接返回。4xx、3xx 和已完成的 5xx 作为响应返回，不触发切换。恢复 Wi-Fi 后后续请求再次优先 Wi-Fi。

HTTPS 默认启用证书 bundle 和主机名校验。重定向默认最多 3 次，并限制在同一 origin；跨 origin 的 Location 由调用方自行处理。请求使用完整内存缓冲，未提供 `Content-Length` 时由 transport 根据 body 长度生成。

天气由 `weather_service` 管理：启动立即刷新，随后由 `app_runtime` 每 10 分钟调用一次。请求失败保留上一份有效快照，设备无定位或没有 API key 时保留 fallback 快照。
