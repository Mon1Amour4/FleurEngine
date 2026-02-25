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

TEST(AssetCacheTest, ComplexSingleThreadLoadReleaseCombinations)
{
    Fleur::AssetCache<TestAsset> cache;

    const std::vector<std::string> paths = {
        "Textures/A/stone.png",
        "Textures/A/metal.png",
        "Textures/B/brick.png",
        "Textures/B/wood.png",
    };
    const std::vector<Fleur::AssetID> ids = {11, 22, 33, 44};
    const std::vector<std::string> names = {"stone.png", "metal.png", "brick.png", "wood.png"};
    uint32_t registerOps = 0;
    uint32_t getOps = 0;
    uint32_t releaseOps = 0;
    uint32_t asyncOps = 0;

    std::vector<Fleur::AssetHandle> handles;
    handles.reserve(paths.size());

    for (size_t i = 0; i < paths.size(); ++i)
    {
        auto rec = cache.Register(paths[i], ids[i]);
        ++registerOps;
        ASSERT_TRUE(rec.registered);
        ASSERT_NE(rec.asset.obj, nullptr);
        EXPECT_EQ(rec.asset.handle.id, ids[i]);
        handles.push_back(rec.asset.handle);
    }

    for (size_t i = 0; i < paths.size(); ++i)
    {
        auto byName = cache.Get(names[i]);
        ++getOps;
        ASSERT_NE(byName.obj, nullptr);
        EXPECT_EQ(byName.handle.id, ids[i]);

        auto byId = cache.Get(ids[i]);
        ++getOps;
        ASSERT_NE(byId.obj, nullptr);
        EXPECT_EQ(byId.handle.id, ids[i]);

        auto byHandle = cache.Get(handles[i]);
        ++getOps;
        ASSERT_NE(byHandle.obj, nullptr);
        EXPECT_EQ(byHandle.handle.id, ids[i]);
    }

    auto existing = cache.Register(paths[0], 999);
    ++registerOps;
    EXPECT_TRUE(existing.alreadyExist);
    EXPECT_EQ(existing.asset.handle.id, ids[0]);

    auto asyncOp = cache.RegisterAsync(paths[1], 888);
    ++asyncOps;
    ASSERT_NE(asyncOp, nullptr);
    EXPECT_EQ(asyncOp->asset.handle.id, ids[1]);

    cache.Release(names[2]);
    ++releaseOps;
    auto releasedByName = cache.Get(names[2]);
    ++getOps;
    EXPECT_EQ(releasedByName.obj, nullptr);

    cache.Release(ids[3]);
    ++releaseOps;
    auto releasedById = cache.Get(ids[3]);
    ++getOps;
    EXPECT_EQ(releasedById.obj, nullptr);

    cache.Release(handles[0]);
    ++releaseOps;
    auto releasedByHandle = cache.Get(handles[0]);
    ++getOps;
    EXPECT_EQ(releasedByHandle.obj, nullptr);

    cache.RemoveFromAsyncOperations("metal.png");
    cache.Release(ids[1]);
    ++releaseOps;
    auto releasedAfterAsync = cache.Get(ids[1]);
    ++getOps;
    EXPECT_EQ(releasedAfterAsync.obj, nullptr);

    std::cout << "[AssetCacheTest::SingleThread] register=" << registerOps << ", async_register=" << asyncOps << ", get=" << getOps
              << ", release=" << releaseOps << std::endl;
}

