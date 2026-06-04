// Randomized soak / churn tests for MM::MemoryManager, adapting the core idea of
// Wolfram Gloger's dlmalloc t-test1.c and mimalloc's test-stress.c to Fleur's
// API (byte-sized allocate<uint8_t>/deallocate). A pool of slots is churned with
// random alloc/free; on every step the harness asserts the metamorphic
// invariants that catch the bugs API-point tests miss:
//   * data integrity  — each allocation keeps its fill pattern until freed
//   * alignment        — every pointer is 8-byte aligned
//   * no overlap       — no two live allocations share a byte (aliasing / a
//                        phantom free slot would trip this)
// Sizes are chosen to span the SLUB (<=2032) and TLSF (>2032) routes. RNG seeds
// are fixed so failures are reproducible.
#include <cstdint>
#include <random>
#include <vector>

#include "MemoryManager.h"

#include "gtest/gtest.h"

namespace
{
struct Slot
{
    uint8_t* p = nullptr;
    uint32_t size = 0;
    uint8_t seed = 0;
};

void Fill(uint8_t* p, uint32_t n, uint8_t seed)
{
    for (uint32_t i = 0; i < n; ++i)
        p[i] = static_cast<uint8_t>(seed * 31u + i);
}

bool Verify(const uint8_t* p, uint32_t n, uint8_t seed)
{
    for (uint32_t i = 0; i < n; ++i)
        if (p[i] != static_cast<uint8_t>(seed * 31u + i))
            return false;
    return true;
}

bool Overlaps(uintptr_t a0, uintptr_t a1, uintptr_t b0, uintptr_t b1)
{
    return a0 < b1 && b0 < a1;
}
}  // namespace

class AllocatorSoakTest : public ::testing::Test
{
protected:
    static constexpr size_t kCapacity = 128ull * 1024 * 1024;

    MM::MemoryManager* mm = nullptr;

    void SetUp() override
    {
        mm = MM::MemoryManager::ManagerFabric(kCapacity);
        ASSERT_NE(mm, nullptr);
    }

    // One churn run. Each iteration either allocates into a free slot (filling and
    // checking invariants) or verifies + frees an occupied one. checkAlign is off
    // for TLSF-range runs: TLSF currently returns pointers that are not 8-byte
    // aligned once blocks have been split (see DISABLED_TlsfChurn_AlignmentBug).
    void RunChurn(uint32_t minSize, uint32_t maxSize, int slotCount, int ops, uint32_t rngSeed, bool checkAlign)
    {
        std::mt19937 rng(rngSeed);
        std::uniform_int_distribution<uint32_t> sizeDist(minSize, maxSize);
        std::uniform_int_distribution<int> slotDist(0, slotCount - 1);
        std::uniform_int_distribution<int> seedDist(0, 255);

        std::vector<Slot> live(slotCount);

        for (int op = 0; op < ops; ++op)
        {
            int idx = slotDist(rng);
            Slot& s = live[idx];

            if (s.p == nullptr)
            {
                uint32_t size = sizeDist(rng);
                uint8_t* p = mm->allocate<uint8_t>(size);
                ASSERT_NE(p, nullptr) << "alloc failed at op " << op << " size " << size;
                if (checkAlign)
                    ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 8u, 0u)
                        << "misaligned at op " << op << " size " << size;

                uintptr_t a0 = reinterpret_cast<uintptr_t>(p);
                uintptr_t a1 = a0 + size;
                for (int j = 0; j < slotCount; ++j)
                {
                    if (j == idx || live[j].p == nullptr)
                        continue;
                    uintptr_t b0 = reinterpret_cast<uintptr_t>(live[j].p);
                    uintptr_t b1 = b0 + live[j].size;
                    ASSERT_FALSE(Overlaps(a0, a1, b0, b1))
                        << "live allocations overlap at op " << op;
                }

                uint8_t seed = static_cast<uint8_t>(seedDist(rng));
                Fill(p, size, seed);
                s = {p, size, seed};
            }
            else
            {
                ASSERT_TRUE(Verify(s.p, s.size, s.seed))
                    << "data corrupted at op " << op << " size " << s.size;
                mm->deallocate<uint8_t>(s.p, s.size);
                s = {};
            }
        }

