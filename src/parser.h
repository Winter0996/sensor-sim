#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace sensor {

struct Detection {
    std::uint32_t id;
    float range_m;
    float azimuth_deg;
    std::uint64_t timestamp_ms;
};

// Wire format (little-endian, no padding): id(4) range(4) azimuth(4) timestamp(8)
constexpr std::size_t kDetectionSize = 20;

std::vector<std::uint8_t> serialize(const Detection& d);

// Returns std::nullopt if the buffer is the wrong size or the values are invalid.
std::optional<Detection> parse(const std::uint8_t* data, std::size_t size);

}  // namespace sensor