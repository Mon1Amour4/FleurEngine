#include "Fleur/Lux/TangentGeneration.hpp"
#include "Fleur/Math/Functions.hpp"

#include "gtest/gtest.h"

#include <cmath>
#include <limits>

namespace
{
constexpr float kTolerance = 1e-5f;

void ExpectFinite(Fleur::Vec3 value)
{
    EXPECT_TRUE(std::isfinite(value.x));
    EXPECT_TRUE(std::isfinite(value.y));
    EXPECT_TRUE(std::isfinite(value.z));
}

void ExpectUnitLength(Fleur::Vec3 value)
{
    EXPECT_NEAR(Fleur::Math::length(value), 1.0f, kTolerance);
}
} // namespace

TEST(TangentGenerationTest, AccumulatesTriangleWithNormalUvOrientation)
{
    const auto frame = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(1.0f, 0.0f),
        Fleur::Vec2(0.0f, 1.0f));

    ASSERT_TRUE(frame.valid);
    EXPECT_NEAR(frame.tangent.x, 1.0f, kTolerance);
    EXPECT_NEAR(frame.tangent.y, 0.0f, kTolerance);
    EXPECT_NEAR(frame.tangent.z, 0.0f, kTolerance);
    EXPECT_NEAR(frame.bitangent.x, 0.0f, kTolerance);
    EXPECT_NEAR(frame.bitangent.y, 1.0f, kTolerance);
    EXPECT_NEAR(frame.bitangent.z, 0.0f, kTolerance);
    EXPECT_FLOAT_EQ(frame.handedness, 1.0f);

    const auto tangent = Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(0.0f, 0.0f, 1.0f), frame.tangent, frame.bitangent);
    ASSERT_TRUE(tangent.has_value());
    const Fleur::Vec3 direction(tangent->x, tangent->y, tangent->z);
    ExpectFinite(direction);
    ExpectUnitLength(direction);
    EXPECT_NEAR(Fleur::Math::dot(direction, Fleur::Vec3(0.0f, 0.0f, 1.0f)), 0.0f, kTolerance);
    EXPECT_NEAR(direction.x, 1.0f, kTolerance);
    EXPECT_NEAR(direction.y, 0.0f, kTolerance);
    EXPECT_NEAR(direction.z, 0.0f, kTolerance);
    EXPECT_FLOAT_EQ(tangent->w, 1.0f);
}

TEST(TangentGenerationTest, AccumulatesSmallScaleFiniteTriangle)
{
    constexpr float scale = 1.0e-20f;
    const auto frame = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(scale, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, scale, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(scale, 0.0f),
        Fleur::Vec2(0.0f, scale),
        0.0f);

    ASSERT_TRUE(frame.valid);
    ExpectFinite(frame.tangent);
    ExpectFinite(frame.bitangent);
    EXPECT_NEAR(frame.tangent.x, 1.0f, kTolerance);
    EXPECT_NEAR(frame.tangent.y, 0.0f, kTolerance);
    EXPECT_NEAR(frame.tangent.z, 0.0f, kTolerance);
    EXPECT_NEAR(frame.bitangent.x, 0.0f, kTolerance);
    EXPECT_NEAR(frame.bitangent.y, 1.0f, kTolerance);
    EXPECT_NEAR(frame.bitangent.z, 0.0f, kTolerance);
    EXPECT_FLOAT_EQ(frame.handedness, 1.0f);
}

TEST(TangentGenerationTest, PreservesNegativeHandednessForMirroredUvOrientation)
{
    const auto frame = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(0.0f, 1.0f),
        Fleur::Vec2(1.0f, 0.0f));

    ASSERT_TRUE(frame.valid);
    EXPECT_NEAR(frame.tangent.x, 0.0f, kTolerance);
    EXPECT_NEAR(frame.tangent.y, 1.0f, kTolerance);
    EXPECT_NEAR(frame.bitangent.x, 1.0f, kTolerance);
    EXPECT_NEAR(frame.bitangent.y, 0.0f, kTolerance);
    EXPECT_FLOAT_EQ(frame.handedness, -1.0f);

    const auto tangent = Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(0.0f, 0.0f, 1.0f), frame.tangent, frame.bitangent);
    ASSERT_TRUE(tangent.has_value());
    const Fleur::Vec3 direction(tangent->x, tangent->y, tangent->z);
    ExpectFinite(direction);
    ExpectUnitLength(direction);
    EXPECT_NEAR(Fleur::Math::dot(direction, Fleur::Vec3(0.0f, 0.0f, 1.0f)), 0.0f, kTolerance);
    EXPECT_FLOAT_EQ(tangent->w, -1.0f);
}