        for (Slot& s : live)
        {
            if (s.p)
            {
                EXPECT_TRUE(Verify(s.p, s.size, s.seed)) << "data corrupted on drain";
                mm->deallocate<uint8_t>(s.p, s.size);
            }
        }
    }
};

TEST_F(AllocatorSoakTest, AlignmentAcrossSizes)
{
    const uint32_t sizes[] = {1, 2, 4, 7, 8, 15, 16, 17, 31, 64, 100, 255, 256, 1024, 2032, 2033, 4096, 65536, 200000};
    for (uint32_t sz : sizes)
    {
        uint8_t* p = mm->allocate<uint8_t>(sz);
        ASSERT_NE(p, nullptr) << "size " << sz;
        EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % 8u, 0u) << "size " << sz;
        Fill(p, sz, 7);
        EXPECT_TRUE(Verify(p, sz, 7)) << "size " << sz;
        mm->deallocate<uint8_t>(p, sz);
    }
}

TEST_F(AllocatorSoakTest, SlubChurn_IntegrityAlignmentNoOverlap)
{
    RunChurn(/*min*/ 1, /*max*/ 2032, /*slots*/ 256, /*ops*/ 20000, 0xC0FFEEu, /*checkAlign*/ true);
}

// DISABLED: exposes serious TLSF bugs under fragmentation churn. The SAME random
// alloc/free workload that the SLUB range survives cleanly (SlubChurn above) makes
// the TLSF path FAIL in two ways:
//   1. Misaligned pointers — once a free block is split, the remainder's payload
//      starts right after the used_block_header (2-byte flag + u32 size +
//      prev_phys_block ptr), which is not 8-byte aligned (observed %8==3). UB for
//      any over-1-aligned object. (checkAlign=true trips ~op 31 at this seed.)
//   2. Access violation (SEH 0xC0000005) — with alignment checks off, the run
//      crashes a bit later, i.e. TLSF corrupts metadata / dereferences a bad
//      block header during split/coalesce under churn.
// Re-enable (and flip checkAlign on) once TLSF aligns payloads to 8 and the
// split/coalesce path is fixed.
TEST_F(AllocatorSoakTest, DISABLED_TlsfChurn_MisalignAndCrash)
{
    RunChurn(/*min*/ 1, /*max*/ 200000, /*slots*/ 128, /*ops*/ 8000, 0xBEEF1234u, /*checkAlign*/ true);
}

// Localizes the TLSF corruption: churns the TLSF range and runs the integrity
// checker (MemoryManager::CheckTlsf -> TLSFAllocator::Check) after EVERY op, so
// the first inconsistency is reported with its op index and reason instead of a
// late access violation. DISABLED (fails by design); run with
//   FleurTests --gtest_also_run_disabled_tests --gtest_filter=*LocalizeWithCheck*
TEST_F(AllocatorSoakTest, DISABLED_TlsfChurn_LocalizeWithCheck)
{
    std::mt19937 rng(0xBEEF1234u);
    std::uniform_int_distribution<uint32_t> sizeDist(2100, 200000);  // TLSF route only
    std::uniform_int_distribution<int> slotDist(0, 127);

    std::vector<Slot> live(128);
    const char* err = nullptr;

    ASSERT_TRUE(mm->CheckTlsf(&err)) << "dirty at start: " << (err ? err : "");

    for (int op = 0; op < 8000; ++op)
    {
        int idx = slotDist(rng);
        Slot& s = live[idx];

        if (s.p == nullptr)
        {
            uint32_t size = sizeDist(rng);
            uint8_t* p = mm->allocate<uint8_t>(size);
            ASSERT_NE(p, nullptr) << "alloc failed at op " << op;
            s = {p, size, 0};
        }
        else
        {
            mm->deallocate<uint8_t>(s.p, s.size);
            s = {};
        }

        ASSERT_TRUE(mm->CheckTlsf(&err))
            << "TLSF integrity broke at op " << op << ": " << (err ? err : "");
    }
}

