// Compile-time contract tests. The real checks are static_asserts evaluated by
// the compiler; each TEST body just SUCCEED()s so the suite reports a passing
// case and the contract is visible in test output. If the build compiles, the
// contracts below hold.
//
// Contracts pinned here (from CoreLibConcepts.h / Uptr.h / Sptr.h):
//  - Uptr is move-only (FLEUR_NON_COPYABLE): not copyable, but movable.
//  - Sptr is copyable (shared ownership).
//  - FleurAllocator concept = DefaultConstructible + ByteAllocator. Satisfied by
//    DefaultAllocator, not by a bare `int`.
//  - ArrayType / NotArrayType concepts classify array vs non-array types.
#include <type_traits>

#include "CoreLibConcepts.h"
#include "DefaultAllocator.h"
#include "Sptr.h"
#include "Uptr.h"

#include "gtest/gtest.h"

using namespace Fleur;

// --- Uptr move-only contract ---

static_assert(!std::is_copy_constructible_v<Uptr<int>>, "Uptr must be non-copy-constructible (FLEUR_NON_COPYABLE)");
static_assert(!std::is_copy_assignable_v<Uptr<int>>, "Uptr must be non-copy-assignable (FLEUR_NON_COPYABLE)");
static_assert(std::is_move_constructible_v<Uptr<int>>, "Uptr must be move-constructible");
static_assert(std::is_move_assignable_v<Uptr<int>>, "Uptr must be move-assignable");

TEST(Contract_Uptr, MoveOnly)
{
    SUCCEED();  // asserts above are compile-time
}

// --- Sptr copyable contract ---

static_assert(std::is_copy_constructible_v<Sptr<int>>, "Sptr must be copy-constructible (shared ownership)");
static_assert(std::is_copy_assignable_v<Sptr<int>>, "Sptr must be copy-assignable (shared ownership)");
static_assert(std::is_move_constructible_v<Sptr<int>>, "Sptr must be move-constructible");

TEST(Contract_Sptr, Copyable)
{
    SUCCEED();
}

// --- FleurAllocator concept ---

static_assert(FleurAllocator<DefaultAllocator>, "DefaultAllocator must satisfy the FleurAllocator concept");
// `int` has no allocate/deallocate, so it fails the ByteAllocator requirement.
static_assert(!FleurAllocator<int>, "a plain `int` must NOT satisfy FleurAllocator");

// A type that is default-constructible but lacks allocate/deallocate also fails.
namespace
{
struct NotAnAllocator
{
    int x = 0;
};
}  // namespace
static_assert(!FleurAllocator<NotAnAllocator>, "type without allocate/deallocate must NOT satisfy FleurAllocator");

TEST(Contract_Allocator, ConceptSatisfaction)
{
    SUCCEED();
}

// --- ArrayType / NotArrayType concepts ---

static_assert(ArrayType<int[]>, "int[] must be an ArrayType");
static_assert(ArrayType<int[4]>, "int[4] must be an ArrayType");
static_assert(!ArrayType<int>, "int must NOT be an ArrayType");
static_assert(NotArrayType<int>, "int must satisfy NotArrayType");
static_assert(!NotArrayType<int[]>, "int[] must NOT satisfy NotArrayType");

TEST(Contract_ArrayConcepts, Classification)
{
    SUCCEED();
}
