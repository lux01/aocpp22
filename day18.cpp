//   Copyright 28/12/2022 William Marlow
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
#include <format>
#include <exception>
#include <set>
#include <string>
#include <numeric>
#include <algorithm>

uint32_t parseNumber(std::string_view str) {
    auto endPtr = const_cast<char *>(str.data() + str.size());
    auto n = std::strtoul(str.data(), &endPtr, 10);
    if (endPtr == str.data()) {
        throw std::runtime_error(std::format("Failed to parse string as number: '{}'", str));
    }
    return n;
}

struct Cube {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t z = 0;

    constexpr auto operator<=>(const Cube &other) const noexcept {
        auto cmpX = x <=> other.x;
        if (cmpX != std::strong_ordering::equal) {
            return cmpX;
        }
        auto cmpY = y <=> other.y;
        if (cmpY != std::strong_ordering::equal) {
            return cmpY;
        }
        return z <=> other.z;
    }
};

std::set<Cube> parse(std::istream &input) {
    std::set<Cube> cubes;
    std::string line;
    while (std::getline(input, line)) {
        auto comma = line.find(',');
        auto comma2 = line.find(',', comma + 1);

        std::string_view l = line;

        auto x = parseNumber(l.substr(0, comma));
        auto y = parseNumber(l.substr(comma + 1, comma2 - comma - 1));
        auto z = parseNumber(l.substr(comma2 + 1));

        cubes.insert({x, y, z});
    }
    return cubes;
}

auto surfaceArea(const std::set<Cube> &cubes) {
    size_t count = 0;
    for (const auto& [x, y, z] : cubes) {
        count += !cubes.contains({x - 1, y, z});
        count += !cubes.contains({x + 1, y, z});
        count += !cubes.contains({x, y - 1, z});
        count += !cubes.contains({x, y + 1, z});
        count += !cubes.contains({x, y, z - 1});
        count += !cubes.contains({x, y, z + 1});
    }
    return count;
}

template <typename Fn>
void xyPlane(const Cube& min, const Cube& max, Fn&& fn) {
    for (uint32_t y = min.y; y <= max.y; y++) {
        for (uint32_t x = min.x; x <= max.x; x++) {
            fn(x, y);
        }
    }
}

template <typename Fn>
void xzPlane(const Cube& min, const Cube& max, Fn&& fn) {
    for (uint32_t z = min.z; z <= max.z; z++) {
        for (uint32_t x = min.x; x <= max.x; x++) {
            fn(x, z);
        }
    }
}

template <typename Fn>
void yzPlane(const Cube& min, const Cube& max, Fn&& fn) {
    for (uint32_t z = min.z; z <= max.z; z++) {
        for (uint32_t y = min.y; y <= max.y; y++) {
            fn(y, z);
        }
    }
}

std::set<Cube> findAirPockets(const std::set<Cube>& cubes) {
    // Determine the bounding box of the cube
    auto min = std::reduce(cubes.cbegin(), cubes.cend(), Cube{UINT32_MAX, UINT32_MAX, UINT32_MAX},
                           [](const Cube& a, const Cube& b) { return Cube{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)}; });
    auto max = std::reduce(cubes.cbegin(), cubes.cend(), Cube{0, 0, 0},
                           [](const Cube& a, const Cube& b) { return Cube{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}; });

    // Construct the negative of the droplet, this is the set of all air bubbles and the surrounding air
    std::set<Cube> negative;
    for (auto z = min.z; z <= max.z; z++) {
        for (auto y = min.y; y <= max.y; y++) {
            for (auto x = min.x; x <= max.x; x++) {
                if (!cubes.contains({x, y, z})) {
                    negative.insert({x, y, z});
                }
            }
        }
    }

    // Create a subset of the negative which are all the points on the outer boundary of the negative
    std::set<Cube> unvisitedNegativeCubes;

    xyPlane(min, max, [&](uint32_t x, uint32_t y) {
        auto minCube = Cube{x, y, min.z};
        auto maxCube = Cube{x, y, max.z};
        if (negative.contains(minCube)) unvisitedNegativeCubes.insert(minCube);
        if (negative.contains(maxCube)) unvisitedNegativeCubes.insert(maxCube);
    });
    xzPlane(min, max, [&](uint32_t x, uint32_t z) {
        auto minCube = Cube{x, min.y, z};
        auto maxCube = Cube{x, max.y, z};
        if (negative.contains(minCube)) unvisitedNegativeCubes.insert(minCube);
        if (negative.contains(maxCube)) unvisitedNegativeCubes.insert(maxCube);
    });
    yzPlane(min, max, [&](uint32_t y, uint32_t z) {
        auto minCube = Cube{max.x, y, z};
        auto maxCube = Cube{max.x, y, z};
        if (negative.contains(minCube)) unvisitedNegativeCubes.insert(minCube);
        if (negative.contains(maxCube)) unvisitedNegativeCubes.insert(maxCube);
    });

    // For each element in the unvisited set, add all its neighbours that are in the negative
    // to the unvisited set and remove the element from the negative and the unvisited set
    while (!unvisitedNegativeCubes.empty()) {
        auto it = std::min_element(unvisitedNegativeCubes.begin(), unvisitedNegativeCubes.end());
        auto cube = *it;
        unvisitedNegativeCubes.erase(it);
        negative.erase(cube);

        auto [x, y, z] = cube;
        if (negative.contains({x - 1, y, z})) unvisitedNegativeCubes.insert({x - 1, y, z});
        if (negative.contains({x + 1, y, z})) unvisitedNegativeCubes.insert({x + 1, y, z});
        if (negative.contains({x, y - 1, z})) unvisitedNegativeCubes.insert({x, y - 1, z});
        if (negative.contains({x, y + 1, z})) unvisitedNegativeCubes.insert({x, y + 1, z});
        if (negative.contains({x, y, z - 1})) unvisitedNegativeCubes.insert({x, y, z - 1});
        if (negative.contains({x, y, z + 1})) unvisitedNegativeCubes.insert({x, y, z + 1});
    }
    return negative;
}

int main(int argc, char **argv)
try {
    for (int i = 1; i < argc; i++) {
        auto input = std::ifstream(argv[i]);
        auto cubes = parse(input);

        auto part1 = surfaceArea(cubes);
        auto air = findAirPockets(cubes);
        auto part2 = part1 - surfaceArea(air);
        std::cout << "Part 1: " << part1 << std::endl;
        std::cout << "Part 2: " << part2 << std::endl;
        std::cout << std::endl;
    }
    return 0;
} catch (const std::exception &ex) {
    std::cerr << "ERROR: " << ex.what() << std::endl;
    return 1;
}