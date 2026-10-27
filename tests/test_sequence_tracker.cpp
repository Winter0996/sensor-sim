#include <gtest/gtest.h>

#include <cstdint>

#include "sequence_tracker.h"

using sensor::SequenceTracker;
using Kind = sensor::SequenceTracker::Kind;

TEST(SequenceTrackerTest, FirstPacketIsAccepted) {
    SequenceTracker t;
    auto r = t.observe(10);
    EXPECT_EQ(r.kind, Kind::First);
    EXPECT_EQ(t.stats().observed, 1u);
    EXPECT_EQ(t.stats().lost, 0u);
}

TEST(SequenceTrackerTest, InOrderPacketsAreNotLoss) {
    SequenceTracker t;
    t.observe(10);
    EXPECT_EQ(t.observe(11).kind, Kind::InOrder);
    EXPECT_EQ(t.observe(12).kind, Kind::InOrder);
    EXPECT_EQ(t.stats().observed, 3u);
    EXPECT_EQ(t.stats().lost, 0u);
}

TEST(SequenceTrackerTest, DetectsGap) {
    SequenceTracker t;
    t.observe(0);
    t.observe(1);
    auto r = t.observe(5);
    EXPECT_EQ(r.kind, Kind::Gap);
    EXPECT_EQ(r.missing, 3u);
    EXPECT_EQ(t.stats().lost, 3u);
}

TEST(SequenceTrackerTest, MultipleGapsAccumulate) {
    SequenceTracker t;
    t.observe(0);
    t.observe(2);  // missing 1
    t.observe(5);  // missing 3, 4
    EXPECT_EQ(t.stats().lost, 3u);
}

TEST(SequenceTrackerTest, LateArrivalRecoversLoss) {
    SequenceTracker t;
    t.observe(0);
    t.observe(1);
    t.observe(3);  // 2 is missing
    EXPECT_EQ(t.stats().lost, 1u);

    auto r = t.observe(2);
    EXPECT_EQ(r.kind, Kind::Late);
    EXPECT_EQ(t.stats().lost, 0u);
    EXPECT_EQ(t.stats().out_of_order, 1u);
}

TEST(SequenceTrackerTest, LateArrivalAfterBiggerGapOnlyRecoversOnePacket) {
    SequenceTracker t;
    t.observe(0);
    t.observe(5);  // 1, 2, 3, 4 missing
    EXPECT_EQ(t.stats().lost, 4u);

    EXPECT_EQ(t.observe(2).kind, Kind::Late);
    EXPECT_EQ(t.stats().lost, 3u);
}

TEST(SequenceTrackerTest, DetectsDuplicate) {
    SequenceTracker t;
    t.observe(0);
    t.observe(1);
    EXPECT_EQ(t.observe(1).kind, Kind::Duplicate);
    EXPECT_EQ(t.stats().duplicates, 1u);
    EXPECT_EQ(t.stats().lost, 0u);
}

TEST(SequenceTrackerTest, HandlesWraparound) {
    SequenceTracker t;
    t.observe(0xFFFFFFFEu);
    EXPECT_EQ(t.observe(0xFFFFFFFFu).kind, Kind::InOrder);
    EXPECT_EQ(t.observe(0u).kind, Kind::InOrder);
    EXPECT_EQ(t.stats().lost, 0u);
    EXPECT_EQ(t.stats().restarts, 0u);
}

TEST(SequenceTrackerTest, LargeForwardJumpIsTreatedAsRestart) {
    SequenceTracker t;
    t.observe(0);
    t.observe(1);
    t.observe(2);
    EXPECT_EQ(t.observe(100000).kind, Kind::Restart);
    EXPECT_EQ(t.stats().restarts, 1u);
    EXPECT_EQ(t.stats().lost, 0u);
    EXPECT_EQ(t.observe(100001).kind, Kind::InOrder);
}

TEST(SequenceTrackerTest, SenderRestartingFromZeroIsTreatedAsRestart) {
    SequenceTracker t;
    t.observe(5000);
    t.observe(5001);
    EXPECT_EQ(t.observe(0).kind, Kind::Restart);
    EXPECT_EQ(t.observe(1).kind, Kind::InOrder);
    EXPECT_EQ(t.stats().restarts, 1u);
}