TEST(TangentGenerationTest, RejectsDegenerateUvTriangleWithoutNonFiniteValues)
{
    const auto frame = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(0.5f, 0.5f),
        Fleur::Vec2(1.0f, 1.0f));

    EXPECT_FALSE(frame.valid);
    ExpectFinite(frame.tangent);
    ExpectFinite(frame.bitangent);
    EXPECT_FLOAT_EQ(frame.handedness, 1.0f);
}

TEST(TangentGenerationTest, RejectsZeroAreaGeometryEvenWithValidUvs)
{
    const auto frame = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(2.0f, 0.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(1.0f, 0.0f),
        Fleur::Vec2(0.0f, 1.0f));

    EXPECT_FALSE(frame.valid);
    ExpectFinite(frame.tangent);
    ExpectFinite(frame.bitangent);
}

TEST(TangentGenerationTest, RejectsNonFiniteAndInvalidInputs)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();

    const auto nonFinitePosition = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(nan, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(1.0f, 0.0f),
        Fleur::Vec2(0.0f, 1.0f));
    EXPECT_FALSE(nonFinitePosition.valid);

    const auto nonFiniteUv = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(infinity, 0.0f),
        Fleur::Vec2(0.0f, 1.0f));
    EXPECT_FALSE(nonFiniteUv.valid);

    const auto negativeEpsilon = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(1.0f, 0.0f),
        Fleur::Vec2(0.0f, 1.0f),
        -1.0f);
    EXPECT_FALSE(negativeEpsilon.valid);

    const auto invalidNormal = Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(nan, 0.0f, 1.0f), Fleur::Vec3(1.0f, 0.0f, 0.0f), Fleur::Vec3(0.0f, 1.0f, 0.0f));
    EXPECT_FALSE(invalidNormal.has_value());

    const auto invalidAccumulation = Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(0.0f, 0.0f, 1.0f), Fleur::Vec3(nan), Fleur::Vec3(infinity));
    ASSERT_TRUE(invalidAccumulation.has_value());
    EXPECT_TRUE(std::isfinite(invalidAccumulation->x));
    EXPECT_TRUE(std::isfinite(invalidAccumulation->y));
    EXPECT_TRUE(std::isfinite(invalidAccumulation->z));
    EXPECT_TRUE(std::isfinite(invalidAccumulation->w));
    EXPECT_FLOAT_EQ(invalidAccumulation->w, 1.0f);
}

TEST(TangentGenerationTest, FinalizesLargeFiniteVectorsWithoutOverflow)
{
    const auto tangent = Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(1.0e20f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0e20f, 0.0f),
        Fleur::Vec3(0.0f, 0.0f, 1.0e20f));

    ASSERT_TRUE(tangent.has_value());
    const Fleur::Vec3 direction(tangent->x, tangent->y, tangent->z);
    ExpectFinite(direction);
    ExpectUnitLength(direction);
    EXPECT_NEAR(direction.x, 0.0f, kTolerance);
    EXPECT_NEAR(direction.y, 1.0f, kTolerance);
    EXPECT_NEAR(direction.z, 0.0f, kTolerance);
    EXPECT_FLOAT_EQ(tangent->w, 1.0f);
}

TEST(TangentGenerationTest, RejectsZeroAndNonFiniteNormals)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    const Fleur::Vec3 tangent(1.0f, 0.0f, 0.0f);
    const Fleur::Vec3 bitangent(0.0f, 1.0f, 0.0f);

    EXPECT_FALSE(Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(0.0f), tangent, bitangent).has_value());
    EXPECT_FALSE(Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(nan, 0.0f, 1.0f), tangent, bitangent).has_value());
    EXPECT_FALSE(Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(infinity, 0.0f, 1.0f), tangent, bitangent).has_value());
}

TEST(TangentGenerationTest, RejectsDeterminantsAtOrBelowEpsilon)
{
    constexpr float epsilon = 1e-4f;

    const auto atBoundary = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(epsilon, 0.0f),
        Fleur::Vec2(0.0f, 1.0f),
        epsilon);
    EXPECT_FALSE(atBoundary.valid);

    const auto belowBoundary = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(epsilon * 0.5f, 0.0f),
        Fleur::Vec2(0.0f, 1.0f),
        epsilon);
    EXPECT_FALSE(belowBoundary.valid);

    const auto aboveBoundary = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(epsilon * 2.0f, 0.0f),
        Fleur::Vec2(0.0f, 1.0f),
        epsilon);
    EXPECT_TRUE(aboveBoundary.valid);
}

