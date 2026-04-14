//
// Created by Hanhong Wong on 2026/4/14.
//

#ifndef CACHE_ARCLFUPART_H
#define CACHE_ARCLFUPART_H
#include <mutex>
#include <unordered_map>

#include "ArcNode.h"

namespace EasyCache {
    template <typename Key, typename Value>
    class ArcLfuPart;

    template <typename Key, typename Value>
    class ArcFreqList {
    private:
        using Node = EasyCache::ArcNode<Key, Value>;
        using NodePtr = std::shared_ptr<Node>;
        NodePtr mainHead_;
        NodePtr mainTail_;

    public:
        ArcFreqList() {
            mainHead_ = std::make_shared<Node>(Key(), Value());
            mainTail_ = std::make_shared<Node>(Key(), Value());

            mainHead_->next_ = mainTail_;
            mainTail_->prev_ = mainHead_;
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
            return mainHead_->next_;
        }

        NodePtr getLastNode() {
            return mainTail_->prev_.lock();
        }

        bool isEmpty() {
            return mainHead_->next_ == mainTail_;
        }

        void putFront(NodePtr node) {
            node->next_ = mainHead_->next_;
            node->prev_ = mainHead_;
            mainHead_->next_->prev_ = node;
            mainHead_->next_ = node;
        }

        friend class ArcLfuPart<Key, Value>;
    };

    template <typename Key, typename Value>
    class ArcLfuPart  {
    public:
        ArcLfuPart(size_t capacity, size_t maxAverageFreq = 1000000) : capacity_(capacity), min_freq_(0),
                                                                         maxAverageFreq_(maxAverageFreq) {
            initList();
        };

        bool get(Key key, Value& value)  {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!mainMap_.contains(key)) {
                return false;
            }

            auto node = mainMap_[key];
            auto temp = freqMap_[node->getAccessCount()];
            temp->removeNode(node);
            if (temp->isEmpty() && node->getAccessCount() == min_freq_) {
                min_freq_++;
            }
            node->increaseAccessCount();
            if (freqMap_.contains(node->getAccessCount())) {
                freqMap_[node->getAccessCount()]->putFront(node);
            }
            else {
                auto freq_map = std::make_shared<ArcFreqList<Key, Value>>();
                freqMap_[node->getAccessCount()] = freq_map;
                freq_map->putFront(node);
            }
            value = mainMap_[key]->value_;
            increaseFreq();
            return true;
        }

        Value get(Key key) {
            Value value{};
            get(key, value);
            return value;
        }

        void put(Key key, Value value) {
            if (mainMap_.contains(key)) {
                get(key, value);
                mainMap_[key]->value_ = value;
                return;
            }

            std::lock_guard<std::mutex> lock(mutex_);
            NodePtr node = mainMap_[key];

            if (mainMap_.size() == capacity_) {
                auto temp = freqMap_[min_freq_];
                NodePtr deleteNode = temp->getLastNode();
                temp->removeNode(deleteNode);
                mainMap_.erase(deleteNode->key_);
                decreaseFreq(deleteNode->getAccessCount());
                putRecentInGhost(deleteNode);
            }

            NodePtr newNode = std::make_shared<Node>(key, value);
            mainMap_[key] = newNode;
            min_freq_ = 1;
            if (freqMap_.contains(min_freq_)) {
                freqMap_[min_freq_]->putFront(newNode);
            }
            else {
                auto freqList = std::make_shared<ArcFreqList<Key, Value>>();
                freqMap_[min_freq_] = freqList;
                freqList->putFront(newNode);
            }
            increaseFreq();
        }

        bool checkGhost(const Key& key) {
            auto it = ghostMap_.find(key);
            if (it == ghostMap_.end()) {
                return false;
            }
            auto node = it->second;
            removeGhostNode(node);
            ghostMap_.erase(node->getKey());
            return true;
        }

        void increaseCapacity() {
            capacity_++;
        }

