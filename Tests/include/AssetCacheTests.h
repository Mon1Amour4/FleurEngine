#pragma once

#include <atomic>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "Renderer/Material.h"
#include "AssetCache.h"
#include "gtest/gtest.h"

namespace
{

struct TestAsset
{
    TestAsset() = default;
    explicit TestAsset(std::string_view name)
        : m_Name(name)
    {
    }

    std::string_view GetName() const
    {
        return m_Name;
    }

private:
    std::string m_Name;
};

}  // namespace

TEST(AssetCacheTest, RegisterCreatesAssetAndHandle)
{
    Fleur::AssetCache<TestAsset> cache;

    auto record = cache.Register("Textures/brick.png", 42);

    EXPECT_TRUE(record.registered);
    EXPECT_FALSE(record.alreadyExist);
    ASSERT_NE(record.asset.obj, nullptr);
    EXPECT_EQ(record.asset.handle.id, 42u);
    EXPECT_EQ(record.asset.handle.generation, 1u);
    EXPECT_EQ(record.asset.obj->GetName(), "brick.png");
}

TEST(AssetCacheTest, RegisterReturnsExistingRecordForSamePath)
{
    Fleur::AssetCache<TestAsset> cache;

    auto first = cache.Register("Textures/brick.png", 42);
    auto second = cache.Register("Textures/brick.png", 77);

    EXPECT_FALSE(first.alreadyExist);
    EXPECT_TRUE(second.alreadyExist);
    EXPECT_EQ(second.asset.handle.id, first.asset.handle.id);
}

TEST(AssetCacheTest, GetByHandleReturnsNullForInvalidGeneration)
{
    Fleur::AssetCache<TestAsset> cache;
    auto record = cache.Register("Textures/brick.png", 42);

    Fleur::AssetHandle wrongGeneration{record.asset.handle.id, record.asset.handle.generation + 1};
    auto asset = cache.Get(wrongGeneration);

    EXPECT_EQ(asset.obj, nullptr);
    EXPECT_FALSE(asset.handle.IsValid());
}

TEST(AssetCacheTest, ReleaseByHandleInvalidatesAssetLookup)
{
    Fleur::AssetCache<TestAsset> cache;
    auto record = cache.Register("Textures/brick.png", 42);

    cache.Release(record.asset.handle);
    auto asset = cache.Get(record.asset.handle);

    EXPECT_EQ(asset.obj, nullptr);
    EXPECT_FALSE(asset.handle.IsValid());
}

TEST(AssetCacheTest, RegisterAsyncReturnsExistingOperationForSamePath)
{
    Fleur::AssetCache<TestAsset> cache;

    auto first = cache.RegisterAsync("Textures/brick.png", 42);
    auto second = cache.RegisterAsync("Textures/brick.png", 77);

    EXPECT_NE(first, nullptr);
    EXPECT_EQ(first->status.GetStatus(), Fleur::REGISTERED);
    EXPECT_EQ(second.get(), first.get());
}
