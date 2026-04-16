//
// Created by Hanhong Wong on 2026/4/14.
//

#ifndef CACHE_ARCLFUPART_H
#define CACHE_ARCLFUPART_H
#include <list>
#include <map>
#include <mutex>
#include <unordered_map>

#include "ArcNode.h"

namespace EasyCache {
    template <typename Key, typename Value>
    class ArcLfuPart {
    public:
        ArcLfuPart(size_t capacity) {
            capacity_ = capacity;
            min_freq_ = 0;

        };

        bool get(Key key, Value &value) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!containsInMain(key)) {
                return false;
            }
            auto node = mainMap_[key];
            updateNodeToRecent(node);
            return true;
        }

        Value get(Key key) {
            Value value{};
            get(key, value);
            return value;
        }

        void put(Key key, Value value) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (containsInMain(key)) {
                auto node = mainMap_[key];
                node->value_ = value;
                updateNodeToRecent(node);
                return;
            }

            if (capacity_ > 0 && mainMap_.size() == capacity_) {
                removeLeastFreqMain();
            }

            min_freq_ = 1;
            NodePtr newNode = std::make_shared<ArcNode<Key, Value>>(key, value);
            if (freqMap_.find(min_freq_) != freqMap_.end()) {
                freqMap_[min_freq_].push_front(newNode);
            } else {
                FreqList freqList = {};
                freqList.push_front(newNode);
                freqMap_[min_freq_] = freqList;
            }
        }

        bool checkGhost(Key key) {
            auto it = ghostMap_.find(key);
            if (it != ghostMap_.end()) {
                auto node = it->second;
                removeFromListGhost(node);
                ghostMap_.erase(it);
                return true;
            }
            return false;
        }

        bool containsInMain(Key key) {
            return mainMap_.contains(key);
        }

        void increaseCapacity() {
            capacity_++;
        }

        bool decreaseCapacity() {
            if (capacity_ <= 0) {
                return false;
            }
            if (mainMap_.size() == capacity_ ) {
                removeLeastFreqMain();
            }
            capacity_--;
            return true;
        }
    private:
        using Node = ArcNode<Key, Value>;
        using NodePtr = std::shared_ptr<Node>;
        using NodeMap = std::unordered_map<Key, NodePtr>;
        using FreqList = std::list<NodePtr>;
        using FreqMap = std::map<size_t, FreqList>;
        size_t capacity_ = {};
        size_t min_freq_ = {};
        std::mutex mutex_;

        NodeMap mainMap_ = {};
        NodeMap ghostMap_ = {};
        FreqMap freqMap_ = {};

        NodePtr ghostHead_;
        NodePtr ghostTail_;

        void updateNodeToRecent(NodePtr node) {
            auto oldFreq = node->accessCount_;
            node->increaseAccessCount();
            auto newFreq = node->accessCount_;

            removeFromListMain(node);
            if (freqMap_[oldFreq].empty() && oldFreq == min_freq_) {
                min_freq_++;
            }

            if (freqMap_.find(newFreq) != freqMap_.end()) {
                freqMap_[newFreq].push_front(node);
            } else {
                FreqList freqList = {};
                freqList.push_front(node);
                freqMap_[newFreq] = freqList;
            }
        }

        void initGhostList() {
            ghostHead_ = std::make_shared<Node>();
            ghostTail_ = std::make_shared<Node>();

            ghostHead_->next = ghostTail_;
            ghostTail_->prev = ghostHead_;
        }

        void removeFromListMain(NodePtr node) {
            auto& list = freqMap_[node->accessCount_];
            list.remove(node);
        }

        void removeFromListGhost(NodePtr node) {
            auto next = node->next_;
            auto prev = node->prev_.lock();

            if (!prev || !next) {
                return;
            }

            prev->next_ = next;
            next->prev_ = prev;
        }

        void putFreqMain(NodePtr node) {
            auto list = freqMap_[node->accessCount_];
            list.push_front(node);
        }

        void putRecentGhost(NodePtr node) {
            node->next_ = ghostHead_->next_;
            node->prev_ = ghostHead_;
            ghostHead_->next_->prev_ = node;
            ghostHead_->next_ = node;
        }

        void removeLeastFreqMain() {
            auto minFreqList = freqMap_[min_freq_];
            auto deleteNode = minFreqList.back();
            removeFromListMain(deleteNode);
            mainMap_.erase(deleteNode->key_);

            if (ghostMap_.size() >= capacity_) {
                removeLeastRecentGhost();
            }
            putRecentGhost(deleteNode);
        }

        void removeLeastRecentGhost() {
            auto deleteNode = ghostTail_->prev_.lock();
            removeFromListGhost(deleteNode);
            ghostMap_.erase(deleteNode->key_);
        }
    };
};
#endif //CACHE_ARCLFUPART_H
