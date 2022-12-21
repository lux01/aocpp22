//   Copyright 20/12/2022 William Marlow
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
#include <vector>
#include <string>
#include <format>
#include <fstream>

#include <boost/json.hpp>

auto parseInput(std::istream& input) {
    std::string line;
    std::vector<std::tuple<boost::json::value, boost::json::value>> packetPairs;
    while (std::getline(input, line)) {
        auto p1 = boost::json::parse(line);

        std::getline(input, line);
        auto p2 = boost::json::parse(line);

        std::getline(input, line);
        packetPairs.emplace_back(p1, p2);
    }
    return packetPairs;
}

std::strong_ordering operator<=>(const boost::json::value&, const boost::json::value&);

std::strong_ordering operator<=>(const boost::json::array& first, const boost::json::array& second) {
    for (size_t i = 0; i < std::min(first.size(), second.size()); i++) {
        auto result = first[i] <=> second[i];
        if (result != std::strong_ordering::equal)
            return result;
    }
    return first.size() <=> second.size();
}

std::strong_ordering operator<=>(const boost::json::value& first, const boost::json::value& second) {
    if (first.is_number() && second.is_number()) {
        return first.as_int64() <=> second.as_int64();
    }
    if (first.is_number() && second.is_array()) {
        boost::json::array tmp;
        tmp.push_back(first);
        return tmp <=> second.as_array();
    }
    if (first.is_array() && second.is_number()) {
        boost::json::array tmp;
        tmp.push_back(second);
        return first.as_array() <=> tmp;
    }
    const auto& a1 = first.as_array();
    const auto& a2 = second.as_array();
    return a1 <=> a2;
}

int main(int argc, char** argv)
try {
    if (argc < 2) {
        std::cerr << "No input file specified" << std::endl;
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        std::cout << "Input file: " << argv[i] << std::endl;
        std::ifstream file(argv[i]);

        auto packetPairs = parseInput(file);

        auto part1 = 0ULL;
        size_t index = 1;
        for (const auto& [first, second] : packetPairs) {
            if (first < second) {
                part1 += index;
            }
            index++;
        }
        std::cout << "Part 1: " << part1 << std::endl;


        boost::json::value two = boost::json::parse("[[2]]");
        boost::json::value six = boost::json::parse("[[6]]");
        std::vector<const boost::json::value*> packets;
        for (const auto& [first, second] : packetPairs) {
            packets.push_back(&first);
            packets.push_back(&second);
        }
        packets.push_back(&two);
        packets.push_back(&six);

        std::sort(packets.begin(), packets.end(), [](const boost::json::value* a, const boost::json::value* b) {
            return *a < *b;
        });

        auto twoIdx = std::find(packets.begin(), packets.end(), &two);
        auto sixIdx = std::find(twoIdx, packets.end(), &six);

        auto part2 = (1 + std::distance(packets.begin(), twoIdx)) * (1 + std::distance(packets.begin(), sixIdx));
        std::cout << "Part 2: " << part2 << std::endl;
        std::cout << std::endl;
    }
    return 0;
} catch (const std::exception& ex) {
    std::cerr << "ERROR: " << ex.what() << std::endl;
    return 1;
}