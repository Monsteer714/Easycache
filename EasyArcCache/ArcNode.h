//
// Created by Hanhong Wong on 2026/4/14.
//

#ifndef CACHE_ARCNODE_H
#define CACHE_ARCNODE_H

#include <stdio.h>
#include <memory>

#include "ArcLfuPart.h"
#include "ArcLruPart.h"

namespace EasyCache {
    template <typename Key, typename Value>
    class ArcNode {
    private:
        Key key_;
        Value value_;
        size_t accessCount_ = {};
        std::weak_ptr<ArcNode<Key, Value>> prev_;
        std::shared_ptr<ArcNode<Key, Value>> next_;

    public:
        ArcNode(const Key& key, const Value& value) : key_(key), value_(value) {
        }

        const Key& getKey() const {
            return key_;
        }

        const Value& getValue() const {
            return value_;
        }

        const size_t& getAccessCount() const {
            return accessCount_;
        }

        void setValue(const Value& value) {
            value_ = value;
        }

        void increaseAccessCount() {
            accessCount_++;
        }

        std::shared_ptr<ArcNode<Key, Value>> getNext() const {
            return next_;
        }

        std::weak_ptr<ArcNode<Key, Value>> getPrev() const {
            return prev_;
        }

        friend class EasyCache::ArcLruPart<Key, Value>;
        friend class EasyCache::ArcLfuPart<Key, Value>;
    };
};
#endif //CACHE_ARCNODE_H
