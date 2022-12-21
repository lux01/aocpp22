//   Copyright 18/12/2022 William Marlow
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

#include <fstream>
#include <iostream>
#include <cstdint>
#include <map>
#include <string>
#include <set>
#include <algorithm>
#include <vector>
#include <bitset>

using Point = std::tuple<uint64_t, uint64_t>;
using HeightMap = std::map<Point, char>;

struct Mountain {
    const std::map<Point, char> vertices;
    const std::map<Point, std::bitset<4>> edges;
    std::map<Point, uint64_t> distances;
    Point start;
    Point end;

    explicit Mountain(const char* filename);

private:
    void calculateDistancesToEnd();
};

std::map<Point, char> parseVertices(const char* fileName) {
    std::map<Point, char> vertices;
    std::ifstream file(fileName);
    if (!file) {
        throw std::runtime_error(std::string("Failed to open file: ") + fileName);
    }
    std::string line;

    uint64_t height = 0;
    uint64_t width;
    while (std::getline(file, line)) {
        width = 0;
        for (char c: line) {
            vertices.emplace(Point{width, height}, c);
            width++;
        }
        height++;
    }
    return vertices;
}

char clampHeight(char h) {
    if (h == 'S') return 'a';
    if (h == 'E') return 'z';
    return h;
}

std::map<Point, std::bitset<4>> computeEdges(const std::map<Point, char>& vertices) {
    auto testEdge = [vertices](char myHeight, const Point& p) {
        auto it = vertices.find(p);
        if (it == vertices.end())
            return false;
        auto neighbourHeight = clampHeight(it->second);
        return neighbourHeight + 1 >= myHeight;
    };
    std::map<Point, std::bitset<4>> edges;

    for (const auto& [p, h] : vertices) {
        auto [x, y] = p;
        std::bitset<4> neighbours;
        neighbours.set(0, testEdge(clampHeight(h), Point{x - 1, y}));
        neighbours.set(1, testEdge(clampHeight(h), Point{x + 1, y}));
        neighbours.set(2, testEdge(clampHeight(h), Point{x, y - 1}));
        neighbours.set(3, testEdge(clampHeight(h), Point{x, y + 1}));
        edges[p] = neighbours;
    }

    return edges;
}

Mountain::Mountain(const char* fileName)
: vertices(parseVertices(fileName))
, edges(computeEdges(vertices))
{
    for (const auto& [p, h] : vertices) {
        if (h == 'S') {
            start = p;
        } else if (h == 'E') {
            end = p;
        }
    }

    calculateDistancesToEnd();
}

void Mountain::calculateDistancesToEnd() {
    std::set<Point> Q;

    // For each vertex v
    //  * set distance[v] to zero
    //  * add v to Q
    for (const auto& [point, _] : vertices) {
        distances[point] = UINT64_MAX;
        Q.insert(point);
    }
    distances[end] = 0;

    auto testNeighbour = [&](const Point& p, uint64_t tentativeDistance) {
        if (Q.contains(p) && tentativeDistance < distances[p]) {
            distances[p] = tentativeDistance;
        }
    };

    while (!Q.empty()) {
        auto point = *std::min_element(Q.begin(), Q.end(), [this](const Point& p1, const Point& p2) {
            return distances[p1] < distances[p2];
        });
        Q.erase(point);

        auto [x, y] = point;
        auto distance = distances[point];
        if (distance == UINT64_MAX) {
            continue;
        }
        auto tentativeDistance = distance + 1;
        auto neighbours = edges.at(point);
        if (neighbours.test(0)) {
            testNeighbour(Point{x - 1, y}, tentativeDistance);
        }
        if (neighbours.test(1)) {
            testNeighbour(Point{x + 1, y}, tentativeDistance);
        }
        if (neighbours.test(2)) {
            testNeighbour(Point{x, y - 1}, tentativeDistance);
        }
        if (neighbours.test(3)) {
            testNeighbour(Point{x, y + 1}, tentativeDistance);
        }
    }
}

int main(int argc, char** argv)
try {
    if (argc < 2) {
        std::cerr << "No input file specified" << std::endl;
        return 1;
    }
    Mountain mountain(argv[1]);
    auto part1 = mountain.distances[mountain.start];
    std::cout << "Part 1: " << part1 << std::endl;

    auto part2 = part1;
    for (const auto& [point, distance] : mountain.distances) {
        if (clampHeight(mountain.vertices.at(point)) == 'a' && distance < part2) {
            part2 = distance;
        }
    }
    std::cout << "Part 2: " << part2 << std::endl;
    return 0;
} catch (const std::exception& ex) {
    std::cerr << "ERROR: " << ex.what() << std::endl;
    return 1;
}