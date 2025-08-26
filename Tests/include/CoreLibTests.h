#include "Sptr.h"
#include "Uptr.h"
#include "gtest/gtest.h"

using namespace Fleur;

TEST(CoreLibTest, Uptr_DefaultConstructor)
{
    Uptr<int> Ptr;
    EXPECT_EQ(Ptr.Get(), nullptr);
}

TEST(CoreLibTest, Uptr_ConstructorWithPointer)
{
    int* rawPtr = new int(42);
    Uptr<int> Ptr(rawPtr);
    EXPECT_EQ(Ptr.Get(), rawPtr);
    EXPECT_EQ(*Ptr, 42);
}

TEST(CoreLibTest, Uptr_MoveConstructor)
{
    int* rawPtr = new int(42);
    Uptr<int> Ptr1(rawPtr);
    Uptr<int> Ptr2(std::move(Ptr1));

    EXPECT_EQ(Ptr2.Get(), rawPtr);
    EXPECT_EQ(Ptr1.Get(), nullptr);
    EXPECT_EQ(*Ptr2, 42);
}

TEST(CoreLibTest, Uptr_MoveAssignment)
{
    int* rawPtr1 = new int(42);
    int* rawPtr2 = new int(100);

    Uptr<int> ptr1(rawPtr1);
    Uptr<int> ptr2(rawPtr2);

    ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr2.Get(), rawPtr1);
    EXPECT_EQ(ptr1.Get(), nullptr);
    EXPECT_EQ(*ptr2, 42);
}

TEST(CoreLibTest, Uptr_Reset)
{
    int* rawPtr = new int(42);
    Uptr<int> ptr(rawPtr);

    ptr.Reset();
    EXPECT_EQ(ptr.Get(), nullptr);

    int* newPtr = new int(100);
    ptr.Reset(newPtr);
    EXPECT_EQ(ptr.Get(), newPtr);
    EXPECT_EQ(*ptr, 100);
}

TEST(CoreLibTest, Uptr_Release)
{
    int* rawPtr = new int(42);
    Uptr<int> ptr(rawPtr);

    int* releasedPtr = ptr.Release();
    EXPECT_EQ(releasedPtr, rawPtr);
    EXPECT_EQ(ptr.Get(), nullptr);

    delete releasedPtr;
}

TEST(CoreLibTest, Uptr_Swap)
{
    int* rawPtr1 = new int(42);
    int* rawPtr2 = new int(100);

    Uptr<int> ptr1(rawPtr1);
    Uptr<int> ptr2(rawPtr2);

    ptr1.Swap(ptr2);

    EXPECT_EQ(ptr1.Get(), rawPtr2);
    EXPECT_EQ(ptr2.Get(), rawPtr1);
    EXPECT_EQ(*ptr1, 100);
    EXPECT_EQ(*ptr2, 42);
}

TEST(CoreLibTest, Uptr_BoolOperator)
{
    Uptr<int> ptr;
    EXPECT_FALSE(ptr);

    int* rawPtr = new int(42);
    ptr.Reset(rawPtr);
    EXPECT_TRUE(ptr);
}

TEST(CoreLibTest, UptrArray_ConstructorWithPointer)
{
    int* rawPtr = new int[5]{1, 2, 3, 4, 5};
    Uptr<int[]> ptr(5, rawPtr);

    EXPECT_EQ(ptr.Get(), rawPtr);
    EXPECT_EQ(ptr.Size(), 5);
    EXPECT_EQ(ptr[0], 1);
    EXPECT_EQ(ptr[4], 5);
}

TEST(CoreLibTest, UptrArray_Reset)
{
    int* rawPtr = new int[5]{1, 2, 3, 4, 5};
    Uptr<int[]> ptr(5, rawPtr);

    ptr.Reset();
    EXPECT_EQ(ptr.Get(), nullptr);
    EXPECT_EQ(ptr.Size(), 0);

    int* newPtr = new int[3]{10, 20, 30};
    ptr.Reset(3, newPtr);
    EXPECT_EQ(ptr.Get(), newPtr);
    EXPECT_EQ(ptr.Size(), 3);
    EXPECT_EQ(ptr[0], 10);
    EXPECT_EQ(ptr[2], 30);
}

