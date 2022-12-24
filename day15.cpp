//   Copyright 21/12/2022 William Marlow
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <format>
#include <string>
#include <complex>
#include <algorithm>
#include <numeric>
#include <optional>

using Point = std::pair<int, int>;

int metric(const Point &a, const Point &b) {
    return std::abs(a.first - b.first) + std::abs(a.second - b.second);
}

int parseNumber(std::string_view str) {
    auto endPtr = const_cast<char *>(str.data() + str.size());
    auto n = std::strtol(str.data(), &endPtr, 10);
    if (endPtr == str.data()) {
        throw std::runtime_error(std::format("Failed to parse string as number: '{}'", str));
    }
    return n;
}

struct Sensor {
    Point location;
    Point beacon;
    int strength;
    explicit Sensor(Point location, Point beacon, int strength)
    : location(location), beacon(beacon), strength(strength) {}
};

std::vector<Sensor> parse(std::istream &input) {
    std::string line;
    std::vector<Sensor> sensors;
    while (std::getline(input, line)) {
        size_t start = 12;
        size_t end = line.find(',', start);
        auto sensorX = parseNumber(line.substr(start, end - start));

        start = end + 4;
        end = line.find(':', start);
        auto sensorY = parseNumber(line.substr(start, end - start));

        start = end + 25;
        end = line.find(',', start);
        auto beaconX = parseNumber(line.substr(start, end - start));

        start = end + 4;
        auto beaconY = parseNumber(line.substr(start));

        Point sensor(sensorX, sensorY);
        Point beacon(beaconX, beaconY);
        sensors.emplace_back(sensor, beacon, metric(sensor, beacon));
    }

    return sensors;
}

/**
 * Given a sensor centred on (X, Y) with strength d
 * and a fixed y value, a SensorRange represents the
 * minimum and maximum x values that satisfy the
 * constraint.
 *
 * |x - X| <= d - |y - Y|
 */
struct SensorRange {
    int start;
    int end;

private:
    explicit SensorRange(int start, int end) : start(start), end(end) {}

public:
    static std::optional<SensorRange> fromSensor(const Sensor& sensor, int y) {
        auto dy = std::abs(sensor.location.second - y);
        if (dy > sensor.strength) {
            return std::nullopt;
        }
        auto reducedStrength = sensor.strength - dy;
        return SensorRange(sensor.location.first - reducedStrength, sensor.location.first + reducedStrength);
    }

    [[nodiscard]] bool overlaps(const SensorRange& other) const noexcept {
        return (start <= other.end && other.start <= end) || (end + 1 == other.start);
    }

    [[nodiscard]] SensorRange merge(const SensorRange& other) const noexcept {
        return SensorRange(std::min(start, other.start), std::max(end, other.end));
    }
};

void reduce(std::vector<SensorRange>& ranges, int minX, int maxX) {
    for (auto& range : ranges) {
        range.start = std::clamp(range.start, minX, maxX);
        range.end = std::clamp(range.end, minX, maxX);
    }

    std::sort(ranges.begin(), ranges.end(), [](const SensorRange& a, const SensorRange& b) {
        if (a.start == b.start) {
            return a.end < b.end;
        }
        return a.start < b.start;
    });

    for (size_t i = 0; i < ranges.size() - 1; i++) {
        while (i < ranges.size() - 1 && ranges[i].overlaps(ranges[i + 1])) {
            ranges[i] = ranges[i].merge(ranges[i + 1]);
            ranges.erase(ranges.begin() + i + 1);
        }
    }
}

auto getRanges(const std::vector<Sensor>& sensors, int minX, int maxX, int y) {
    std::vector<SensorRange> ranges;
    for (const auto& sensor : sensors) {
        auto range = SensorRange::fromSensor(sensor, y);
        if (range.has_value()) {
            ranges.push_back(range.value());
        }
    }
    reduce(ranges, minX, maxX);
    return ranges;
}

auto part1(const std::vector<Sensor>& sensors, int row) {
    int xMin = INT_MAX;
    int xMax = INT_MIN;
    int strengthMax = 0;
    for (const auto &[location, beacon, strength]: sensors) {
        xMin = std::min(xMin, std::min(location.first, beacon.first));
        xMax = std::max(xMax, std::max(location.first, beacon.first));
        strengthMax = std::max(strengthMax, strength);
    }
    auto ranges = getRanges(sensors, xMin - strengthMax, xMax + strengthMax, row);
    return std::transform_reduce(ranges.begin(), ranges.end(), 0, std::plus(), [](const SensorRange& range) {
        return range.end - range.start;
    });
}

void part2(const std::vector<Sensor>& sensors, int row) {
    for (int y = 0; y <= 2 * row; y++) {
        auto ranges = getRanges(sensors, 0, 2 * row, y);
        if (ranges.size() == 2) {
            auto x = ranges[0].end + 1;
            std::cout << std::format("{}{:06}", 4*x, y) << std::endl;
            return;
        }
    }
}

int main(int argc, char **argv)
try {
    for (int i = 2; i < argc; i += 2) {
        std::fstream file(argv[i - 1]);
        int row = parseNumber(argv[i]);

        auto sensors = parse(file);

        std::cout << "Part 1: " << part1(sensors, row) << std::endl;
        std::cout << "Part 2: ";
        part2(sensors, row);
        std::cout << std::endl;
    }
    return 0;
} catch (const std::exception &ex) {
    std::cerr << "ERROR: " << ex.what() << std::endl;
    return 1;
}