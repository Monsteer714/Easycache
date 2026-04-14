//
// Created by Hanhong Wong on 2026/4/14.
//

#ifndef CACHE_ARCCACHE_H
#define CACHE_ARCCACHE_H
#include <memory>

#include "ArcLfuPart.h"
#include "ArcLruPart.h"
#include "../EasyCachePolicy.h"

namespace EasyCache {
    template <typename Key, typename Value>
    class ArcCache : public EasyCache::EasyCachePolicy<Key, Value> {
    public:
        ArcCache(unsigned long capacity, unsigned long transformThreshold) : capacity_(capacity),
                                                                             transformThreshold_(transformThreshold),
                                                                             lru(std::make_unique<EasyCache::ArcLruPart<
                                                                                 Key, Value>>(
                                                                                 capacity_, transformThreshold_)),
                                                                             lfu(std::make_unique<EasyCache::ArcLfuPart<
                                                                                 Key, Value>>(
                                                                                 capacity_, transformThreshold_)) {
        }

        ~ArcCache() override = default;

        bool get(Key key, Value& value) override {

        }

        Value get(Key key) override {

        }

        void put(Key key, Value value) override {

        }
    private:
        unsigned long capacity_ = {};
        unsigned long transformThreshold_ = {};
        std::unique_ptr<EasyCache::ArcLruPart<Key, Value>> lru = {};
        std::unique_ptr<EasyCache::ArcLfuPart<Key, Value>> lfu = {};
    };
}
#endif //CACHE_ARCCACHE_H
