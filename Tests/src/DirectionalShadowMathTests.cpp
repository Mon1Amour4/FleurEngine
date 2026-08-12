#include "Fleur/Lux/Vulkan/DirectionalShadowMath.hpp"

#include "gtest/gtest.h"

#include <array>

TEST(DirectionalShadowMathTest, SnapsToNearestTexelForPositiveAndNegativeCoordinates)
{
    EXPECT_FLOAT_EQ(Fleur::ShadowMath::SnapToTexel(0.49f, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(Fleur::ShadowMath::SnapToTexel(0.51f, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(Fleur::ShadowMath::SnapToTexel(-0.49f, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(Fleur::ShadowMath::SnapToTexel(-0.51f, 1.0f), -1.0f);
}

TEST(DirectionalShadowMathTest, CreatesOverlapOnlyBeforeEachInteriorSplit)
{
    constexpr std::array<float, 5> splits{10.0f, 31.622f, 100.0f, 316.22f, 1000.0f};

    const auto band = Fleur::ShadowMath::GetCascadeOverlap(splits, 1, 0.1f, 0.1f);

    EXPECT_LT(band.start, band.end);
    EXPECT_FLOAT_EQ(band.end, splits[1]);
    EXPECT_GT(band.start, splits[0]);
}

TEST(DirectionalShadowMathTest, FinalCascadeHasNoOverlap)
{
    constexpr std::array<float, 5> splits{10.0f, 31.622f, 100.0f, 316.22f, 1000.0f};
    const auto band = Fleur::ShadowMath::GetCascadeOverlap(splits, 4, 0.1f, 0.1f);

    EXPECT_FLOAT_EQ(band.start, band.end);
}
