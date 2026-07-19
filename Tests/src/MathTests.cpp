#include "Fleur/Math/Math.hpp"

#include "gtest/gtest.h"

#include <type_traits>

TEST(MathTest, PublicVectorAliasesAreAvailable)
{
    static_assert(std::is_same_v<Fleur::Vec2, Fleur::Math::Vec2>);
    static_assert(std::is_same_v<Fleur::Vec3, Fleur::Math::Vec3>);
    static_assert(std::is_same_v<Fleur::Vec4, Fleur::Math::Vec4>);
    static_assert(std::is_same_v<Fleur::Math::Vec2, Fleur::Math::vec2>);
    static_assert(std::is_same_v<Fleur::Math::Vec3, Fleur::Math::vec3>);
    static_assert(std::is_same_v<Fleur::Math::Vec4, Fleur::Math::vec4>);

    const Fleur::Math::Vec3 value(1.0f, 2.0f, 3.0f);

    EXPECT_FLOAT_EQ(value.x, 1.0f);
    EXPECT_FLOAT_EQ(value.y, 2.0f);
    EXPECT_FLOAT_EQ(value.z, 3.0f);
}

TEST(MathTest, PublicMatrixAliasesAreAvailable)
{
    static_assert(std::is_same_v<Fleur::Mat2, Fleur::Math::Mat2>);
    static_assert(std::is_same_v<Fleur::Mat3, Fleur::Math::Mat3>);
    static_assert(std::is_same_v<Fleur::Mat4, Fleur::Math::Mat4>);
    static_assert(std::is_same_v<Fleur::Math::Mat2, Fleur::Math::mat2>);
    static_assert(std::is_same_v<Fleur::Math::Mat3, Fleur::Math::mat3>);
    static_assert(std::is_same_v<Fleur::Math::Mat4, Fleur::Math::mat4>);

    const Fleur::Math::Mat4 identity(1.0f);

    EXPECT_FLOAT_EQ(identity[0][0], 1.0f);
    EXPECT_FLOAT_EQ(identity[1][1], 1.0f);
    EXPECT_FLOAT_EQ(identity[2][2], 1.0f);
    EXPECT_FLOAT_EQ(identity[3][3], 1.0f);
}

TEST(MathTest, AbsoluteValueSupportsScalarsAndVectors)
{
    EXPECT_FLOAT_EQ(Fleur::Math::abs(-3.5f), 3.5f);

    const Fleur::Math::Vec3 value(-1.0f, 2.0f, -3.0f);
    const Fleur::Math::Vec3 absoluteValue = Fleur::Math::abs(value);

    EXPECT_FLOAT_EQ(absoluteValue.x, 1.0f);
    EXPECT_FLOAT_EQ(absoluteValue.y, 2.0f);
    EXPECT_FLOAT_EQ(absoluteValue.z, 3.0f);
}
