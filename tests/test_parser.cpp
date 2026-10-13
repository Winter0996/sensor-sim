#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "parser.h"

using sensor::Detection;

TEST(ParserTest, RoundTripPreservesFields) {
    Detection in{42, 1500.5f, 123.25f, 1700000000123ULL};
    std::vector<std::uint8_t> bytes = sensor::serialize(in);
    ASSERT_EQ(bytes.size(), sensor::kDetectionSize);

    auto out = sensor::parse(bytes.data(), bytes.size());
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out->id, in.id);
    EXPECT_FLOAT_EQ(out->range_m, in.range_m);
    EXPECT_FLOAT_EQ(out->azimuth_deg, in.azimuth_deg);
    EXPECT_EQ(out->timestamp_ms, in.timestamp_ms);
}

TEST(ParserTest, UsesLittleEndianLayout) {
    Detection d{1, 0.0f, 0.0f, 0};
    std::vector<std::uint8_t> bytes = sensor::serialize(d);
    EXPECT_EQ(bytes[0], 1);
    EXPECT_EQ(bytes[1], 0);
}

TEST(ParserTest, RejectsWrongSize) {
    std::vector<std::uint8_t> bytes(sensor::kDetectionSize - 1, 0);
    EXPECT_FALSE(sensor::parse(bytes.data(), bytes.size()).has_value());
}

TEST(ParserTest, RejectsNullPointer) {
    EXPECT_FALSE(sensor::parse(nullptr, sensor::kDetectionSize).has_value());
}

TEST(ParserTest, RejectsNegativeRange) {
    Detection bad{1, -5.0f, 10.0f, 0};
    std::vector<std::uint8_t> bytes = sensor::serialize(bad);
    EXPECT_FALSE(sensor::parse(bytes.data(), bytes.size()).has_value());
}

TEST(ParserTest, RejectsAzimuthOutOfRange) {
    Detection bad{1, 100.0f, 360.0f, 0};
    std::vector<std::uint8_t> bytes = sensor::serialize(bad);
    EXPECT_FALSE(sensor::parse(bytes.data(), bytes.size()).has_value());
}