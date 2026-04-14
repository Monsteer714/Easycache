//
// Created by Hanhong Wong on 2026/4/9.
//

#ifndef CACHE_EASYLFUCACHE_H
#define CACHE_EASYLFUCACHE_H
#include <memory>
#include <mutex>
#include <unordered_map>

#include "EasyCachePolicy.h"


namespace EasyCache {
    template <typename Key, typename Value>
    class EasyLfuCache;

    template <typename Key, typename Value>
    class FreqList {
    private:
        struct Node {
            Key key_;
            Value value_;
            int freq_;
            std::weak_ptr<Node> prev_;
            std::shared_ptr<Node> next_;

            Node(Key key, Value value, size_t freq) : key_(key), value_(value), freq_(freq) {
            }

            Node(Key key, Value value) : key_(key), value_(value), freq_(1) {
            }
        };

        using NodePtr = std::shared_ptr<Node>;
        NodePtr dummyHead_;
        NodePtr dummyTail_;

    public:
        FreqList() {
            dummyHead_ = std::make_shared<Node>(Key(), Value());
            dummyTail_ = std::make_shared<Node>(Key(), Value());

            dummyHead_->next_ = dummyTail_;
            dummyTail_->prev_ = dummyHead_;
        }

        void removeNode(NodePtr node) {
            auto prev = node->prev_.lock();
            auto next = node->next_;
            if (!prev || !next) {
                return;
            }

            prev->next_ = next;
            next->prev_ = prev;
        }

        NodePtr getFirstNode() {
            return dummyHead_->next_;
        }

        NodePtr getLastNode() {
            return dummyTail_->prev_.lock();
        }

        bool isEmpty() {
            return dummyHead_->next_ == dummyTail_;
        }

        void putFront(NodePtr node) {
            node->next_ = dummyHead_->next_;
            node->prev_ = dummyHead_;
            dummyHead_->next_->prev_ = node;
            dummyHead_->next_ = node;
        }

        friend class EasyLfuCache<Key, Value>;
    };

    template <typename Key, typename Value>
    class EasyLfuCache : public EasyCache::EasyCachePolicy<Key, Value> {
    public:
        EasyLfuCache(size_t capacity, size_t maxAverageFreq = 1000000) : capacity_(capacity), min_freq_(0),
                                                                         maxAverageFreq_(maxAverageFreq) {
        };

        ~EasyLfuCache() override = default;

        bool get(Key key, Value& value) override {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!nodeMap_.contains(key)) {
                return false;
            }

            auto node = nodeMap_[key];
            auto temp = freqMap_[node->freq_];
            temp->removeNode(node);
            if (temp->isEmpty() && node->freq_ == min_freq_) {
                min_freq_++;
            }
            node->freq_++;
            if (freqMap_.contains(node->freq_)) {
                freqMap_[node->freq_]->putFront(node);
            }
            else {
                auto freq_map = std::make_shared<FreqList<Key, Value>>();
                freqMap_[node->freq_] = freq_map;
                freq_map->putFront(node);
            }
            value = nodeMap_[key]->value_;
            increaseFreq();
            return true;
        }

        Value get(Key key) override {
            Value value{};
            get(key, value);
            return value;
        }

        void put(Key key, Value value) override {
            if (nodeMap_.contains(key)) {
                get(key, value);
                nodeMap_[key]->value_ = value;
                return;
            }

            NodePtr node = nodeMap_[key];

            if (nodeMap_.size() == capacity_) {
                auto temp = freqMap_[min_freq_];
                NodePtr deleteNode = temp->getLastNode();
                temp->removeNode(deleteNode);
                nodeMap_.erase(deleteNode->key_);
                decreaseFreq(deleteNode->freq_);
            }

            NodePtr newNode = std::make_shared<Node>(key, value);
            nodeMap_[key] = newNode;
            min_freq_ = 1;
            if (freqMap_.contains(min_freq_)) {
                freqMap_[min_freq_]->putFront(newNode);
            }
            else {
                auto freqList = std::make_shared<FreqList<Key, Value>>();
                freqMap_[min_freq_] = freqList;
                freqList->putFront(newNode);
            }
            increaseFreq();
        }

    private:
        using Node = typename FreqList<Key, Value>::Node;
        using NodePtr = FreqList<Key, Value>::NodePtr;
        using NodeMap = std::unordered_map<Key, NodePtr>;
        using FreqMap = std::unordered_map<size_t, std::shared_ptr<FreqList<Key, Value>>>;
        size_t capacity_ {0};
        size_t min_freq_ = {0};
        size_t maxAverageFreq_ = {0};
        size_t curAverageFreq_ = {0};
        size_t curTotalFreq_ = {0};
        NodeMap nodeMap_;
        FreqMap freqMap_;
        std::mutex mutex_;

        void increaseFreq() {
            curTotalFreq_++;
            if (nodeMap_.empty()) {
                curAverageFreq_ = 0;
            } else {
                curAverageFreq_ = curTotalFreq_ / nodeMap_.size();
            }

            if (curAverageFreq_ > maxAverageFreq_) {
                handleAverageFreq();
            }
        }

        void decreaseFreq(size_t num) {
            curTotalFreq_ -= num;
            if (nodeMap_.empty()) {
                curAverageFreq_ = 0;
            } else {
                curAverageFreq_ = curTotalFreq_ / nodeMap_.size();
            }
        }

        void handleAverageFreq() {
            if (nodeMap_.empty()) {
                return;
            }
            for (auto it : nodeMap_) {
                auto node = it.second;

                if (!node) {
                    continue;
                }
                //remove from list
                auto list = freqMap_[node->freq_];
                list->removeNode(node);

                size_t decay = maxAverageFreq_ / 2;

                int oldFreq = node->freq_;

                node->freq_ -= decay;

                if (node->freq_ <= 0) {
                    node->freq_ = 1;
                }

                curTotalFreq_ += node->freq_ - oldFreq;

                //add to list
                if (freqMap_.contains(node->freq_)) {
                    freqMap_[node->freq_]->putFront(node);
                } else {
                    auto freqList = std::make_shared<FreqList<Key, Value>>();
                    freqMap_[node->freq_] = freqList;
                    freqList->putFront(node);
                }
            }
        }

    };

    template <typename Key, typename Value>
    class EasyHashLfuCache : public EasyCachePolicy<Key, Value> {
    public:
        EasyHashLfuCache(size_t capacity, size_t slice_num) : capacity_(capacity), slice_num_(slice_num) {
            for (size_t i = 0; i < slice_num_; i++) {
                caches_.emplace_back(std::make_unique<EasyLfuCache<Key, Value>>(capacity_));
            }
        }

        bool get(Key key, Value& value) {
            size_t index = Hash(key) % slice_num_;
            return caches_[index]->get(key, value);
        }

        void put(Key key, Value value) {
            size_t index = Hash(key) % slice_num_;
            caches_[index]->put(key, value);
        }

        Value get(Key key) {
            Value value{};
            size_t index = Hash(key) % slice_num_;
            caches_[index]->get(key, value);
            return value;
        }

        size_t Hash(Key key) {
            std::hash<Key> hasher;
            return hasher(key);
        }

    private:
        size_t capacity_;
        size_t slice_num_;
        std::vector<std::unique_ptr<EasyLfuCache<Key, Value>>> caches_;
    };
}

#endif //CACHE_EASYLFUCACHE_H
