// Edge-case coverage for Fleur::AssetCache<T> / AssetHandle, complementing the
// happy-path + stress cases in AssetCacheTests.cpp. Focus: null/never-registered
// lookups, double-release, wrong-generation release, async-before-completion,
// no-refcount semantics, and handle validity. Bugs found during analysis are
// captured as DISABLED_ with notes.
#include <string>
#include <string_view>

#include "Renderer/Material.h"
#include "AssetCache.h"

#include "gtest/gtest.h"

namespace
{
struct TestAsset
{
    TestAsset() = default;
    explicit TestAsset(std::string_view name) : m_Name(name)
    {
    }

    std::string_view GetName() const
    {
        return m_Name;
    }

private:
    std::string m_Name;
};

using Cache = Fleur::AssetCache<TestAsset>;
}  // namespace

// --- Null / never-registered lookups ---

TEST(AssetCacheEdge, GetByName_NeverRegistered_ReturnsNull)
{
    Cache cache;
    auto a = cache.Get("nope.png");
    EXPECT_EQ(a.obj, nullptr);
    EXPECT_FALSE(a.handle.IsValid());
}

TEST(AssetCacheEdge, GetByHandle_NeverRegistered_ReturnsNull)
{
    Cache cache;
    auto a = cache.Get(Fleur::AssetHandle{777, 1});
    EXPECT_EQ(a.obj, nullptr);
}

TEST(AssetCacheEdge, GetByHandle_DefaultHandle_ReturnsNull)
{
    Cache cache;
    auto a = cache.Get(Fleur::AssetHandle{});
    EXPECT_EQ(a.obj, nullptr);
}

// --- Release robustness ---

TEST(AssetCacheEdge, ReleaseByHandle_WrongGeneration_IsNoOp)
{
    Cache cache;
    auto rec = cache.Register("Textures/brick.png", 42);

    cache.Release(Fleur::AssetHandle{42, rec.asset.handle.generation + 1});  // wrong gen
    EXPECT_NE(cache.Get(Fleur::AssetHandle{42, 1}).obj, nullptr)
        << "wrong-generation release must not evict the live asset";
}

TEST(AssetCacheEdge, DoubleReleaseByName_IsNoOp)
{
    Cache cache;
    cache.Register("Textures/brick.png", 42);
    cache.Release("brick.png");
    cache.Release("brick.png");  // second must be safe
    EXPECT_EQ(cache.Get("brick.png").obj, nullptr);
}

TEST(AssetCacheEdge, DoubleReleaseByHandle_IsNoOp)
{
    Cache cache;
    auto rec = cache.Register("Textures/brick.png", 42);
    cache.Release(rec.asset.handle);
    cache.Release(rec.asset.handle);  // safe
    EXPECT_EQ(cache.Get(rec.asset.handle).obj, nullptr);
}

// --- No refcount: one Release removes regardless of Register count ---

TEST(AssetCacheEdge, RegisterManySamePath_SingleReleaseRemoves)
{
    Cache cache;
    cache.Register("Textures/brick.png", 42);
    cache.Register("Textures/brick.png", 43);
    cache.Register("Textures/brick.png", 44);
    cache.Release("brick.png");
    EXPECT_EQ(cache.Get("brick.png").obj, nullptr) << "no refcount: a single release evicts it";
}

// --- Async ---

TEST(AssetCacheEdge, RegisterAsync_GetBeforeCompletion_ReturnsRegisteredAsset)
{
    Cache cache;
    auto op = cache.RegisterAsync("Textures/brick.png", 42);
    ASSERT_NE(op, nullptr);
    EXPECT_EQ(op->status.GetStatus(), Fleur::REGISTERED);
    EXPECT_NE(cache.Get("brick.png").obj, nullptr);
    EXPECT_NE(cache.Get(op->asset.handle).obj, nullptr);
}

TEST(AssetCacheEdge, ReleaseByName_ActiveAsync_SetsTerminateAndKeepsAsset)
{
    Cache cache;
    auto op = cache.RegisterAsync("Textures/async.png", 151);
    cache.Release("async.png");

    EXPECT_EQ(op->status.GetStatus(), Fleur::LOADING_STATUS_TO_TERMINATE);
    EXPECT_NE(cache.Get(Fleur::AssetHandle{151, 1}).obj, nullptr)
        << "async asset is kept until the async op is removed";
    EXPECT_FALSE(op->status.SetStatus(Fleur::LOADED))
        << "TO_TERMINATE must reject a transition to LOADED";
}

