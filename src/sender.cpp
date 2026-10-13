#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <thread>
#include <vector>

#include "parser.h"

int main(int argc, char** argv) {
    const std::uint16_t port =
        (argc > 1) ? static_cast<std::uint16_t>(std::atoi(argv[1])) : 9000;
    const int count = (argc > 2) ? std::atoi(argv[2]) : 100;
    constexpr int kRateHz = 20;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        std::perror("socket");
        return 1;
    }

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr) != 1) {
        std::fprintf(stderr, "Invalid destination address\n");
        close(fd);
        return 1;
    }

    // Simulated target: closing at 15 m/s while sweeping 2 degrees per second, with noise.
    std::mt19937 rng(12345);
    std::normal_distribution<float> range_noise(0.0f, 5.0f);
    std::normal_distribution<float> azimuth_noise(0.0f, 0.3f);

    const auto period = std::chrono::milliseconds(1000 / kRateHz);
    auto next_send = std::chrono::steady_clock::now();

    std::printf("Sending %d detections to 127.0.0.1:%u at %d Hz\n", count,
                static_cast<unsigned>(port), kRateHz);

    for (int i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kRateHz);

        sensor::Detection d{};
        d.id = static_cast<std::uint32_t>(i);
        d.range_m = std::max(0.0f, 3000.0f - 15.0f * t + range_noise(rng));
        float azimuth = std::fmod(45.0f + 2.0f * t + azimuth_noise(rng), 360.0f);
        if (azimuth < 0.0f) {
            azimuth += 360.0f;
        }
        d.azimuth_deg = azimuth;
        d.timestamp_ms = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());

        std::vector<std::uint8_t> bytes = sensor::serialize(d);
        ssize_t sent = sendto(fd, bytes.data(), bytes.size(), 0,
                              reinterpret_cast<const sockaddr*>(&dest), sizeof dest);
        if (sent != static_cast<ssize_t>(bytes.size())) {
            std::perror("sendto");
        }

        if (i % kRateHz == 0) {
            std::printf("sent id=%u range=%.1f azimuth=%.1f\n", d.id, d.range_m,
                        d.azimuth_deg);
        }

        // Sleep until an absolute time so timing errors do not accumulate.
        next_send += period;
        std::this_thread::sleep_until(next_send);
    }

    close(fd);
    std::printf("Done.\n");
    return 0;
}