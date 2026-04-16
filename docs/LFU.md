# LFU（最不经常使用）缓存算法

LFU（Least Frequently Used）缓存会淘汰访问频率最低的数据。

## 原理简介
- 每个缓存项维护一个访问计数。
- 淘汰时移除访问次数最少的项。

## 主要接口
- `put(key, value)`: 插入或更新缓存项。
- `get(key, value)`: 查询缓存项，命中返回 true 并增加计数。

## 适用场景
- 访问频率分布稳定，热点数据明显。

## 相关实现
- 详见 `EasyLfuCache.h`。

