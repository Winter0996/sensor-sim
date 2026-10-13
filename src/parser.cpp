#include "parser.h"

#include <cmath>
#include <cstring>

namespace sensor {
namespace {

void put_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
    }
}

void put_u64(std::vector<std::uint8_t>& out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
    }
}

std::uint32_t get_u32(const std::uint8_t* p) {
    std::uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        v |= static_cast<std::uint32_t>(p[i]) << (8 * i);
    }
    return v;
}

std::uint64_t get_u64(const std::uint8_t* p) {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= static_cast<std::uint64_t>(p[i]) << (8 * i);
    }
    return v;
}

std::uint32_t float_bits(float f) {
    std::uint32_t b;
    std::memcpy(&b, &f, sizeof b);
    return b;
}

float bits_float(std::uint32_t b) {
    float f;
    std::memcpy(&f, &b, sizeof f);
    return f;
}

}  // namespace

std::vector<std::uint8_t> serialize(const Detection& d) {
    std::vector<std::uint8_t> out;
    out.reserve(kDetectionSize);
    put_u32(out, d.id);
    put_u32(out, float_bits(d.range_m));
    put_u32(out, float_bits(d.azimuth_deg));
    put_u64(out, d.timestamp_ms);
    return out;
}

std::optional<Detection> parse(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size != kDetectionSize) {
        return std::nullopt;
    }

    Detection d;
    d.id = get_u32(data);
    d.range_m = bits_float(get_u32(data + 4));
    d.azimuth_deg = bits_float(get_u32(data + 8));
    d.timestamp_ms = get_u64(data + 12);

    if (!std::isfinite(d.range_m) || d.range_m < 0.0f) {
        return std::nullopt;
    }
    if (!std::isfinite(d.azimuth_deg) || d.azimuth_deg < 0.0f || d.azimuth_deg >= 360.0f) {
        return std::nullopt;
    }
    return d;
}

}  // namespace sensor