// Run one or all algorithms on given numbers. Tests and benchmarks are in
// tests/tests.cpp and bench/bench.cpp.

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "primality.hpp"

namespace {

struct Options {
    std::string command;
    std::string algo;
    std::vector<u64> numbers;
    int rounds = DEFAULT_ROUNDS;
    u64 seed = 42;
};

void usage() {
    std::cerr <<
        "usage:\n"
        "  primality list\n"
        "  primality run <algo|all> <n>... [--rounds K] [--seed S]\n";
}

u64 parse_u64(const std::string& s) {
    size_t pos = 0;
    u64 v = std::stoull(s, &pos);
    if (pos != s.size()) throw std::invalid_argument("not a number: " + s);
    return v;
}

Options parse_args(int argc, char** argv) {
    if (argc < 2) throw std::invalid_argument("missing command");
    Options o;
    o.command = argv[1];
    if (o.command == "list") return o;
    if (argc < 3) throw std::invalid_argument("missing algorithm");
    o.algo = argv[2];

    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(arg + " needs a value");
            return argv[++i];
        };
        if (arg == "--rounds") o.rounds = std::stoi(value());
        else if (arg == "--seed") o.seed = parse_u64(value());
        else o.numbers.push_back(parse_u64(arg));
    }
    return o;
}

std::vector<const Algorithm*> select_algorithms(const std::string& name) {
    std::vector<const Algorithm*> out;
    for (const auto& a : ALGORITHMS)
        if (name == "all" || name == a.name) out.push_back(&a);
    if (out.empty()) throw std::invalid_argument("unknown algorithm: " + name);
    return out;
}

int cmd_list() {
    for (const auto& a : ALGORITHMS)
        std::cout << a.name << "\t" << a.author << "\t"
                  << (a.randomized ? "randomized" : "deterministic") << "\n";
    return 0;
}

int cmd_run(const Options& o) {
    if (o.numbers.empty()) throw std::invalid_argument("run needs at least one number");
    for (const Algorithm* a : select_algorithms(o.algo)) {
        // Reseed per algorithm so results don't depend on which others ran first.
        std::mt19937_64 rng(o.seed);
        TestParams p{o.rounds, rng};
        for (u64 n : o.numbers) {
            std::cout << a->name << "\t" << n << "\t";
            try {
                std::cout << (a->fn(n, p) ? "prime" : "composite") << "\n";
            } catch (const std::logic_error& e) {
                std::cout << "error: " << e.what() << "\n";
            }
        }
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Options o = parse_args(argc, argv);
        if (o.command == "list") return cmd_list();
        if (o.command == "run") return cmd_run(o);
        throw std::invalid_argument("unknown command: " + o.command);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n\n";
        usage();
        return EXIT_FAILURE;
    }
}