TEST(CoreLibTest, UptrArray_Release)
{
    Uptr<int[]> ptr(5, new int[5]{1, 2, 3, 4, 5});

    int* releasedPtr = ptr.Release();
    EXPECT_EQ(ptr.Get(), nullptr);
    EXPECT_EQ(ptr.Size(), 0);

    delete[] releasedPtr;
}

TEST(CoreLibTest, UptrArray_Swap)
{
    int* rawPtr1 = new int[3]{1, 2, 3};
    int* rawPtr2 = new int[2]{10, 20};

    Uptr<int[]> ptr1(3, rawPtr1);
    Uptr<int[]> ptr2(2, rawPtr2);

    ptr1.Swap(ptr2);

    EXPECT_EQ(ptr1.Get(), rawPtr2);
    EXPECT_EQ(ptr2.Get(), rawPtr1);
    EXPECT_EQ(ptr1.Size(), 2);
    EXPECT_EQ(ptr2.Size(), 3);
    EXPECT_EQ(ptr1[0], 10);
    EXPECT_EQ(ptr2[2], 3);
}

TEST(CoreLibTest, UptrArray_BoolOperator)
{
    Uptr<int[]> ptr;
    EXPECT_FALSE(ptr);

    int* rawPtr = new int[5]{1, 2, 3, 4, 5};
    ptr.Reset(5, rawPtr);
    EXPECT_TRUE(ptr);
}

TEST(CoreLibTest, Uptr_Comparison_SamePointers)
{
    int* rawPtr = new int(42);
    Uptr<int> ptr1(rawPtr);
    Uptr<int> ptr2(rawPtr);

    EXPECT_EQ(ptr1 <=> ptr2, std::strong_ordering::equal);
    EXPECT_FALSE(ptr1 < ptr2);
    EXPECT_FALSE(ptr1 > ptr2);
    EXPECT_TRUE(ptr1 == ptr2);

    ptr1.Release();
}

TEST(CoreLibTest, Uptr_Comparison_DifferentPointers)
{
    int* rawPtr1 = new int(42);
    int* rawPtr2 = new int(100);

    Uptr<int> ptr1(rawPtr1);
    Uptr<int> ptr2(rawPtr2);

    EXPECT_NE(ptr1 <=> ptr2, std::strong_ordering::equal);
    EXPECT_TRUE(ptr1.Get() < ptr2.Get() || ptr1.Get() > ptr2.Get());
}

TEST(CoreLibTest, UptrArray_PointerArithmetic)
{
    int* rawPtr = new int[5]{1, 2, 3, 4, 5};
    Uptr<int[]> ptr(5, rawPtr);

    EXPECT_EQ(ptr[0], 1);
    EXPECT_EQ(ptr[1], 2);
    EXPECT_EQ(ptr[4], 5);

    EXPECT_EQ(rawPtr + 0, ptr + 0);
    EXPECT_EQ(rawPtr + 1, ptr + 1);
    EXPECT_EQ(rawPtr + 4, ptr + 4);
}

TEST(CoreLibTest, SingleObject_DefaultConstruction)
{
    Uptr<int> ptr = MakeUnique<int>();
    EXPECT_TRUE(ptr);
    EXPECT_EQ(*ptr, 0);
}

TEST(CoreLibTest, SingleObject_CustomConstruction)
{
    Uptr<int> ptr = MakeUnique<int>(42);
    EXPECT_TRUE(ptr);
    EXPECT_EQ(*ptr, 42);
}

struct TestStruct
{
    int a;
    float b;

    TestStruct(int x, float y)
        : a(x)
        , b(y)
    {
    }
};