TEST(AssetCacheTest, ComplexMultiThreadLoadReleaseCombinations)
{
    Fleur::AssetCache<TestAsset> cache;

    constexpr uint32_t threadCount = 8;
    constexpr uint32_t operationsPerThread = 2000;
    constexpr uint32_t assetCount = 12;

    std::vector<std::string> paths;
    std::vector<Fleur::AssetID> ids;
    paths.reserve(assetCount);
    ids.reserve(assetCount);
    for (uint32_t i = 0; i < assetCount; ++i)
    {
        paths.emplace_back("Textures/Stress/asset_" + std::to_string(i) + ".png");
        ids.emplace_back(100 + i);
    }

    std::atomic<bool> failed{false};
    std::atomic<uint64_t> registerOps{0};
    std::atomic<uint64_t> duplicateRegisterOps{0};
    std::atomic<uint64_t> asyncRegisterOps{0};
    std::atomic<uint64_t> getByNameOps{0};
    std::atomic<uint64_t> getByIdOps{0};
    std::atomic<uint64_t> releaseByNameOps{0};
    std::atomic<uint64_t> releaseByIdOps{0};
    std::atomic<uint64_t> releaseByHandleOps{0};
    std::atomic<uint64_t> invariantFailures{0};
    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (uint32_t t = 0; t < threadCount; ++t)
    {
        workers.emplace_back(
            [&cache, &paths, &ids, &failed, &registerOps, &duplicateRegisterOps, &asyncRegisterOps, &getByNameOps, &getByIdOps, &releaseByNameOps,
             &releaseByIdOps, &releaseByHandleOps, &invariantFailures, seed = static_cast<uint32_t>(0xBADC0DE + t)]()
            {
                std::mt19937 rng(seed);
                std::uniform_int_distribution<uint32_t> opDist(0, 7);
                std::uniform_int_distribution<uint32_t> idxDist(0, static_cast<uint32_t>(paths.size() - 1));

                Fleur::AssetHandle lastHandle{};

                for (uint32_t i = 0; i < operationsPerThread; ++i)
                {
                    const uint32_t idx = idxDist(rng);
                    const std::string& path = paths[idx];
                    const Fleur::AssetID id = ids[idx];
                    const uint32_t op = opDist(rng);

                    switch (op)
                    {
                    case 0:
                    {
                        ++registerOps;
                        auto rec = cache.Register(path, id);
                        if (rec.asset.obj)
                            lastHandle = rec.asset.handle;
                        break;
                    }
                    case 1:
                    {
                        ++duplicateRegisterOps;
                        auto rec = cache.Register(path, id + 1000);
                        if (rec.asset.obj && rec.alreadyExist && rec.asset.handle.id != id && rec.asset.handle.id != id + 1000)
                        {
                            failed.store(true);
                            ++invariantFailures;
                        }
                        break;
                    }
                    case 2:
                    {
                        ++asyncRegisterOps;
                        auto opResult = cache.RegisterAsync(path, id);
                        if (opResult->asset.obj)
                            lastHandle = opResult->asset.handle;
                        break;
                    }
                    case 3:
                    {
                        ++getByNameOps;
                        auto asset = cache.Get(path);
                        if (asset.obj)
                        {
                            auto byHandle = cache.Get(asset.handle);
                            if (!byHandle.obj || byHandle.handle.id != asset.handle.id)
                            {
                                failed.store(true);
                                ++invariantFailures;
                            }
                            lastHandle = asset.handle;
                        }
                        break;
                    }
                    case 4:
                    {
                        ++getByIdOps;
                        auto asset = cache.Get(id);
                        if (asset.obj && asset.handle.id != id)
                        {
                            failed.store(true);
                            ++invariantFailures;
                        }
                        if (asset.obj)
                            lastHandle = asset.handle;
                        break;
                    }
                    case 5:
                    {
                        ++releaseByNameOps;
                        cache.Release(path);
                        break;
                    }
                    case 6:
                    {
                        ++releaseByIdOps;
                        cache.Release(id);
                        break;
                    }
                    case 7:
                    {
                        if (lastHandle.IsValid())
                        {
                            ++releaseByHandleOps;
                            cache.Release(lastHandle);
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }
            });
    }

    for (auto& thread : workers)
        thread.join();

    ASSERT_FALSE(failed.load());

    // Deterministic cleanup after stress phase.
    for (uint32_t i = 0; i < assetCount; ++i)
    {
        const std::string fileName = std::string("asset_") + std::to_string(i) + ".png";
        cache.RemoveFromAsyncOperations(fileName);
        cache.Release(paths[i]);
    }

    for (uint32_t i = 0; i < assetCount; ++i)
    {
        auto rec = cache.Register(paths[i], ids[i]);
        ASSERT_NE(rec.asset.obj, nullptr);
        EXPECT_EQ(rec.asset.handle.id, ids[i]);
    }

    std::cout << "[AssetCacheTest::MultiThread] register=" << registerOps.load() << ", dup_register=" << duplicateRegisterOps.load()
              << ", async_register=" << asyncRegisterOps.load() << ", get_by_name=" << getByNameOps.load() << ", get_by_id=" << getByIdOps.load()
              << ", release_by_name=" << releaseByNameOps.load() << ", release_by_id=" << releaseByIdOps.load()
              << ", release_by_handle=" << releaseByHandleOps.load() << ", invariant_failures=" << invariantFailures.load() << std::endl;
}
