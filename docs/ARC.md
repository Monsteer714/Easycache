# ARC（自适应替换缓存）算法

ARC（Adaptive Replacement Cache）结合了 LRU 和 LFU 的优点，能自适应不同访问模式。

## 原理简介
- 同时维护 LRU 和 LFU 两种缓存列表。
- 根据访问模式动态调整两者比例。

## 主要接口
- `put(key, value)`: 插入或更新缓存项。
- `get(key, value)`: 查询缓存项。

## 适用场景
- 访问模式变化频繁。

## 相关实现
- 详见 `EasyArcCache/ArcCache.h`。

