#include "Tests.h"

int main(int argC, char** argV)
{
    ::testing::InitGoogleTest(&argC, argV);
    return RUN_ALL_TESTS();
}
