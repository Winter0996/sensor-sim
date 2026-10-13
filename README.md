# sensor-sim

A small C++17 project that simulates the messaging side of a radar sensor system: synthetic "detections" are serialized into a fixed binary format and parsed back with validation. Built to practice systems-level C++ on Linux with automated testing.

## Status

Work in progress.

- [x] Binary serializer/parser with input validation
- [x] Google Test suite (round trip, byte layout, invalid input)
- [ ] UDP sender and receiver
- [ ] Sequence numbers to detect dropped or out-of-order packets
- [ ] CI pipeline (build, test, static analysis)

## Wire format

Little-endian, no padding, 20 bytes per message:

| Field        | Type     | Bytes |
|--------------|----------|-------|
| id           | uint32   | 4     |
| range_m      | float32  | 4     |
| azimuth_deg  | float32  | 4     |
| timestamp_ms | uint64   | 8     |

Fields are written one at a time instead of copying a struct, so the layout does not depend on compiler padding or the host's byte order. The parser rejects wrong-size buffers, null pointers, negative or non-finite ranges, and azimuths outside [0, 360).

## Build and test

Requires a C++17 compiler, CMake 3.14+, and internet access on the first configure (Google Test is fetched automatically).

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

## Layout

    src/parser.h, src/parser.cpp   serialize/parse library
    tests/test_parser.cpp          Google Test suite
    CMakeLists.txt                 build configuration