// Experiment: same TLSF churn but with an 8-byte, 8-aligned element type, so
// sizeof(T)*count is always a multiple of 8 and NO misalignment can occur. If
// this stays clean, the crash needs the byte-type misalignment; if it still
// breaks, the corruption is independent of alignment.
TEST_F(AllocatorSoakTest, DISABLED_TlsfChurn_AlignedType_Experiment)
{
    struct alignas(8) Aligned8
    {
        uint64_t v;
    };

    std::mt19937 rng(0xBEEF1234u);
    std::uniform_int_distribution<uint32_t> cntDist(260, 25000);  // 2080..200000 bytes -> TLSF
    std::uniform_int_distribution<int> slotDist(0, 127);

    struct LiveA
    {
        Aligned8* p = nullptr;
        uint32_t n = 0;
        uint64_t mark = 0;
    };
    std::vector<LiveA> live(128);
    const char* err = nullptr;

    for (int op = 0; op < 8000; ++op)
    {
        int idx = slotDist(rng);
        LiveA& s = live[idx];

        if (s.p == nullptr)
        {
            uint32_t n = cntDist(rng);
            s.p = mm->allocate<Aligned8>(n);
            ASSERT_NE(s.p, nullptr) << "op " << op;
            ASSERT_EQ(reinterpret_cast<uintptr_t>(s.p) % 8u, 0u) << "misaligned at op " << op;
            s.n = n;
            s.mark = static_cast<uint64_t>(op) * 2654435761u + 1;
            s.p[0].v = s.mark;
            s.p[n - 1].v = s.mark;
        }
        else
        {
            ASSERT_EQ(s.p[0].v, s.mark) << "corrupt head at op " << op;
            ASSERT_EQ(s.p[s.n - 1].v, s.mark) << "corrupt tail at op " << op;
            mm->deallocate<Aligned8>(s.p, s.n);
            s = {};
        }

        ASSERT_TRUE(mm->CheckTlsf(&err)) << "TLSF broke at op " << op << ": " << (err ? err : "");
    }
}

