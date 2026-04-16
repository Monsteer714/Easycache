//
// Created by Hanhong Wong on 2026/4/14.
//

#ifndef CACHE_ARCLRUPART_H
#define CACHE_ARCLRUPART_H
#include <mutex>
#include <unordered_map>

#include "ArcNode.h"
namespace EasyCache {
    template <typename Key, typename Value>
    class ArcLruPart {
    public:
        ArcLruPart(size_t capacity, size_t transformThreshold) {
            capacity_ = capacity;
            transformThreshold_ = transformThreshold;
            initLists();
            mainMap_ = std::unordered_map<Key, NodePtr>();
            ghostMap_ = std::unordered_map<Key, NodePtr>();
        };

        bool get(Key key, Value &value, bool& shouldTransform) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!containInMain(key)) {
                return false;
            }

            auto node = mainMap_[key];
            updateNodeToRecent(node);
            value = node->getValue();

            shouldTransform = node->getAccessCount() >= transformThreshold_;

            return true;
        }

        Value get(Key key) {
            Value value{};
            get(key, value);
            return value;
        }

        void put(Key key, Value value) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (containInMain(key)) {
                auto node = mainMap_[key];
                node->setValue(value);
                updateNodeToRecent(node);
                return;
            }

            NodePtr newNode = std::make_shared<Node>(key, value);
            mainMap_.emplace(key, newNode);
            putMainRecent(newNode);

            if (mainMap_.size() > capacity_) {
                removeLeastRecentInMain();
            }
        }

        bool checkGhost(Key key) {
            auto it = ghostMap_.find(key);
            if (it == ghostMap_.end()) {
                return false;
            }
            auto node = it->second;
            removeFromList(node);
            ghostMap_.erase(key);
            return true;
        }

        void increaseCapacity() {
            capacity_++;
        }

        bool decreaseCapacity() {
            if (capacity_ <= 0) {
                return false;
            }
            if (mainMap_.size() >= capacity_ ) {
                removeLeastRecentInMain();
            }
            capacity_--;
            return true;
        }
    private:
        using Node = EasyCache::ArcNode<Key, Value>;
        using NodePtr = std::shared_ptr<Node>;
        using NodeMap = std::unordered_map<Key, NodePtr>;
        std::mutex mutex_;
        size_t capacity_ = {};
        size_t transformThreshold_ = {};
        NodeMap mainMap_ = {};
        NodeMap ghostMap_ = {};

        NodePtr mainHead_ = {};
        NodePtr ghostHead_ = {};

        NodePtr mainTail_ = {};
        NodePtr ghostTail_ = {};
    private:
        void initLists() {
            mainHead_ = std::make_shared<Node>(Key(), Value());
            mainTail_ = std::make_shared<Node>(Key(), Value());
            mainHead_->next_ = mainTail_;
            mainTail_->prev_ = mainHead_;

            ghostHead_ = std::make_shared<Node>(Key(), Value());
            ghostTail_ = std::make_shared<Node>(Key(), Value());
            ghostHead_->next_ = ghostTail_;
            ghostTail_->prev_ = ghostHead_;
        }

        void updateNodeToRecent(NodePtr node) {
            node->increaseAccessCount();
            removeFromList(node);
            putMainRecent(node);
        }

        void removeFromList(NodePtr node) {
            auto prev = node->prev_.lock();
            auto next = node->next_;

            if (!prev || !next) {
                return;
            }

            prev->next_ = next;
            next->prev_ = prev;
        }

        void removeLeastRecentInMain() {
            auto deleteNode = mainTail_->prev_.lock();
            if (!deleteNode || deleteNode == mainHead_) {
                return;
            }
            removeFromList(deleteNode);
            mainMap_.erase(deleteNode->key_);

            if (ghostMap_.size() >= capacity_) {
                removeLeastRecentInGhost();
            }
            putGhostRecent(deleteNode);
        }

        void removeLeastRecentInGhost() {
            auto deleteNode = ghostTail_->prev_.lock();
            if (!deleteNode || deleteNode == ghostHead_) {
                return;
            }
            removeFromList(deleteNode);
            ghostMap_.erase(deleteNode->key_);
        }

        void putMainRecent(NodePtr node) {
            node->next_ = mainHead_->next_;
            node->prev_ = mainHead_;
            mainHead_->next_->prev_ = node;
            mainHead_->next_ = node;
        }

        void putGhostRecent(NodePtr node) {
            node->setAccessCount(1);

            node->next_ = ghostHead_->next_;
            node->prev_ = ghostHead_;
            ghostHead_->next_->prev_ = node;
            ghostHead_->next_ = node;

            ghostMap_.emplace(node->key_, node);
        }

        bool containInMain(const Key& key) {
            return mainMap_.contains(key);
        }

        bool containInGhost(const Key& key) {
            return ghostMap_.contains(key);
        }

    };
}
#endif //CACHE_ARCLRUPART_H