TEST(TangentGenerationTest, FinalizesAccumulationAcrossMultipleTriangles)
{
    const auto first = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(0.0f, 0.0f, 0.0f),
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(1.0f, 0.0f),
        Fleur::Vec2(0.0f, 1.0f));
    const auto second = Fleur::Graphics::AccumulateTriangleTangent(
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(2.0f, 0.0f, 0.0f),
        Fleur::Vec3(1.0f, 1.0f, 0.0f),
        Fleur::Vec2(0.0f, 0.0f),
        Fleur::Vec2(2.0f, 0.0f),
        Fleur::Vec2(0.0f, 1.0f));

    ASSERT_TRUE(first.valid);
    ASSERT_TRUE(second.valid);
    const auto tangent = Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(0.0f, 0.0f, 1.0f),
        first.tangent + second.tangent,
        first.bitangent + second.bitangent);

    ASSERT_TRUE(tangent.has_value());
    const Fleur::Vec3 direction(tangent->x, tangent->y, tangent->z);
    ExpectFinite(direction);
    ExpectUnitLength(direction);
    EXPECT_NEAR(direction.x, 1.0f, kTolerance);
    EXPECT_NEAR(direction.y, 0.0f, kTolerance);
    EXPECT_NEAR(direction.z, 0.0f, kTolerance);
    EXPECT_FLOAT_EQ(tangent->w, 1.0f);
}

TEST(TangentGenerationTest, OrthonormalizesAccumulatedTangentAgainstNormal)
{
    const auto tangent = Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(0.0f, 0.0f, 1.0f),
        Fleur::Vec3(2.0f, 1.0f, 3.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f));

    ASSERT_TRUE(tangent.has_value());
    const Fleur::Vec3 direction(tangent->x, tangent->y, tangent->z);
    ExpectFinite(direction);
    ExpectUnitLength(direction);
    EXPECT_NEAR(Fleur::Math::dot(direction, Fleur::Vec3(0.0f, 0.0f, 1.0f)), 0.0f, kTolerance);
    EXPECT_NEAR(direction.x, 2.0f / std::sqrt(5.0f), kTolerance);
    EXPECT_NEAR(direction.y, 1.0f / std::sqrt(5.0f), kTolerance);
    EXPECT_NEAR(direction.z, 0.0f, kTolerance);
    EXPECT_FLOAT_EQ(tangent->w, 1.0f);
}

TEST(TangentGenerationTest, UsesDeterministicOrthonormalFallbackWhenAccumulationIsMissing)
{
    const auto tangent = Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(0.0f, 0.0f, 1.0f), Fleur::Vec3(0.0f), Fleur::Vec3(0.0f));

    ASSERT_TRUE(tangent.has_value());
    const Fleur::Vec3 direction(tangent->x, tangent->y, tangent->z);
    ExpectFinite(direction);
    ExpectUnitLength(direction);
    EXPECT_NEAR(Fleur::Math::dot(direction, Fleur::Vec3(0.0f, 0.0f, 1.0f)), 0.0f, kTolerance);
    EXPECT_NEAR(direction.x, 1.0f, kTolerance);
    EXPECT_NEAR(direction.y, 0.0f, kTolerance);
    EXPECT_NEAR(direction.z, 0.0f, kTolerance);
    EXPECT_FLOAT_EQ(tangent->w, 1.0f);
}

TEST(TangentGenerationTest, UsesPositiveHandednessForFallbackWithOpposingBitangent)
{
    const auto tangent = Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(0.0f, 0.0f, 1.0f), Fleur::Vec3(0.0f), Fleur::Vec3(0.0f, -1.0f, 0.0f));

    ASSERT_TRUE(tangent.has_value());
    EXPECT_FLOAT_EQ(tangent->w, 1.0f);
}

TEST(TangentGenerationTest, PreservesNegativeHandednessForSmallFiniteBitangent)
{
    const auto tangent = Fleur::Graphics::FinalizeTangent(
        Fleur::Vec3(0.0f, 0.0f, 1.0f),
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, -1.0e-12f, 0.0f));

    ASSERT_TRUE(tangent.has_value());
    EXPECT_FLOAT_EQ(tangent->w, -1.0f);
}
