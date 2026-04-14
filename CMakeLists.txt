cmake_minimum_required(VERSION 4.1)
project(Cache)

set(CMAKE_CXX_STANDARD 20)


add_executable(Cache main.cpp
        EasyCachePolicy.h
        EasyLruCache.h
        EasyLfuCache.h
        EasyArcCache/ArcLfuPart.h
        EasyArcCache/ArcNode.h
        EasyArcCache/ArcLruPart.h
        EasyArcCache/ArcCache.h)
