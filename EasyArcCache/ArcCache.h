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
                                                                             lru_(std::make_unique<EasyCache::ArcLruPart<
                                                                                 Key, Value>>(
                                                                                 capacity_, transformThreshold_)),
                                                                             lfu_(std::make_unique<EasyCache::ArcLfuPart<
                                                                                 Key, Value>>(
                                                                                 capacity_, transformThreshold_)) {
        }

        ~ArcCache() override = default;

        bool get(Key key, Value& value) override {
            checkGhost(key);

            bool putToLfu = false;

            if (lru_->get(key, value, putToLfu)) {
                if (putToLfu) {
                    lfu_->put(key, value);
                }
                return true;
            }

            return lfu_->get(key, value);
        }

        Value get(Key key) override {
            Value value{};
            get(key, value);
            return value;
        }

        void put(Key key, Value value) override {
            checkGhost(key);

            lru_->put(key, value);
            if (lfu_->contain(key)) {
                lfu_->put(key, value);
            }
        }
    private:
        unsigned long capacity_ = {};
        unsigned long transformThreshold_ = {};
        std::unique_ptr<EasyCache::ArcLruPart<Key, Value>> lru_ = {};
        std::unique_ptr<EasyCache::ArcLfuPart<Key, Value>> lfu_ = {};

        void checkGhost(Key key) {
            if (lru_->checkGhost(key)) {
                if (lfu_->decreaseCapacity()) {
                    lru_->increaseCapacity();
                }
            }
            if (lfu_->checkGhost(key)) {
                if (lru_->decreaseCapacity()) {
                    lfu_->increaseCapacity();
                }
            }
        }
    };
}
#endif //CACHE_ARCCACHE_H
