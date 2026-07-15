#include "Fleur/Math/Math.hpp"

#include "gtest/gtest.h"

#include <type_traits>

TEST(MathTest, PublicVectorAliasesAreAvailable)
{
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
    static_assert(std::is_same_v<Fleur::Math::Mat2, Fleur::Math::mat2>);
    static_assert(std::is_same_v<Fleur::Math::Mat3, Fleur::Math::mat3>);
    static_assert(std::is_same_v<Fleur::Math::Mat4, Fleur::Math::mat4>);

    const Fleur::Math::Mat4 identity(1.0f);

    EXPECT_FLOAT_EQ(identity[0][0], 1.0f);
    EXPECT_FLOAT_EQ(identity[1][1], 1.0f);
    EXPECT_FLOAT_EQ(identity[2][2], 1.0f);
    EXPECT_FLOAT_EQ(identity[3][3], 1.0f);
}
