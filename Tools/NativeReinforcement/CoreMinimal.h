#pragma once
// Test-only container adapter for compiling the real reinforcement rules without
// UE. This does not verify Unreal ABI, reflection, actors, or UObject lifetime.
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
using int32 = std::int32_t;
using uint8 = std::uint8_t;
#define TEXT(value) value
#define SOULREALTIMEBATTLE_API

struct FName
{
    std::string Value;
    FName() = default;
    FName(const char* Text) : Value(Text) {}
    bool IsNone() const { return Value.empty(); }
    bool LexicalLess(const FName& Other) const { return Value < Other.Value; }
    bool operator==(const FName& Other) const { return Value == Other.Value; }
    bool operator!=(const FName& Other) const { return !(*this == Other); }
};
template<class K, class V> struct TPair { K Key; V Value; };
template<class T> struct TArray : std::vector<T>
{
    using std::vector<T>::vector;
    int32 Num() const { return static_cast<int32>(this->size()); }
    void Add(const T& Value) { this->push_back(Value); }
    template<class P> void Sort(P Predicate) { std::sort(this->begin(), this->end(), Predicate); }
};
template<class T> struct TSet : TArray<T>
{
    void Add(const T& Value)
    {
        if (std::find(this->begin(), this->end(), Value) == this->end()) this->push_back(Value);
    }
};
template<class K, class V> struct TMap : TArray<TPair<K, V>>
{
    const V* Find(const K& Key) const
    {
        for (const auto& Pair : *this) if (Pair.Key == Key) return &Pair.Value;
        return nullptr;
    }
    V& FindOrAdd(const K& Key)
    {
        for (auto& Pair : *this) if (Pair.Key == Key) return Pair.Value;
        this->push_back({Key, V{}});
        return this->back().Value;
    }
    void Add(const K& Key, const V& Value) { FindOrAdd(Key) = Value; }
};
struct FMath
{
    template<class T> static T Max(T A, T B) { return std::max(A, B); }
    template<class T> static T Min(T A, T B) { return std::min(A, B); }
    template<class T> static T Clamp(T V, T Lo, T Hi) { return std::clamp(V, Lo, Hi); }
};
