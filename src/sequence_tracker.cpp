#include "sequence_tracker.h"

namespace sensor {

SequenceTracker::Result SequenceTracker::observe(std::uint32_t id) {
    ++stats_.observed;

    if (!started_) {
        started_ = true;
        expected_ = id + 1;
        return {Kind::First, 0};
    }

    // Signed difference handles 32-bit wraparound.
    const std::int32_t diff = static_cast<std::int32_t>(id - expected_);

    if (diff == 0) {
        ++expected_;
        return {Kind::InOrder, 0};
    }

    if (diff > kResyncThreshold || diff < -kResyncThreshold) {
        ++stats_.restarts;
        missing_.clear();
        expected_ = id + 1;
        return {Kind::Restart, 0};
    }

    if (diff > 0) {
        const auto skipped = static_cast<std::uint32_t>(diff);
        for (std::uint32_t i = 0; i < skipped; ++i) {
            missing_.insert(expected_ + i);
        }
        while (missing_.size() > kMaxTracked) {
            missing_.erase(missing_.begin());
        }
        stats_.lost += skipped;
        expected_ = id + 1;
        return {Kind::Gap, skipped};
    }

    // diff < 0: an older id showed up again.
    auto it = missing_.find(id);
    if (it != missing_.end()) {
        missing_.erase(it);
        if (stats_.lost > 0) {
            --stats_.lost;
        }
        ++stats_.out_of_order;
        return {Kind::Late, 0};
    }

    ++stats_.duplicates;
    return {Kind::Duplicate, 0};
}

}  // namespace sensor