// Big evidence base for the TLSF fixes (A/B/C1/C2). Churns byte buffers (worst
// alignment) over a matrix of seeds x size-profiles x intensities, asserting on
// EVERY op: 8-alignment, data integrity, no-overlap, and (per cadence) the full
// Level1+Level2 integrity checker. OOM is a defined outcome (skip, not a failure)
// so capacity never produces a false negative — only real corruption fails.
static void RunChurnChecked(MM::MemoryManager* m, uint32_t lo, uint32_t hi, int slots,
                            int ops, uint32_t seed, int checkEvery, int verifyAllEvery = 0)
{
    std::mt19937 rng(seed);
    std::uniform_int_distribution<uint32_t> sizeDist(lo, hi);
    std::uniform_int_distribution<int> slotDist(0, slots - 1);
    std::uniform_int_distribution<int> seedDist(0, 255);

    std::vector<Slot> live(slots);
    const char* err = nullptr;

    for (int op = 0; op < ops; ++op)
    {
        // Deep cross-corruption check: verify EVERY live allocation's bytes, not
        // just the one being freed. Catches a write into a neighbor immediately.
        if (verifyAllEvery > 0 && (op % verifyAllEvery) == 0)
            for (const Slot& ls : live)
                if (ls.p)
                    ASSERT_TRUE(Verify(ls.p, ls.size, ls.seed)) << "cross-corruption op " << op << " seed " << seed;

        int idx = slotDist(rng);
        Slot& s = live[idx];

        if (s.p == nullptr)
        {
            uint32_t size = sizeDist(rng);
            uint8_t* p = m->allocate<uint8_t>(size);
            if (p == nullptr)
                continue;  // capacity reached — defined behavior, not corruption

            ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 8u, 0u)
                << "misaligned op " << op << " size " << size << " seed " << seed;

            uintptr_t a0 = reinterpret_cast<uintptr_t>(p);
            uintptr_t a1 = a0 + size;
            for (int j = 0; j < slots; ++j)
            {
                if (j == idx || live[j].p == nullptr)
                    continue;
                uintptr_t b0 = reinterpret_cast<uintptr_t>(live[j].p);
                uintptr_t b1 = b0 + live[j].size;
                ASSERT_FALSE(Overlaps(a0, a1, b0, b1)) << "overlap op " << op << " seed " << seed;
            }

            uint8_t sd = static_cast<uint8_t>(seedDist(rng));
            Fill(p, size, sd);
            s = {p, size, sd};
        }
        else
        {
            ASSERT_TRUE(Verify(s.p, s.size, s.seed)) << "corrupt op " << op << " seed " << seed;
            m->deallocate<uint8_t>(s.p, s.size);
            s = {};
        }

        if (checkEvery > 0 && (op % checkEvery) == 0)
            ASSERT_TRUE(m->CheckTlsf(&err)) << "TLSF op " << op << " seed " << seed << ": " << (err ? err : "");
    }

    for (Slot& s : live)
    {
        if (s.p)
        {
            ASSERT_TRUE(Verify(s.p, s.size, s.seed)) << "drain corrupt seed " << seed;
            m->deallocate<uint8_t>(s.p, s.size);
        }
    }
    ASSERT_TRUE(m->CheckTlsf(&err)) << "post-drain seed " << seed << ": " << (err ? err : "");
}

// Fill the arena with TLSF blocks until OOM, writing to each. If allocate ever
// returns a pointer past the real VirtualAlloc region (PageAllocator capacity
// over-extension), the write here faults. Clean OOM = correct.
TEST_F(AllocatorSoakTest, DISABLED_ExhaustProbe)
{
    MM::MemoryManager* m = MM::MemoryManager::ManagerFabric(32ull * 1024 * 1024);  // 32 MiB
    ASSERT_NE(m, nullptr);

    int n = 0;
    std::vector<uint8_t*> keep;
    for (;;)
    {
        uint8_t* p = m->allocate<uint8_t>(1000000);  // 1 MB TLSF blocks, NOT freed
        if (p == nullptr)
            break;  // graceful OOM — correct
        p[0] = 1;
        p[999999] = 2;
        keep.push_back(p);
        ++n;
    }
    std::cout << "[EXHAUST] TLSF: " << n << " blocks held, pages exhausted" << std::endl;

    // PageAllocator is now empty. Hammer SLUB until its pre-allocated chunks fill
    // and it must request a new page -> exercises SLUB's page-OOM handling.
    int sn = 0;
    for (int i = 0; i < 500000; ++i)
    {
        uint8_t* s = m->allocate<uint8_t>(64);  // SLUB
        if (s == nullptr)
        {
            std::cout << "[EXHAUST] SLUB OOM cleanly after " << sn << " slots" << std::endl;
            break;
        }
        s[0] = static_cast<uint8_t>(i);
        ++sn;
    }
    std::cout << "[EXHAUST] SLUB done, " << sn << " slots" << std::endl;
}

