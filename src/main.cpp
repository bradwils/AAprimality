// Single entry point for all primality tests: run one or all algorithms on
// given numbers, or benchmark them over an input file with CSV output.

#include <chrono>
#include <cstdlib>
#include <fstream>
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
    std::string input;
    int rounds = 20;
    u64 seed = 42;
    int reps = 5;
    int warmup = 1;
};

void usage() {
    std::cerr <<
        "usage:\n"
        "  primality list\n"
        "  primality run   <algo|all> <n>... [--rounds K] [--seed S]\n"
        "  primality bench <algo|all> --input FILE [--reps R] [--warmup W]\n"
        "                  [--rounds K] [--seed S]\n"
        "\n"
        "bench writes CSV to stdout: algorithm,n,bits,rep,result,ns\n";
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
        else if (arg == "--reps") o.reps = std::stoi(value());
        else if (arg == "--warmup") o.warmup = std::stoi(value());
        else if (arg == "--input") o.input = value();
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

// One number per line; blank lines and lines starting with '#' are skipped.
std::vector<u64> read_numbers(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open " + path);
    std::vector<u64> nums;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        nums.push_back(parse_u64(line));
    }
    return nums;
}

int bit_length(u64 n) { return n == 0 ? 0 : 64 - __builtin_clzll(n); }

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

int cmd_bench(const Options& o) {
    if (o.input.empty()) throw std::invalid_argument("bench needs --input FILE");
    const std::vector<u64> nums = read_numbers(o.input);
    volatile bool sink = false;  // stops the compiler discarding results

    std::cout << "algorithm,n,bits,rep,result,ns\n";
    for (const Algorithm* a : select_algorithms(o.algo)) {
        std::mt19937_64 rng(o.seed);
        TestParams p{o.rounds, rng};
        try {
            for (u64 n : nums) {
                for (int w = 0; w < o.warmup; ++w) sink = a->fn(n, p);
                for (int r = 0; r < o.reps; ++r) {
                    auto t0 = std::chrono::steady_clock::now();
                    bool result = a->fn(n, p);
                    auto t1 = std::chrono::steady_clock::now();
                    sink = result;
                    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
                    std::cout << a->name << "," << n << "," << bit_length(n) << ","
                              << r << "," << result << "," << ns << "\n";
                }
            }
        } catch (const std::logic_error& e) {
            std::cerr << "skipping " << a->name << ": " << e.what() << "\n";
        }
    }
    (void)sink;
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Options o = parse_args(argc, argv);
        if (o.command == "list") return cmd_list();
        if (o.command == "run") return cmd_run(o);
        if (o.command == "bench") return cmd_bench(o);
        throw std::invalid_argument("unknown command: " + o.command);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n\n";
        usage();
        return EXIT_FAILURE;
    }
}
