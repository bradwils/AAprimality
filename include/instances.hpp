// Loader for data/instances.csv, written by scripts/gen_data.py.
#pragma once

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "primality.hpp"

struct Instance {
    std::string family;
    int bits;
    u64 n;
    bool prime; // expected result, from the Python prime check in gen_data.py
};

inline std::vector<Instance> load_instances(const std::string& path = "data/instances.csv") {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open " + path + " (run scripts/gen_data.py from the repo root)");
    std::vector<Instance> out;
    std::string line, family, bits, n, label;
    std::getline(in, line); // header
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::getline(ss, family, ',');
        std::getline(ss, bits, ',');
        std::getline(ss, n, ',');
        std::getline(ss, label, ',');
        out.push_back({family, std::stoi(bits), std::stoull(n), label == "prime"});
    }
    return out;
}

inline std::vector<Instance> of_family(const std::vector<Instance>& all, const std::string& family, int lo, int hi) {
    std::vector<Instance> v;
    for (const Instance& i : all)
        if (i.family == family && i.bits >= lo && i.bits <= hi) v.push_back(i);
    return v;
}
