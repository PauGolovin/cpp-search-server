#pragma once
#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>

using namespace std;

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Bucket {
        mutex mutex_;
        map<Key, Value> map_;
    };

public:
    static_assert(is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        lock_guard<mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            : guard(bucket.mutex_)
            , ref_to_value(bucket.map_[key]) {
        }
    };

    explicit ConcurrentMap(size_t bucket_count)
        : buckets_(bucket_count) {
    }

    Access operator[](const Key& key) {
        auto& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return { key, bucket };
    }

    map<Key, Value> BuildOrdinaryMap() {
        map<Key, Value> result;
        for (auto& [mutex, map] : buckets_) {
            lock_guard g(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

    size_t Erase(const Key & key) {
        auto& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        lock_guard g(bucket.mutex_);
        size_t result = bucket.map_.erase(key);
        return result;
    }

private:
    vector<Bucket> buckets_;
};