TEST_F(AllocatorSoakTest, DISABLED_TlsfLargeProbe)
{
    MM::MemoryManager* m = MM::MemoryManager::ManagerFabric(1024ull * 1024 * 1024);
    ASSERT_NE(m, nullptr);
    const char* err = nullptr;

    const uint32_t sizes[] = {262144u, 500000u, 1000000u, 2000000u, 3000000u, 4000000u, 4194303u};
    for (uint32_t sz : sizes)
    {
        uint8_t* p = m->allocate<uint8_t>(sz);
        std::cout << "[PROBE] size " << sz << " -> " << static_cast<void*>(p) << std::endl;
        if (p)
        {
            p[0] = 1;
            p[sz - 1] = 2;
            ASSERT_TRUE(m->CheckTlsf(&err)) << "after alloc " << sz << ": " << (err ? err : "");
            m->deallocate<uint8_t>(p, sz);
            ASSERT_TRUE(m->CheckTlsf(&err)) << "after free " << sz << ": " << (err ? err : "");
        }
    }
    std::cout << "[PROBE] singles ok; starting mixed-full 768-slot accumulation" << std::endl;

    for (int r = 0; r < 20; ++r)
    {
        RunChurnChecked(m, 1, 4194303, /*slots*/ 768, /*ops*/ 5000, 1000u + r, /*checkEvery*/ 1);
        std::cout << "[PROBE] mixed-full 768 run " << r << " ok" << std::endl;
    }
    std::cout << "[PROBE] churn ok" << std::endl;
}

TEST_F(AllocatorSoakTest, DISABLED_TlsfStressMatrix)
{
    MM::MemoryManager* m = MM::MemoryManager::ManagerFabric(1024ull * 1024 * 1024);  // 1 GiB
    ASSERT_NE(m, nullptr);

    struct Profile
    {
        const char* name;
        uint32_t lo, hi;
    };
    // Covers: SLUB, the router boundary (2032/2033), every TLSF magnitude up to
    // just under MEDIUN_SIZE (4 MiB), and the full-range mix.
    const Profile profs[] = {
        {"slub", 1, 2032},
        {"tlsf-edge", 2033, 2100},
        {"tlsf-small", 2033, 8192},
        {"tlsf-mid", 8193, 65536},
        {"tlsf-large", 65537, 262143},
        {"tlsf-huge", 262144, 2000000},
        {"tlsf-near-max", 3500000, 4194303},
        {"mixed-full", 1, 4194303},
    };
    const int slotCounts[] = {64, 256, 768};  // three fragmentation pressures

    uint64_t totalOps = 0;
    int runs = 0;

    // Tier 1 — breadth: 80 seeds x 8 profiles x 3 slot-counts, full Level1+Level2
    // integrity check on EVERY op (+ alignment + no-overlap + on-free integrity).
    for (uint32_t seed = 1; seed <= 80; ++seed)
    {
        for (const Profile& p : profs)
        {
            for (int slots : slotCounts)
            {
                std::cout << "[CFG] seed " << seed << " profile " << p.name << " slots " << slots << std::endl;
                std::cout.flush();
                RunChurnChecked(m, p.lo, p.hi, slots, /*ops*/ 2500, seed * 2654435761u + p.lo + slots, /*checkEvery*/ 1);
                totalOps += 2500;
                ++runs;
            }
        }
        if (seed % 10 == 0)
            std::cout << "[STRESS] tier1 seed " << seed << "/80 ok, ops=" << totalOps << std::endl;
    }

    // Tier 2 — depth: long runs, big pools, full-range, sparser structural check
    // but with deep verify-ALL-live cross-corruption scans.
    for (uint32_t seed = 1; seed <= 16; ++seed)
    {
        RunChurnChecked(m, 1, 4194303, /*slots*/ 768, /*ops*/ 300000, seed * 40503u + 7,
                        /*checkEvery*/ 200, /*verifyAllEvery*/ 2000);
        totalOps += 300000;
        ++runs;
        std::cout << "[STRESS] tier2 seed " << seed << "/16 ok, ops=" << totalOps << std::endl;
    }

    std::cout << "[STRESS] DONE runs=" << runs << " totalOps=" << totalOps << " all clean" << std::endl;
}

// Long-running soak; opt-in via --gtest_also_run_disabled_tests.
TEST_F(AllocatorSoakTest, DISABLED_HeavySoak)
{
    RunChurn(/*min*/ 1, /*max*/ 262144, /*slots*/ 512, /*ops*/ 200000, 0xABCDEF01u, /*checkAlign*/ false);
}