TEST(CoreLibTest, SingleObject_CustomType)
{
    Uptr<TestStruct> ptr = MakeUnique<TestStruct>(10, 3.14f);
    EXPECT_TRUE(ptr);
    EXPECT_EQ(ptr->a, 10);
    EXPECT_FLOAT_EQ(ptr->b, 3.14f);
}

TEST(CoreLibTest, SingleObject_MemoryDeallocation)
{
    Uptr<int> ptr = MakeUnique<int>(100);
    EXPECT_TRUE(ptr);
    EXPECT_EQ(*ptr, 100);

    ptr.Reset();
    EXPECT_FALSE(ptr);
}

TEST(CoreLibTest, Sptr_DefaultConstructor)
{
    Sptr<int> ptr;
    EXPECT_EQ(ptr.Get(), nullptr);
    EXPECT_EQ(ptr.UseCount(), 0);
}

TEST(CoreLibTest, Sptr_ConstructorWithPointer)
{
    Sptr<int> ptr(new int(42));

    EXPECT_EQ(*ptr, 42);
    EXPECT_EQ(ptr.UseCount(), 1);
}

TEST(CoreLibTest, Sptr_CopyConstructor)
{
    Sptr<int> ptr1(new int(42));
    Sptr<int> ptr2(ptr1);

    EXPECT_EQ(*ptr1, 42);
    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(ptr1.Get(), ptr2.Get());
    EXPECT_EQ(ptr1.UseCount(), 2);
    EXPECT_EQ(ptr2.UseCount(), 2);
}

TEST(CoreLibTest, Sptr_MoveConstructor)
{
    Sptr<int> ptr1(new int(42));
    Sptr<int> ptr2(std::move(ptr1));

    EXPECT_EQ(ptr2.UseCount(), 1);
    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(ptr1.Get(), nullptr);
}

TEST(CoreLibTest, Sptr_CopyAssignment)
{
    Sptr<int> ptr1(new int(42));
    Sptr<int> ptr2;

    ptr2 = ptr1;

    EXPECT_EQ(*ptr1, 42);
    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(ptr1.UseCount(), 2);
    EXPECT_EQ(ptr2.UseCount(), 2);
}

TEST(CoreLibTest, Sptr_MoveAssignment)
{
    Sptr<int> ptr1(new int(42));
    Sptr<int> ptr2;

    ptr2 = std::move(ptr1);

    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(ptr2.UseCount(), 1);
    EXPECT_EQ(ptr1.Get(), nullptr);
}

TEST(CoreLibTest, Sptr_Reset)
{
    Sptr<int> ptr(new int(42));
    EXPECT_EQ(*ptr, 42);

    ptr.Reset(new int(100));
    EXPECT_EQ(*ptr, 100);
    EXPECT_EQ(ptr.UseCount(), 1);
}

TEST(CoreLibTest, Sptr_Swap)
{
    Sptr<int> ptr1(new int(42));
    Sptr<int> ptr2(new int(100));

    ptr1.Swap(ptr2);

    EXPECT_EQ(*ptr1, 100);
    EXPECT_EQ(*ptr2, 42);
}

TEST(CoreLibTest, Sptr_BoolOperator)
{
    Sptr<int> ptr;
    EXPECT_FALSE(ptr);

    ptr.Reset(new int(42));
    EXPECT_TRUE(ptr);
}

TEST(CoreLibTest, SptrArray_ConstructorWithPointer)
{
    Sptr<int[]> ptr(5, new int[5]{1, 2, 3, 4, 5});

    EXPECT_EQ(ptr.GetSize(), 5);
    EXPECT_EQ(ptr[0], 1);
    EXPECT_EQ(ptr[4], 5);
}

TEST(CoreLibTest, SptrArray_Reset)
{
    Sptr<int[]> ptr(5, new int[5]{1, 2, 3, 4, 5});
    ptr.Reset(3, new int[3]{10, 20, 30});

    EXPECT_EQ(ptr.GetSize(), 3);
    EXPECT_EQ(ptr[0], 10);
    EXPECT_EQ(ptr[2], 30);
}

