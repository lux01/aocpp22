//   Copyright 24/12/2022 William Marlow
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

#include <format>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <utility>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <numeric>
#include <algorithm>
#include <set>
#include <deque>

size_t parseNumber(std::string_view str) {
    auto endPtr = const_cast<char *>(str.data() + str.size());
    auto n = std::strtoull(str.data(), &endPtr, 10);
    if (endPtr == str.data()) {
        throw std::runtime_error(std::format("Failed to parse string as number: '{}'", str));
    }
    return n;
}

struct Valve;
using ValveNetwork = std::unordered_map<std::string, std::unique_ptr<Valve>>;

struct Valve {
    size_t flowRate = 0;
    const std::string label;
private:
    const std::vector<std::string> connections;
public:
    std::unordered_map<const Valve*, std::vector<const Valve*>> routes;
    std::unordered_map<const Valve*, size_t> distances;

    explicit Valve(size_t flowRate, std::string label, std::vector<std::string> connections)
            : flowRate(flowRate), label(std::move(label)), connections(std::move(connections)) {}

    Valve() = default;
    Valve(const Valve &) = default;
    Valve(Valve &&) noexcept = default;

    void calculateDistanceToOtherValves(const ValveNetwork &network);
};

auto parse(std::istream &input) {
    ValveNetwork valves;
    std::string line;
    while (std::getline(input, line)) {
        auto name = line.substr(6, 2);
        auto semi = line.find(';');
        auto flowRate = parseNumber(line.operator std::string_view().substr(23, semi - 23));

        auto start = line.find("to valves ", semi);
        if (start == std::string::npos) {
            start = line.find("to valve ", semi);
            start += 9;
        } else {
            start += 10;
        }
        auto next = line.find(", ", start);
        std::vector<std::string> destinations;
        while (start != std::string::npos) {
            destinations.emplace_back(std::move(line.substr(start, next - start)));

            if (next == std::string::npos) {
                break;
            }
            start = next + 2;
            next = line.find(", ", start);
        }
        auto valve = std::make_unique<Valve>(flowRate, name, std::move(destinations));
        valves[valve->label] = std::move(valve);
    }

    for (auto &[_, valve]: valves) {
        valve->calculateDistanceToOtherValves(valves);
    }
    return valves;
}

struct DistanceSorter {
    const std::unordered_map<const Valve*, size_t> &distances;

    bool operator()(const Valve* a, const Valve* b) const {
        auto da = distances.at(a);
        auto db = distances.at(b);
        if (da == db) {
            return a < b;
        }
        return da < db;
    }
};

void Valve::calculateDistanceToOtherValves(const ValveNetwork &network) {
    // Dijkstra's algorithm
    DistanceSorter sorter{distances};
    std::set<const Valve*> Q;
    std::unordered_map<const Valve*, const Valve*> prev;

    for (const auto &[l, v]: network) {
        distances[v.get()] = UINT32_MAX;
        auto [it, inserted] = Q.emplace(v.get());
        if (!inserted) {
            throw std::runtime_error(std::format("Failed to insert {} into distance map", l));
        }
    }
    distances[this] = 0;

    while (!Q.empty()) {
        auto it = std::min_element(Q.begin(), Q.end(), sorter);
        auto valve = *it;
        Q.erase(it);

        auto distance = distances[valve];
        if (distance == UINT32_MAX) {
            throw std::runtime_error(std::format("Processing node {} which has infinite distance", valve->label));
        }

        for (const auto &neighbour: valve->connections) {
            auto u = network.at(neighbour).get();
            if (Q.contains(u)) {
                auto alt = distance + 1;
                auto &neighbourDistance = distances[u];
                if (alt < neighbourDistance) {
                    neighbourDistance = alt;
                    prev[u] = valve;
                }
            }
        }
    }
}

struct State {
    // The valve we're currently at
    const Valve* current;
    // All the valves opened in our history with the time they were opened at
    std::unordered_map<const Valve*, size_t> openedValves;
    // How many minutes have elapsed since we started
    size_t elapsedTime;

    State(const Valve* current, std::unordered_map<const Valve*, size_t> openedValves, size_t elapsedTime)
    : current(current), openedValves(std::move(openedValves)), elapsedTime(elapsedTime) {}

    constexpr auto operator<=>(const State &state) const noexcept { return elapsedTime <=> state.elapsedTime; }

    [[nodiscard]] size_t currentPressurePerMinute() const {
        return std::transform_reduce(openedValves.begin(), openedValves.end(), size_t(0), std::plus(),
                           [](const auto& kv) -> size_t {
            return kv.first->flowRate;
        });
    }

    [[nodiscard]] size_t totalPressureRelieved(size_t timeLimit) const {
        return std::transform_reduce(openedValves.begin(), openedValves.end(), size_t(0), std::plus(),
                                     [timeLimit](const auto& kv) -> size_t {
            if (kv.second >= timeLimit)
                return 0;
            return kv.first->flowRate * (timeLimit - kv.second);
        });
    }
};

size_t part1(const ValveNetwork &network, size_t timeLimit) {
    std::deque<State> states;
    auto start = network.at("AA").get();
    State maxState(start, {}, 0);
    states.push_back(maxState);
    size_t max = 0;

    while (!states.empty()) {
        // Pop a possible state from the queue
        auto state = states.front();
        states.pop_front();

        bool addedAnyStates = false;
        // For each other valve in the network
        for (const auto &[target, distance]: state.current->distances) {
            // If that valve is not yet open in this state
            if (!state.openedValves.contains(target) && target->flowRate > 0 && state.elapsedTime < timeLimit) {
                // Create a new state that represents spending 'distance' minutes moving to that point and opening
                // that valve in the next minute
                std::unordered_map<const Valve*, size_t> newOpenedValves = state.openedValves;
                newOpenedValves[target] = state.elapsedTime + distance + 1;
                State nextState(target, newOpenedValves, state.elapsedTime + distance + 1);
                states.emplace_back(std::move(nextState));
                addedAnyStates = true;
            }
        }

        if (!addedAnyStates) {
            auto finalPressure = state.totalPressureRelieved(timeLimit);
            if (finalPressure > max) {
                max = finalPressure;
                maxState = state;
            }
        }
    }

    std::cout << "Final state is at time " << maxState.elapsedTime << " with valves\n";

    for (const auto& kv : maxState.openedValves) {
        std::cout << " * " << kv.first->label << " opened at minute " << kv.second << '\n';
    }
    std::cout << "releasing " << maxState.currentPressurePerMinute() << " pressure per minute for a total of " << max << std::endl;

    return max;
}

int main(int argc, char **argv)
try {
    for (int i = 1; i < argc; i++) {
        auto input = std::ifstream(argv[i]);
        auto network = parse(input);
        std::cout << "Part 1: " << part1(network, 30) << std::endl;
    }
    return 0;
} catch (const std::exception &ex) {
    std::cerr << "ERROR: " << ex.what() << std::endl;
    return 1;
}