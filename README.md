# 一个基于事件的 Server 框架

## Demo

1. 提供常用线程原语（基于pthread）
2. 实现了基于 epoll/select 的事件循环
2. 达到 C10K 标准 (2GHz CPU, 100Mbps 网络, 1GB 内存)
3. 支持如下几种
    - 单 Reactor、
    - 单 Reactor + 多 Worker
    - 多 Reactor + 多 Worker 模式
