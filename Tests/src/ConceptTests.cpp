#pragma once

#include "../Engine/Fleur/Concepts.hpp"
#include "gtest/gtest.h"

struct DefaultConstractible
{
    DefaultConstractible() = default;
};
struct NotDefaultConstractible
{
    NotDefaultConstractible(bool) {};
    NotDefaultConstractible() = delete;
};

TEST(IsDefaultConstreactible, PureType)
{
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<DefaultConstractible>, true);
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<NotDefaultConstractible>, false);
}
TEST(IsDefaultConstreactible, ReferenceType)
{
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<DefaultConstractible&>, false);
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<NotDefaultConstractible&>, false);
}
TEST(IsDefaultConstreactible, PointerType)
{
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<DefaultConstractible*>, true);
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<NotDefaultConstractible*>, true);
}
TEST(IsDefaultConstreactible, ConstType)
{
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<const DefaultConstractible>, true);
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<const NotDefaultConstractible>, false);
}
TEST(IsDefaultConstreactible, CVType)
{
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<const volatile DefaultConstractible>, true);
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<const volatile NotDefaultConstractible>, false);
}

struct AbstractType
{
    virtual ~AbstractType() = default;
    virtual void f() = 0;
};

TEST(IsDefaultConstreactible, FundamentalTypes)
{
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<int>, true);
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<void>, false);
}

TEST(IsDefaultConstreactible, ArrayTypes)
{
    // Bounded array of default-constructible elements is constructible; an
    // unbounded array type is not.
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<int[4]>, true);
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<int[]>, false);
}

TEST(IsDefaultConstreactible, AbstractTypeIsNot)
{
    EXPECT_EQ(Fleur::Concepts::IsDefaultConstructible<AbstractType>, false);
}