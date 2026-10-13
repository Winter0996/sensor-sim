#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include "parser.h"

namespace {

volatile std::sig_atomic_t g_running = 1;

void handle_signal(int) { g_running = 0; }

}  // namespace

int main(int argc, char** argv) {
    const std::uint16_t port =
        (argc > 1) ? static_cast<std::uint16_t>(std::atoi(argv[1])) : 9000;
    const char* csv_path = (argc > 2) ? argv[2] : nullptr;

    std::signal(SIGINT, handle_signal);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        std::perror("socket");
        return 1;
    }

    // Wake up every 500 ms so the loop can notice Ctrl+C.
    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0) {
        std::perror("setsockopt");
        close(fd);
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) {
        std::perror("bind");
        close(fd);
        return 1;
    }

    std::ofstream csv;
    if (csv_path != nullptr) {
        csv.open(csv_path);
        if (!csv) {
            std::fprintf(stderr, "Could not open %s for writing\n", csv_path);
            close(fd);
            return 1;
        }
        csv << "id,range_m,azimuth_deg,timestamp_ms\n";
    }

    std::vector<std::uint8_t> buffer(2048);
    unsigned long received = 0;
    unsigned long rejected = 0;

    std::printf("Listening on UDP port %u (Ctrl+C to stop)\n",
                static_cast<unsigned>(port));

    while (g_running) {
        ssize_t n = recvfrom(fd, buffer.data(), buffer.size(), 0, nullptr, nullptr);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;  // timeout or interrupted; check g_running and retry
            }
            std::perror("recvfrom");
            break;
        }

        auto detection = sensor::parse(buffer.data(), static_cast<std::size_t>(n));
        if (!detection) {
            ++rejected;
            std::printf("Rejected packet (%zd bytes)\n", n);
            continue;
        }

        ++received;
        std::printf("id=%u range=%.1f m azimuth=%.1f deg t=%llu ms\n",
                    detection->id, detection->range_m, detection->azimuth_deg,
                    static_cast<unsigned long long>(detection->timestamp_ms));

        if (csv.is_open()) {
            csv << detection->id << ',' << detection->range_m << ','
                << detection->azimuth_deg << ',' << detection->timestamp_ms << '\n';
        }
    }

    std::printf("\nStopped. Received %lu valid, rejected %lu invalid.\n", received,
                rejected);
    close(fd);
    return 0;
}