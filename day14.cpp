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
#include <string>
#include <set>
#include <vector>
#include <numeric>

using Point = std::tuple<int, int>;
using OccupiedSpots = std::set<Point>;

Point operator+(Point a, Point b) {
    return {std::get<0>(a) + std::get<0>(b), std::get<1>(a) + std::get<1>(b)};
}

Point operator-(Point a, Point b) {
    return {std::get<0>(a) - std::get<0>(b), std::get<1>(a) - std::get<1>(b)};
}

int parseNumber(std::string_view str) {
    auto endPtr = const_cast<char *>(str.data() + str.size());
    auto n = std::strtol(str.data(), &endPtr, 10);
    if (endPtr == str.data()) {
        std::string message = "Failed to parse string as number: '";
        message.append(str);
        message.push_back('\'');
        throw std::runtime_error(message);
    }
    return n;
}

std::vector<Point> parseLine(std::string_view line) {
    std::vector<Point> points;
    for (size_t split = line.find(" -> "), start = 0;
         start != std::string_view::npos;
         start = (split == std::string_view::npos) ? split : split + 4, split = line.find(" -> ", start + 1)
            ) {
        auto comma = line.find(',', start);
        auto x = parseNumber(line.substr(start, comma - start));
        auto y = parseNumber(line.substr(comma + 1, split - comma - 1));
        points.emplace_back(x, y);
    }
    return points;
}

template<typename T>
T sign(T val) {
    return (T(0) < val) - (val < T(0));
}

OccupiedSpots parse(std::istream &input) {
    OccupiedSpots rocks;
    std::string line;
    while (std::getline(input, line)) {
        auto points = parseLine(line);
        for (size_t i = 0; i < points.size() - 1; i++) {
            auto [startX, startY] = points[i];
            auto [endX, endY] = points[i + 1];
            auto dx = sign(endX - startX);
            auto dy = sign(endY - startY);

            for (auto x = startX, y = startY; (dx == 0 || x != endX) && (dy == 0 || y != endY); x += dx, y += dy) {
                rocks.insert({x, y});
            }
            rocks.insert({endX, endY});
        }
    }
    return rocks;
}

struct Grain {
    bool hitFloor = false;
    int x = 500;
    int y = 0;

    void findRestingPoint(const OccupiedSpots &occupiedSpots, int floor);
};

void Grain::findRestingPoint(const OccupiedSpots &occupiedSpots, int floor) {
    while (y <= floor) {
        // Check that the grain is still in bounds
        if (!occupiedSpots.contains({x, y + 1})) {
            y += 1;
        } else if (!occupiedSpots.contains({x - 1, y + 1})) {
            y += 1;
            x -= 1;
        } else if (!occupiedSpots.contains({x + 1, y + 1})) {
            y += 1;
            x += 1;
        } else {
            return;
        }
    }
}

int main(int argc, char **argv)
try {
    for (int i = 1; i < argc; i++) {
        std::fstream file(argv[i]);
        if (!file) {
            std::cerr << "Failed to open input file " << argv[i] << std::endl;
            return 1;
        }

        auto occupiedSpots = parse(file);

        auto floor = std::transform_reduce(occupiedSpots.begin(), occupiedSpots.end(), 0,
                                           [](int a, int b) { return std::max(a, b); },
                                           [](const Point &point) { return std::get<1>(point); });

        auto part1 = 0;
        auto part2 = 0;
        bool floorHit = false;

        while (true) {
            Grain grain;
            grain.findRestingPoint(occupiedSpots, floor);
            if (!floorHit) {
                if (grain.y >= floor) {
                    floorHit = true;
                } else {
                    part1++;
                }
            }
            occupiedSpots.emplace(grain.x, grain.y);
            part2++;
            if (grain.x == 500 && grain.y == 0) {
                break;
            }
        }
        std::cout << "Part 1: " << part1 << std::endl;
        std::cout << "Part 2: " << part2 << std::endl;
        std::cout << std::endl;
    }
    return 0;
} catch (const std::exception &ex) {
    std::cerr << "ERROR: " << ex.what() << std::endl;
    return 1;
}
