#include <catch2/catch.hpp>

#include <Systems/InterpolationLogic.h>

#include <list>
#include <cstdint>

// Minimal stand-in for InterpolationComponent::TimePoint — just needs a Tick.
struct FakePt
{
    uint64_t Tick{};
};

using FakeList = std::list<FakePt>;

// ---- AddPointSorted ---------------------------------------------------------

TEST_CASE("AddPointSorted: appends to empty list", "[interpolation]")
{
    FakeList list;
    AddPointSorted(list, FakePt{42});
    REQUIRE(list.size() == 1);
    REQUIRE(list.front().Tick == 42);
}

TEST_CASE("AddPointSorted: inserts out-of-order points in ascending tick order", "[interpolation]")
{
    FakeList list;
    AddPointSorted(list, FakePt{20});
    AddPointSorted(list, FakePt{10});
    AddPointSorted(list, FakePt{15});

    auto it = list.begin();
    REQUIRE(it->Tick == 10); ++it;
    REQUIRE(it->Tick == 15); ++it;
    REQUIRE(it->Tick == 20);
}

TEST_CASE("AddPointSorted: rejects duplicate tick — list stays size 1", "[interpolation]")
{
    // This was the jitter bug: duplicate movement packets caused stuttering.
    FakeList list;
    AddPointSorted(list, FakePt{10});
    AddPointSorted(list, FakePt{10});
    REQUIRE(list.size() == 1);
}

TEST_CASE("AddPointSorted: many duplicates — only one entry kept", "[interpolation]")
{
    FakeList list;
    for (int i = 0; i < 5; ++i)
        AddPointSorted(list, FakePt{100});
    REQUIRE(list.size() == 1);
}

TEST_CASE("AddPointSorted: prepend before existing entries", "[interpolation]")
{
    FakeList list;
    AddPointSorted(list, FakePt{50});
    AddPointSorted(list, FakePt{10});  // should go to front
    REQUIRE(list.front().Tick == 10);
}

// ---- CalcInterpolationDelta -------------------------------------------------

TEST_CASE("CalcInterpolationDelta: at start tick returns ~0", "[interpolation]")
{
    // currentTick == firstTick => delta == 0
    REQUIRE(CalcInterpolationDelta(100, 200, 100) == Approx(0.0f).margin(0.001f));
}

TEST_CASE("CalcInterpolationDelta: midpoint returns 0.5", "[interpolation]")
{
    REQUIRE(CalcInterpolationDelta(100, 200, 150) == Approx(0.5f).epsilon(0.001f));
}

TEST_CASE("CalcInterpolationDelta: at end tick returns 1.0", "[interpolation]")
{
    REQUIRE(CalcInterpolationDelta(100, 200, 200) == Approx(1.0f).epsilon(0.001f));
}

TEST_CASE("CalcInterpolationDelta: past end tick clamps to 1.0", "[interpolation]")
{
    // This was the snap bug: without clamping, delta > 1 would extrapolate
    // past the target position, causing the actor to overshoot.
    REQUIRE(CalcInterpolationDelta(100, 200, 999) == Approx(1.0f).epsilon(0.001f));
}

TEST_CASE("CalcInterpolationDelta: equal ticks use fallback, never divides by zero", "[interpolation]")
{
    // firstTick == secondTick: tickDelta is 0, uses fallback constant 0.0001f
    const float delta = CalcInterpolationDelta(100, 100, 100);
    REQUIRE(delta > 0.f);
    REQUIRE(delta <= 1.f);
}
