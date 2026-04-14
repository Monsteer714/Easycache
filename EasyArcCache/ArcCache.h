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
        ArcCache(unsigned long capacity, unsigned long transformThreshold = 2) : capacity_(capacity),
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
            checkGhost(key);

            bool putToLfu = false;

            if (lru->get(key, value, putToLfu)) {
                if (putToLfu) {
                    lfu->put(key, value);
                }
                return true;
            }

            return lfu->get(key, value);
        }

        Value get(Key key) override {
            Value value{};
            get(key, value);
            return value;
        }

        void put(Key key, Value value) override {
            checkGhost(key);

            lru->put(key, value);
            if (lfu->containsKey(key)) {
                lfu->put(key, value);
            }
        }
    private:
        unsigned long capacity_ = {};
        unsigned long transformThreshold_ = {};
        std::unique_ptr<EasyCache::ArcLruPart<Key, Value>> lru = {};
        std::unique_ptr<EasyCache::ArcLfuPart<Key, Value>> lfu = {};

        void checkGhost(Key key) {
            if (lru->checkGhost(key)) {
                if (lfu->decreaseCapacity()) {
                    lru->increaseCapacity();
                }
            }
            if (lfu->checkGhost(key)) {
                if (lru->decreaseCapacity()) {
                    lfu->increaseCapacity();
                }
            }
        }
    };
}
#endif //CACHE_ARCCACHE_H
