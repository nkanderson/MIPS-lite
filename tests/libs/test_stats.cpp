#include <gtest/gtest.h>

#include "mips_lite_defs.h"
#include "stats.h"

using namespace mips_lite;

TEST(StatsTest, InstructionCategoryCounts) {
    Stats stats;
    stats.incrementCategory(InstructionCategory::ARITHMETIC);
    stats.incrementCategory(InstructionCategory::ARITHMETIC);
    stats.incrementCategory(InstructionCategory::LOGICAL);

    EXPECT_EQ(stats.getCategoryCount(InstructionCategory::ARITHMETIC), 2);
    EXPECT_EQ(stats.getCategoryCount(InstructionCategory::LOGICAL), 1);
    EXPECT_EQ(stats.totalInstructions(), 3);
}

TEST(StatsTest, RegisterAndMemoryStorage) {
    Stats stats;
    stats.addRegister(1);
    stats.addRegister(1);  // duplicate
    stats.addRegister(2);

    stats.addMemoryAddress(0x1A2B);
    stats.addMemoryAddress(0x1A2B);  // duplicate
    stats.addMemoryAddress(0x3C4D);

    EXPECT_EQ(stats.getRegisters().size(), 2);
    EXPECT_TRUE(stats.getRegisters().count(1));
    EXPECT_TRUE(stats.getRegisters().count(2));

    EXPECT_EQ(stats.getMemoryAddresses().size(), 2);
    EXPECT_TRUE(stats.getMemoryAddresses().count(0x1A2B));
    EXPECT_TRUE(stats.getMemoryAddresses().count(0x3C4D));
}

TEST(StatsTest, MetricCounters) {
    Stats stats;
    stats.incrementStalls();
    stats.incrementStalls();
    stats.incrementClockCycles();
    stats.incrementDataHazards();
    stats.incrementDataHazards();
    stats.incrementDataHazards();

    EXPECT_EQ(stats.getStalls(), 2);
    EXPECT_EQ(stats.getClockCycles(), 1);
    EXPECT_EQ(stats.getDataHazards(), 3);
}

TEST(StatsTest, AverageStallsPerHazard) {
    Stats stats;

    // No hazards yet — expect 0.0
    EXPECT_FLOAT_EQ(stats.averageStallsPerHazard(), 0.0f);

    stats.incrementDataHazards();
    stats.incrementStalls();
    stats.incrementStalls();  // 2 stalls, 1 hazard

    EXPECT_FLOAT_EQ(stats.averageStallsPerHazard(), 2.0f);

    stats.incrementDataHazards();  // now 2 hazards total
    EXPECT_FLOAT_EQ(stats.averageStallsPerHazard(), 1.0f);
}
