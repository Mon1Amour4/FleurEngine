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