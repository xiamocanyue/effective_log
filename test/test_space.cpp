#include "../space.h"

#include <gtest/gtest.h>


TEST(SpaceFunctionTest, HandleAdd) {
    bytes b(10);
    gigabytes gb(10);
    kilobytes kb(10);
    EXPECT_EQ(10, b.count());
    EXPECT_EQ(10485760 , space_cast<kilobytes>(gb).count());
    EXPECT_EQ(10485771 , (gb + kb).count());
}