# 编译与运行说明

本项目使用 CMake 构建，支持主流 C++ 编译器。

## 编译步骤
1. 安装 CMake（推荐 3.10 及以上）。
2. 在项目根目录下执行：

```sh
mkdir -p cmake-build-debug
cd cmake-build-debug
cmake ..
make
```

3. 生成的可执行文件位于 `cmake-build-debug/Cache`。

## 运行测试

```sh
./Cache
```

运行后会输出各缓存算法在不同测试场景下的命中率对比。

## 依赖说明
- C++11 及以上标准
- 无第三方库依赖

## 目录结构
- `main.cpp`：主测试程序
- `EasyLruCache.h`、`EasyLfuCache.h` 等：各算法实现
- `EasyArcCache/`：ARC 相关实现
- `output/`：测试输出目录

如有问题请查阅各算法文档或源码注释。
