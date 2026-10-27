#pragma once

#include <cstddef>
#include <cstdint>
#include <set>

namespace sensor {

// Tracks packet ids to detect loss, reordering, duplicates, and sender restarts.
// Ids are compared with a signed 32-bit difference, so wraparound is handled.
class SequenceTracker {
public:
    enum class Kind {
        First,      // first packet seen
        InOrder,    // exactly the id we expected
        Gap,        // ids were skipped; they are now counted as lost
        Late,       // a previously missing id arrived after a later one
        Duplicate,  // an id we already received (or gave up tracking)
        Restart     // large jump in either direction; state was reset
    };

    struct Result {
        Kind kind;
        std::uint32_t missing;  // packets skipped (only meaningful for Gap)
    };

    struct Stats {
        std::uint64_t observed = 0;      // every packet passed to observe()
        std::uint64_t lost = 0;          // detected missing and not (yet) recovered
        std::uint64_t out_of_order = 0;  // arrived after a later packet
        std::uint64_t duplicates = 0;
        std::uint64_t restarts = 0;
    };

    Result observe(std::uint32_t id);
    const Stats& stats() const { return stats_; }

private:
    // A jump bigger than this in either direction is treated as a sender restart.
    static constexpr std::int32_t kResyncThreshold = 1000;
    // Remember at most this many missing ids so memory stays bounded.
    static constexpr std::size_t kMaxTracked = 1024;

    bool started_ = false;
    std::uint32_t expected_ = 0;
    std::set<std::uint32_t> missing_;
    Stats stats_;
};

}  // namespace sensor