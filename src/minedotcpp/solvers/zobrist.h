#pragma once

#include <array>
#include <random>
#include <cassert>
#include <array>

using ZobristKey = uint64_t;

constexpr int MaxSize = 16;

template<class T>
using EachWidth = std::array<T, MaxSize>;

template<class T>
using EachHeight = std::array<T, MaxSize>;

template<class T>
using EachFlag = std::array<T, 2>;

struct zobrist_key_hash
{
    size_t operator()(const ZobristKey& key) const {
        return key;
    }
};

template<typename T>
class key_map : public google::dense_hash_map<const ZobristKey, T, zobrist_key_hash>
{
public:
    inline key_map()
    {
        key_map::set_empty_key(1);
        key_map::set_deleted_key(2);
    }
};



class PRNG {

    uint64_t s;

public:
    constexpr PRNG() : s(1070372)
    {
        assert(s);
    }

    constexpr uint64_t rand64() {
        s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
        return s * 2685821657736338717LL;
    }

};

class ZobristKeysClass
{
public:
    EachWidth<EachHeight<EachFlag<ZobristKey>>> Keys{};

    constexpr ZobristKeysClass()
    {
        //auto rng = std::mt19937_64(0);
        auto rng = PRNG();

        for (int width = 0; width < MaxSize; width++)
        {
            for (int height = 0; height < MaxSize; height++)
            {
                Keys[width][height][0] = static_cast<ZobristKey>(rng.rand64());
            }
        }

    }
};

constexpr ZobristKeysClass ZobristKeys = ZobristKeysClass();