//
// Created by Hanhong Wong on 2026/4/8.
//

#ifndef CACHE_CACHEPOLICY_H
#define CACHE_CACHEPOLICY_H

namespace EasyCache {

template <class Key, class Value>
class EasyCachePolicy {
public:
    virtual ~EasyCachePolicy() = default;

    virtual bool get(Key key, Value& value) = 0;

    virtual Value get(Key key) = 0;

    virtual void put(Key key, Value value) = 0;
};

}
#endif //CACHE_CACHEPOLICY_H