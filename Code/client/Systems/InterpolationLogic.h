#pragma once
#include <cstdint>

// Sorted insert with duplicate-tick guard.
// Works with any list/vector<T> where T has a uint64_t Tick field.
template <typename TList, typename TPoint>
void AddPointSorted(TList& aList, const TPoint& acPoint) noexcept
{
    auto itor = std::begin(aList);
    const auto end = std::cend(aList);
    while (itor != end)
    {
        if (itor->Tick == acPoint.Tick) return;
        if (itor->Tick > acPoint.Tick) { aList.insert(itor, acPoint); return; }
        ++itor;
    }
    aList.push_back(acPoint);
}

// Returns a delta in [0, 1] for aCurrentTick positioned between aFirstTick and aSecondTick.
inline float CalcInterpolationDelta(uint64_t aFirstTick, uint64_t aSecondTick, uint64_t aCurrentTick) noexcept
{
    const float tickDelta = static_cast<float>(aSecondTick - aFirstTick);
    float delta = 0.0001f;
    if (tickDelta > 0.f)
        delta = 1.f / tickDelta * static_cast<float>(aCurrentTick - aFirstTick);
    return delta < 1.0f ? delta : 1.0f;
}
