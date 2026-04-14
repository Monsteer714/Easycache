//
// Created by Hanhong Wong on 2026/4/8.
//

#ifndef CACHE_EASYLRUCACHE_H
#define CACHE_EASYLRUCACHE_H
#include <memory>
#include <unordered_map>

#include "EasyCachePolicy.h"

namespace EasyCache {
    template <class Key, class Value>
    class EasyLruCache;

    template <class Key, class Value>
    class LruNode {
    private:
        Key key_;
        Value value_;
        size_t accessCount_ = {};
        std::weak_ptr<LruNode<Key, Value>> prev_;
        std::shared_ptr<LruNode<Key, Value>> next_;

    public:
        LruNode(const Key& key, const Value& value) : key_(key), value_(value) {
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

        std::shared_ptr<LruNode<Key, Value>> getNext() const {
            return next_;
        }

        std::weak_ptr<LruNode<Key, Value>> getPrev() const {
            return prev_;
        }


        friend class EasyLruCache<Key, Value>;
    };

    template <class Key, class Value>
    class EasyLruCache : public EasyCache::EasyCachePolicy<Key, Value> {
    public:
        using LruNodeType = LruNode<Key, Value>;
        using LruNodePtr = std::shared_ptr<LruNodeType>;
        using LruNodeMap = std::unordered_map<Key, LruNodePtr>;

    private:
        int capacity_ = {};
        std::mutex mutex_;
        LruNodeMap map;
        LruNodePtr dummyHead_;
        LruNodePtr dummyTail_;

    public:
        EasyLruCache(int capacity) : capacity_(capacity) {
            init();
        }

        ~EasyLruCache() override = default;

        bool get(Key key, Value& value) override {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!containsKey(key)) {
                return false;
            }
            value = getNode(key)->getValue();
            return true;
        }

        Value get(Key key) override {
            Value value{};
            get(key, value);
            return value;
        }

        void put(Key key, Value value) override {
            std::lock_guard<std::mutex> lock(mutex_);
            putNode(key, value);
            if (map.size() > capacity_) {
                removeLeastRecent();
            }
        }

        void remove(Key key) {
            LruNodePtr node = getNode(key);
            removeNode(node);
        }

    private:
        void init() {
            dummyHead_ = std::make_shared<LruNodeType>(Key(), Value());
            dummyTail_ = std::make_shared<LruNodeType>(Key(), Value());
            dummyHead_->next_ = dummyTail_;
            dummyTail_->prev_ = dummyHead_;
        }

        LruNodePtr getNode(const Key& key) {
            if (!containsKey(key)) {
                return nullptr;
            }
            auto it = map.find(key);
            auto node = it->second;
            removeNode(node);
            putRecent(node);
            return node;
        }

        void putNode(const Key& key, const Value& value) {
            if (containsKey(key)) {
                auto it = map.find(key);
                auto node = it->second;
                node->setValue(value);
                removeNode(node);
                putRecent(node);
            }
            else {
                auto node = std::make_shared<LruNode<Key, Value>>(key, value);
                putRecent(node);
            }
        }

        void removeNode(LruNodePtr node) {
            if (map.size() > 0) {
                auto prev = node->prev_.lock();
                auto next = node->next_;

                if (!prev || !next) {
                    return;
                }

                prev->next_ = next;
                next->prev_ = prev;
                map.erase(node->getKey());
            }
        }

        void removeLeastRecent() {
            removeNode(dummyTail_->prev_.lock());
        }

        void putRecent(LruNodePtr node) {
            LruNodePtr temp = dummyHead_->next_;
            dummyHead_->next_ = node;
            temp->prev_ = node;
            node->next_ = temp;
            node->prev_ = dummyHead_;
            map[node->getKey()] = node;
        }

        bool containsKey(const Key& key) {
            return map.find(key) != map.end();
        }
    };

    template <class Key, class Value>
    class EasyKLruCache : public EasyLruCache<Key, Value> {
    public:
        EasyKLruCache(int capacity, int history_capacity, int k) : EasyLruCache<Key, Value>(capacity), k_(k),
                                                                   history_cache_(
                                                                       std::make_unique<EasyLruCache<Key, size_t>>(
                                                                           history_capacity)) {
        }

        Value get(Key key) {
            Value value{};
            bool inMainCache = EasyLruCache<Key, Value>::get(key, value);

            size_t history_count = history_cache_->get(key);
            history_count = history_count ? history_count : 0;
            history_cache_->put(key, ++history_count);

            if (inMainCache) {
                return value;
            }

            if (history_count >= k_) {
                auto it = history_map_.find(key);
                if (it != history_map_.end()) {
                    Value storedValue = it->second;

                    history_cache_->remove(key);
                    history_map_.erase(key);

                    EasyLruCache<Key, Value>::put(key, storedValue);

                    return storedValue;
                }
            }

            return value;
        }

        void put(Key key, Value value) {
            Value existingValue{};
            bool inMainCache = EasyLruCache<Key, Value>::get(key, existingValue);

            size_t history_count = history_cache_->get(key);
            history_count = history_count ? history_count : 0;
            history_cache_->put(key, ++history_count);

            if (inMainCache) {
                EasyLruCache<Key, Value>::put(key, value);
                return;
            }

            history_map_[key] = value;

            if (history_count >= k_) {
                history_cache_->remove(key);
                history_map_.erase(key);
                EasyLruCache<Key, Value>::put(key, value);
            }
        }

    private:
        int k_;
        std::unique_ptr<EasyLruCache<Key, size_t>> history_cache_;
        std::unordered_map<Key, Value> history_map_;
    };

    template <class Key, class Value>
    class EasyHashLruCache {
    public:
        EasyHashLruCache(size_t capacity, size_t slices_num) : capacity_(capacity), slices_num_(slices_num) {
            for (size_t i = 0; i < slices_num_; ++i) {
                slices_caches_.emplace_back(std::make_unique<EasyLruCache<Key, Value>>(capacity_));
            }
        }

        bool get(Key key, Value& value) {
            size_t hash = Hash(key) % slices_num_;
            return slices_caches_[hash]->get(key, value);
        }

        void put(Key key, Value value) {
            size_t hash = Hash(key) % slices_num_;
            slices_caches_[hash]->put(key, value);
        }

        Value get(Key key) {
            Value value{};
            get(key, value);
            return value;
        }

        size_t Hash(const Key& key) {
            std::hash<Key> hasher;
            return hasher(key);
        }

    private:
        size_t capacity_;
        size_t slices_num_;
        std::vector<std::unique_ptr<EasyLruCache<Key, Value>>> slices_caches_;
    };
}


#endif //CACHE_EASYLRUCACHE_H
