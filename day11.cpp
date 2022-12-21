//   Copyright 19/12/2022 William Marlow
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

#include <cstdint>
#include <utility>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>

enum class ArithmeticOperation : uint8_t {
    Add,
    Subtract,
    Multiply,
    Divide,
};

using Number = uint64_t;

Number parseNumber(std::string_view str) {
    auto endPtr = const_cast<char *>(str.data() + str.size());
    auto n = std::strtoull(str.data(), &endPtr, 10);
    if (endPtr == str.data()) {
        std::string message = "Failed to parse string as number: '";
        message.append(str);
        message.push_back('\'');
        throw std::runtime_error(message);
    }
    return n;
}

struct Operation {
    ArithmeticOperation operation;
    Number lhs;
    Number rhs;

    explicit Operation(ArithmeticOperation op, Number l, Number r)
    : operation(op), lhs(std::move(l)), rhs(std::move(r))
    {}

    [[nodiscard]] Number evaluate(const Number& old) const {
        const auto& l = (lhs == 0) ? old : lhs;
        const auto& r = (rhs == 0) ? old : rhs;

        switch (operation) {
            case ArithmeticOperation::Add: return l + r;
            case ArithmeticOperation::Subtract: return l - r;
            case ArithmeticOperation::Multiply: return l * r;
            case ArithmeticOperation::Divide: return l / r;
        }
        throw std::runtime_error("Unreachable condition");
    }
};

struct Monkey {
    std::vector<Number> items;
    Operation operation;
    size_t divisor;
    size_t trueTarget;
    size_t falseTarget;
    size_t itemsHandled = 0;

    Monkey(std::vector<Number> items, Operation operation, size_t divisor, size_t trueTarget,
           size_t falseTarget) : items(std::move(items)), operation(operation), divisor(divisor), trueTarget(trueTarget),
                                 falseTarget(falseTarget) {}

    void runTurn(std::vector<Monkey>& monkeys, Number modulo, bool relaxing) {
        std::vector<Number> mishandledItems;
        mishandledItems.reserve(items.size());

        std::transform(items.begin(), items.end(), std::back_inserter(mishandledItems),
                       [this, relaxing, modulo](const Number& worryLevel) {
            Number newWorryLevel = operation.evaluate(worryLevel) % modulo;
            if (relaxing) {
                newWorryLevel /= 3;
            }
            return newWorryLevel;
        });
        auto partitionPoint = std::partition(mishandledItems.begin(), mishandledItems.end(), [this](const Number& worryLevel) {
            return worryLevel % divisor == 0;
        });
        size_t trueItems = 0;
        size_t falseItems = 0;
        for (auto it = mishandledItems.begin(); it != partitionPoint; it++) {
            monkeys[trueTarget].items.push_back(*it);
            trueItems++;
        }
        for (auto it = partitionPoint; it != mishandledItems.end(); it++) {
            monkeys[falseTarget].items.push_back(*it);
            falseItems++;
        }
        itemsHandled += items.size();
        items.clear();
    }
};

namespace {
    std::vector<Number> parseItems(std::string_view description) {
        std::vector<Number> items;

        size_t start = 0;
        size_t comma = description.find(", ");
        while (true) {
            auto n = description.substr(start, comma - start);
            items.push_back(parseNumber(n));

            start = comma;
            if (start == std::string_view::npos) {
                break;
            }
            start += 2;
            comma = description.find(", ", start + 1);
        }
        return items;
    }

    Operation parseOperation(std::string_view description) {
        auto space = description.find(' ');
        std::string_view lhsString = description.substr(0, space);

        auto start = space + 1;
        space = description.find(' ', start + 1);
        std::string_view operationString = description.substr(start, space - start);

        start = space + 1;
        space = description.find_first_of(" \r\n", start + 1);
        std::string_view rhsString = description.substr(start, space - start);

        Number lhs = 0;
        Number rhs = 0;
        if (lhsString != "old") {
            lhs = parseNumber(lhsString);
        }
        if (rhsString != "old") {
            rhs = parseNumber(rhsString);
        }

        if (operationString == "/") {
            return Operation(ArithmeticOperation::Divide, lhs, rhs);
        } else if (operationString == "*") {
            return Operation(ArithmeticOperation::Multiply, lhs, rhs);
        } else if (operationString == "+") {
            return Operation(ArithmeticOperation::Add, lhs, rhs);
        } else {
            return Operation(ArithmeticOperation::Subtract, lhs, rhs);
        }
    }
}

std::vector<Monkey> parseMonkeys(std::istream& input) {
    std::vector<Monkey> monkeys;
    std::string line;
    while (std::getline(input, line)) {
        // Skip over Monkey line

        // Items line
        std::getline(input, line);
        auto items = parseItems(std::string_view (line).substr(18));

        // Operation line
        std::getline(input, line);
        auto operation = parseOperation(std::string_view(line).substr(19));

        // Test line
        std::getline(input, line);
        auto test = parseNumber(std::string_view(line).substr(21));

        // True target line
        std::getline(input, line);
        auto trueTarget = parseNumber(std::string_view(line).substr(29));

        // False target line
        std::getline(input, line);
        auto falseTarget = parseNumber(std::string_view(line).substr(30));

        monkeys.emplace_back(std::move(items), operation, test, trueTarget, falseTarget);

        // Blank line
        std::getline(input, line);
    }

    return monkeys;
}

size_t monkeyBusiness(std::vector<Monkey> monkeys, size_t iterations, bool relaxing) {
    auto modulo = std::transform_reduce(monkeys.begin(), monkeys.end(), Number(1), std::multiplies(), [](const Monkey& monkey) {
        return monkey.divisor;
    });
    for (size_t i = 0; i < iterations; i++) {
        for (auto& monkey : monkeys) {
            monkey.runTurn(monkeys, modulo, relaxing);
        }
    }

    std::sort(monkeys.begin(), monkeys.end(), [](const Monkey& a, const Monkey& b) {
        return a.itemsHandled > b.itemsHandled;
    });
    return monkeys[0].itemsHandled * monkeys[1].itemsHandled;
}

int main(int argc, char** argv)
try {
    if (argc < 2) {
        std::cerr << "No input file specified" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        throw std::runtime_error(std::string("Failed to open file ") + argv[1]);
    }
    std::vector<Monkey> monkeys = parseMonkeys(file);

    auto part1 = monkeyBusiness(monkeys, 20, true);
    std::cout << "Part 1: " << part1 << std::endl;
    auto part2 = monkeyBusiness(monkeys, 10000, false);
    std::cout << "Part 2: " << part2 << std::endl;

    return 0;
} catch (const std::exception& ex) {
    std::cerr << "ERROR: " << ex.what() << std::endl;
    return 1;
}