TEST(CoreLibTest, Wptr_Lock)
{
    Sptr<int> ptr(new int(42));
    Wptr<int> weak(ptr);

    auto locked = weak.Lock();
    EXPECT_EQ(locked.UseCount(), 2);
    EXPECT_EQ(*locked, 42);
}

TEST(CoreLibTest, Wptr_CopyConstructor)
{
    Sptr<int> ptr(new int(42));
    Wptr<int> weak1(ptr);
    Wptr<int> weak2(weak1);

    EXPECT_EQ(weak1.Expired(), false);
    EXPECT_EQ(weak2.Expired(), false);
    EXPECT_EQ(weak1.Lock().UseCount(), 2);
    EXPECT_EQ(weak2.Lock().UseCount(), 2);
}

TEST(CoreLibTest, Wptr_MoveConstructor)
{
    Sptr<int> ptr(new int(42));
    Wptr<int> weak1(ptr);
    Wptr<int> weak2(std::move(weak1));

    EXPECT_FALSE(weak2.Expired());
    EXPECT_EQ(weak1.Lock().Get(), nullptr);
    EXPECT_EQ(*weak2.Lock(), 42);
}

TEST(CoreLibTest, Wptr_Assignment)
{
    Sptr<int> ptr(new int(42));
    Wptr<int> weak1(ptr);

    Wptr<int> weak2 = weak1;

    EXPECT_EQ(weak2.Expired(), false);
    EXPECT_EQ(*weak2.Lock(), 42);
    EXPECT_EQ(weak2.Lock().UseCount(), 2);
}

TEST(CoreLibTest, Wptr_Expired)
{
    Sptr<int> ptr(new int(42));
    Wptr<int> weak(ptr);

    ptr.Reset();

    EXPECT_TRUE(weak.Expired());
    EXPECT_EQ(weak.Lock().Get(), nullptr);
}

TEST(CoreLibTest, SptrAndWptr_Lifecycle)
{
    Sptr<int> shared(new int(42));
    Wptr<int> weak(shared);

    EXPECT_EQ(shared.UseCount(), 1);
    EXPECT_FALSE(weak.Expired());

    {
        auto locked = weak.Lock();
        EXPECT_EQ(locked.UseCount(), 2);
        EXPECT_EQ(*locked, 42);
    }

    shared.Reset();
    EXPECT_TRUE(weak.Expired());
}

static uint32_t Counter = 0;

struct MemLeaker
{
public:
    MemLeaker()
    {
        ++Counter;
    }

    ~MemLeaker()
    {
        --Counter;
    }
};

TEST(CoreLibTest, Uptr_Single_LeakTest)
{
    auto ptr = MakeUnique<MemLeaker>();

    ptr.Reset();

    EXPECT_EQ(Counter, 0);
}

TEST(CoreLibTest, Uptr_Single_LeadTestMove)
{
    auto ptr = MakeUnique<MemLeaker>();

    ptr.Reset();

    EXPECT_EQ(Counter, 0);
}

TEST(CoreLibTest, Uptr_Array_LeakTest)
{
    auto ptr = MakeUnique<MemLeaker[]>(5);

    ptr.Reset();

    EXPECT_EQ(Counter, 0);
}

TEST(CoreLibTest, Sptr_Single_LeakTest)
{
    auto ptr = MakeShared<MemLeaker>();

    ptr.Reset();

    EXPECT_EQ(Counter, 0);
}

TEST(CoreLibTest, Sptr_Array_LeakTest)
{
    auto ptr = MakeShared<MemLeaker[]>(5);

    ptr.Reset();

    EXPECT_EQ(Counter, 0);

    std::shared_ptr<int> uptr(new int(3));

    std::weak_ptr<int> wptr(uptr);
}
