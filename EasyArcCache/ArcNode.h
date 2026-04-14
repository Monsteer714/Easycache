//
// Created by Hanhong Wong on 2026/4/14.
//

#ifndef CACHE_ARCNODE_H
#define CACHE_ARCNODE_H

#include <memory>

namespace EasyCache {

    template <typename Key, typename Value>
    class ArcNode {
    public:
        Key key_;
        Value value_;
        size_t accessCount_ = {1};
        std::weak_ptr<ArcNode<Key, Value>> prev_;
        std::shared_ptr<ArcNode<Key, Value>> next_;

    public:
        ArcNode() {next_ = nullptr;}

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

        void setAccessCount(const size_t& count) {
            this->accessCount_ = count;
        }

        std::shared_ptr<ArcNode<Key, Value>> getNext() const {
            return next_;
        }

        std::weak_ptr<ArcNode<Key, Value>> getPrev() const {
            return prev_;
        }

        template<typename K, typename V> friend class ArcLruPart;
        template<typename K, typename V> friend class ArcLfuPart;
    };
};
#endif //CACHE_ARCNODE_H