TEST(AssetCacheEdge, RemoveBrokenAsyncAsset_UnknownId_IsNoOp)
{
    Cache cache;
    cache.RemoveBrokenAsyncAsset(123);  // must not crash
    EXPECT_TRUE(cache.Register("Textures/brick.png", 42).registered);
}

TEST(AssetCacheEdge, RemoveFromAsyncOperations_UnknownName_IsNoOp)
{
    Cache cache;
    cache.RemoveFromAsyncOperations("ghost.png");  // must not crash
    SUCCEED();
}

// --- Filename collision + handles ---

TEST(AssetCacheEdge, FilenameCollision_ResolvesToFirstRegistrant)
{
    Cache cache;
    auto a = cache.Register("A/brick.png", 101);
    auto b = cache.Register("B/brick.png", 202);  // dedups by basename
    EXPECT_FALSE(a.alreadyExist);
    EXPECT_TRUE(b.alreadyExist);
    EXPECT_EQ(cache.Get("brick.png").handle.id, 101u);
}

TEST(AssetCacheEdge, Handle_IsValid_Semantics)
{
    EXPECT_FALSE(Fleur::AssetHandle{}.IsValid());        // id 0 -> invalid
    EXPECT_TRUE((Fleur::AssetHandle{5, 0}.IsValid()));   // IsValid checks id only

    Cache cache;
    EXPECT_TRUE(cache.Register("x.png", 7).asset.handle.IsValid());
}

TEST(AssetCacheEdge, GetByHandle_StablePointerAcrossCalls)
{
    Cache cache;
    auto rec = cache.Register("x.png", 7);
    TestAsset* p1 = cache.Get(rec.asset.handle).obj;
    TestAsset* p2 = cache.Get(rec.asset.handle).obj;
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1, p2);
}

// --- Documented current behavior (ties to the generation bug) ---

TEST(AssetCacheEdge, ReRegisterSameId_GenerationStaysOne_StaleResolvesToNew)
{
    // Asserts CURRENT behavior: generation is hardcoded to 1, so re-registering
    // the SAME id after release reuses {id,1}; a stale handle therefore resolves
    // to the new asset (the generation guard is ineffective for a reused id).
    Cache cache;
    auto first = cache.Register("Textures/brick.png", 42);
    Fleur::AssetHandle stale = first.asset.handle;  // {42,1}
    cache.Release("brick.png");
    auto again = cache.Register("Textures/brick.png", 42);

    EXPECT_EQ(again.asset.handle.generation, 1u);
    EXPECT_EQ(stale.generation, again.asset.handle.generation);
    EXPECT_NE(cache.Get(stale).obj, nullptr)
        << "stale handle resolves to the new asset (generation guard ineffective for reused id)";
}

TEST(AssetCacheEdge, EmptyPath_RoundTrips)
{
    Cache cache;
    EXPECT_TRUE(cache.Register("", 1).registered);
    EXPECT_NE(cache.Get("").obj, nullptr);
}

// --- Bug documentation (kept disabled) ---

TEST(AssetCacheEdge, DISABLED_RegisterWithIdZero_InvalidLookingHandle)
{
    // BUG: id==0 doubles as the "invalid handle" sentinel (IsValid() == id!=0) AND
    // a usable cache key. Register(path, 0) creates a live asset whose handle
    // reports IsValid()==false. id 0 should be reserved/rejected.
}

TEST(AssetCacheEdge, DISABLED_AsyncOperationsToRelease_LeakAfterAsyncRelease)
{
    // BUG: Release(name/handle) on an asset with an active async op moves the op
    // into asyncOperationsToRelease, but only RemoveBrokenAsyncAsset erases from
    // that map. The normal RemoveFromAsyncOperations path does not, so the
    // shared_ptr is retained indefinitely. Not observable via public API.
}

TEST(AssetCacheEdge, DISABLED_GenerationBumpsOnReRegister)
{
    // Desired (not current) behavior: re-registering a previously-released id
    // should bump generation so stale handles become invalid. Current behavior is
    // covered by ReRegisterSameId_GenerationStaysOne_StaleResolvesToNew.
}