        bool decreaseCapacity() {
            if (capacity_ <= 0) {
                return false;
            }
            if (mainMap_.size() >= capacity_) {
                auto temp = freqMap_[min_freq_];
                NodePtr deleteNode = temp->getLastNode();
                temp->removeNode(deleteNode);
                mainMap_.erase(deleteNode->key_);
                decreaseFreq(deleteNode->getAccessCount());
                putRecentInGhost(deleteNode);
            }
            capacity_--;
            return true;
        }

        bool containsKey(const Key& key) {
            return mainMap_.find(key) != mainMap_.end();
        }
    private:
        using Node = ArcNode<Key, Value>;
        using NodePtr = std::shared_ptr<Node>;
        using NodeMap = std::unordered_map<Key, NodePtr>;
        using FreqMap = std::unordered_map<size_t, std::shared_ptr<ArcFreqList<Key, Value>>>;
        size_t capacity_{0};
        size_t min_freq_ = {0};
        size_t maxAverageFreq_ = {0};
        size_t curAverageFreq_ = {0};
        size_t curTotalFreq_ = {0};
        NodeMap mainMap_;
        NodeMap ghostMap_;

        NodePtr ghostHead_;
        NodePtr ghostTail_;

        FreqMap freqMap_;
        std::mutex mutex_;

        void initList() {
            ghostHead_ = std::make_shared<Node>(Key(), Value());
            ghostTail_ = std::make_shared<Node>(Key(), Value());

            ghostHead_->next_ = ghostTail_;
            ghostTail_->prev_ = ghostHead_;
        }

        void putRecentInGhost(NodePtr node) {
            node->setAccessCount(1);

            node->next_ = ghostHead_->next_;
            node->prev_ = ghostHead_;
            ghostHead_->next_ = node;
            ghostHead_->next_->prev_ = node;

            ghostMap_.emplace(node->getKey(), node);
        }

        bool containInGhost(const Key& key) {
            return ghostMap_.find(key) != ghostMap_.end();
        }

        void removeGhostNode(NodePtr node) {
            auto next = node->next_;
            auto prev = node->prev_.lock();

            if (!prev || !next) {
                return;
            }

            next->prev_ = prev;
            prev->next_ = next;
        }


        void increaseFreq() {
            curTotalFreq_++;
            if (mainMap_.empty()) {
                curAverageFreq_ = 0;
            }
            else {
                curAverageFreq_ = curTotalFreq_ / mainMap_.size();
            }

            if (curAverageFreq_ > maxAverageFreq_) {
                handleAverageFreq();
            }
        }

        void decreaseFreq(size_t num) {
            curTotalFreq_ -= num;
            if (mainMap_.empty()) {
                curAverageFreq_ = 0;
            }
            else {
                curAverageFreq_ = curTotalFreq_ / mainMap_.size();
            }
        }

        void handleAverageFreq() {
            if (mainMap_.empty()) {
                return;
            }
            for (auto it : mainMap_) {
                auto node = it.second;

                if (!node) {
                    continue;
                }
                //remove from list
                auto list = freqMap_[node->getAccessCount()];
                list->removeNode(node);

                size_t decay = maxAverageFreq_ / 2;

                int oldFreq = node->getAccessCount();

                node->setAccessCount(node->getAccessCount() - decay);

                if (node->getAccessCount() <= 0) {
                    node->setAccessCount(1);
                }

                curTotalFreq_ += node->getAccessCount() - oldFreq;

                //add to list
                if (freqMap_.contains(node->getAccessCount())) {
                    freqMap_[node->getAccessCount()]->putFront(node);
                }
                else {
                    auto freqList = std::make_shared<ArcFreqList<Key, Value>>();
                    freqMap_[node->getAccessCount()] = freqList;
                    freqList->putFront(node);
                }
            }
        }
    };
};
#endif //CACHE_ARCLFUPART_H
