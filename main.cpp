#include "include/dual_pivot_quicksort.hpp"
#include "include/Random_Splitting_Sort.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

enum class AlgoMode {
    All,
    RandomSplitting,
    StdSort,
    DualPivot
};

struct TimingStats {
    double avg_ms = 0.0;
    double min_ms = 0.0;
    double max_ms = 0.0;
};

struct AppConfig {
    std::vector<std::size_t> sizes = {0, 1, 2, 10, 100, 1000, 10'000, 100'000, 1'000'000};
    std::uint32_t seed = 20260302u;
    int correctness_cases = 3000;
    int trials_small = 200;
    int trials_medium = 100;
    int trials_large = 30;
    int trials_huge = 10;
    int value_low = -100000;
    int value_high = 100000;
    std::size_t max_safe_elements = 2'000'000;
    double max_safe_ram_gib = 0.5;
    long long max_safe_single_run_ms = 5000;
    rssort::SortConfig rs_cfg{};
    AlgoMode algo = AlgoMode::All;
};

static std::vector<int> make_random_vector(std::size_t n, int low, int high, std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> value_dist(low, high);
    std::vector<int> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        out.push_back(value_dist(rng));
    }
    return out;
}

static bool parse_sizes(const std::string& raw, std::vector<std::size_t>& out) {
    std::vector<std::size_t> parsed;
    std::stringstream ss(raw);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) {
            return false;
        }
        try {
            parsed.push_back(static_cast<std::size_t>(std::stoull(token)));
        } catch (...) {
            return false;
        }
    }
    if (parsed.empty()) {
        return false;
    }
    out = parsed;
    return true;
}

static bool parse_int(const std::string& raw, int& out) {
    try {
        out = std::stoi(raw);
        return true;
    } catch (...) {
        return false;
    }
}

static bool parse_u32(const std::string& raw, std::uint32_t& out) {
    try {
        out = static_cast<std::uint32_t>(std::stoul(raw));
        return true;
    } catch (...) {
        return false;
    }
}

static bool parse_size_t(const std::string& raw, std::size_t& out) {
    try {
        out = static_cast<std::size_t>(std::stoull(raw));
        return true;
    } catch (...) {
        return false;
    }
}

static bool parse_double(const std::string& raw, double& out) {
    try {
        out = std::stod(raw);
        return true;
    } catch (...) {
        return false;
    }
}

static bool parse_algo(const std::string& raw, AlgoMode& out) {
    if (raw == "all") {
        out = AlgoMode::All;
        return true;
    }
    if (raw == "rs") {
        out = AlgoMode::RandomSplitting;
        return true;
    }
    if (raw == "std") {
        out = AlgoMode::StdSort;
        return true;
    }
    if (raw == "dual") {
        out = AlgoMode::DualPivot;
        return true;
    }
    return false;
}

static void print_help() {
    std::cout
        << "Benchmark app (RandomSplitting vs std::sort vs DualPivot)\n"
        << "Flags:\n"
        << "  --sizes=comma_list            Example: --sizes=10000,100000,1000000\n"
        << "  --seed=uint32                 RNG seed\n"
        << "  --correctness-cases=int       Random correctness test count\n"
        << "  --trials-small=int            Trials for n<=1,000\n"
        << "  --trials-medium=int           Trials for n<=10,000\n"
        << "  --trials-large=int            Trials for n<=100,000\n"
        << "  --trials-huge=int             Trials for larger n\n"
        << "  --value-low=int               Min random value\n"
        << "  --value-high=int              Max random value\n"
        << "  --small-threshold=size_t      Base-case threshold for RandomSplitting\n"
        << "  --sample-size=size_t          Pivot sample size for RandomSplitting\n"
        << "  --max-elements=size_t         Safety cap on n\n"
        << "  --max-ram-gib=double          Safety cap on estimated RAM\n"
        << "  --max-single-run-ms=int       Safety cap on one trial runtime\n"
        << "  --algo=all|rs|std|dual        Select algorithm mode (default: all)\n"
        << "  --help                        Show this message\n";
}

static bool parse_args(int argc, char** argv, AppConfig& cfg) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        auto has_prefix = [&](const char* prefix) { return arg.rfind(prefix, 0) == 0; };

        if (has_prefix("--sizes=")) {
            if (!parse_sizes(arg.substr(8), cfg.sizes)) return false;
        } else if (has_prefix("--seed=")) {
            if (!parse_u32(arg.substr(7), cfg.seed)) return false;
        } else if (has_prefix("--correctness-cases=")) {
            if (!parse_int(arg.substr(20), cfg.correctness_cases)) return false;
        } else if (has_prefix("--trials-small=")) {
            if (!parse_int(arg.substr(15), cfg.trials_small)) return false;
        } else if (has_prefix("--trials-medium=")) {
            if (!parse_int(arg.substr(16), cfg.trials_medium)) return false;
        } else if (has_prefix("--trials-large=")) {
            if (!parse_int(arg.substr(15), cfg.trials_large)) return false;
        } else if (has_prefix("--trials-huge=")) {
            if (!parse_int(arg.substr(14), cfg.trials_huge)) return false;
        } else if (has_prefix("--value-low=")) {
            if (!parse_int(arg.substr(12), cfg.value_low)) return false;
        } else if (has_prefix("--value-high=")) {
            if (!parse_int(arg.substr(13), cfg.value_high)) return false;
        } else if (has_prefix("--small-threshold=")) {
            if (!parse_size_t(arg.substr(18), cfg.rs_cfg.small_threshold)) return false;
        } else if (has_prefix("--sample-size=")) {
            if (!parse_size_t(arg.substr(14), cfg.rs_cfg.sample_size)) return false;
        } else if (has_prefix("--max-elements=")) {
            if (!parse_size_t(arg.substr(15), cfg.max_safe_elements)) return false;
        } else if (has_prefix("--max-ram-gib=")) {
            if (!parse_double(arg.substr(14), cfg.max_safe_ram_gib)) return false;
        } else if (has_prefix("--max-single-run-ms=")) {
            int tmp_ms = 0;
            if (!parse_int(arg.substr(20), tmp_ms)) return false;
            cfg.max_safe_single_run_ms = tmp_ms;
        } else if (has_prefix("--algo=")) {
            if (!parse_algo(arg.substr(7), cfg.algo)) return false;
        } else {
            return false;
        }
    }

    if (cfg.value_low > cfg.value_high) return false;
    if (cfg.correctness_cases < 0 || cfg.trials_small <= 0 || cfg.trials_medium <= 0 || cfg.trials_large <= 0 || cfg.trials_huge <= 0) return false;
    if (cfg.max_safe_ram_gib <= 0.0 || cfg.max_safe_single_run_ms <= 0) return false;

    return true;
}

static int trials_for_size(const AppConfig& cfg, std::size_t n) {
    if (n <= 1000) return cfg.trials_small;
    if (n <= 10'000) return cfg.trials_medium;
    if (n <= 100'000) return cfg.trials_large;
    return cfg.trials_huge;
}

static bool run_correctness_tests(rssort::RandomSplittingSorter& rs, dqsort::DualPivotQuickSorter& dual, const AppConfig& cfg) {
    std::mt19937 rng(cfg.seed + 17u);
    std::uniform_int_distribution<int> size_dist(0, 10000);
    std::uniform_int_distribution<int> value_dist(cfg.value_low, cfg.value_high);

    for (int c = 1; c <= cfg.correctness_cases; ++c) {
        const int n = size_dist(rng);
        std::vector<int> input;
        input.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            input.push_back(value_dist(rng));
        }

        std::vector<int> expected = input;
        std::sort(expected.begin(), expected.end());

        std::vector<int> a = input;
        rs.sort(a);
        if (a != expected) {
            std::cout << "Correctness: FAILED RandomSplitting on case " << c << " (n=" << n << ")\n";
            return false;
        }

        std::vector<int> b = input;
        dual.sort(b);
        if (b != expected) {
            std::cout << "Correctness: FAILED DualPivot on case " << c << " (n=" << n << ")\n";
            return false;
        }
    }

    std::cout << "Correctness: PASSED (" << cfg.correctness_cases << " random cases)\n";
    return true;
}

static bool benchmark_algo(
    AlgoMode algo,
    rssort::RandomSplittingSorter& rs,
    dqsort::DualPivotQuickSorter& dual,
    const std::vector<int>& source,
    int trials,
    long long max_safe_single_run_ns,
    TimingStats& out,
    const std::vector<int>& expected) {

    long long total_ns = 0;
    long long min_ns = std::numeric_limits<long long>::max();
    long long max_ns = 0;

    for (int t = 0; t < trials; ++t) {
        std::vector<int> data = source;
        const auto start = std::chrono::high_resolution_clock::now();

        if (algo == AlgoMode::RandomSplitting) {
            rs.sort(data);
        } else if (algo == AlgoMode::DualPivot) {
            dual.sort(data);
        } else {
            std::sort(data.begin(), data.end());
        }

        const auto end = std::chrono::high_resolution_clock::now();
        const long long elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        if (elapsed_ns > max_safe_single_run_ns) {
            std::cout << "Safety exit: single run took " << elapsed_ns
                      << " ns, above safe limit " << max_safe_single_run_ns << " ns\n";
            return false;
        }

        if (data != expected) {
            std::cout << "Benchmark correctness mismatch on trial " << (t + 1) << '\n';
            return false;
        }

        total_ns += elapsed_ns;
        min_ns = std::min(min_ns, elapsed_ns);
        max_ns = std::max(max_ns, elapsed_ns);
    }

    out = TimingStats{
        static_cast<double>(total_ns) / trials / 1'000'000.0,
        static_cast<double>(min_ns) / 1'000'000.0,
        static_cast<double>(max_ns) / 1'000'000.0};

    return true;
}

static std::size_t estimate_dualpivot_extra_bytes(std::size_t n) {
    if (n <= 1) {
        return 0;
    }
    std::size_t depth = 0;
    for (std::size_t m = n; m > 1; m >>= 1) {
        ++depth;
    }
    const std::size_t frame_bytes = sizeof(int) * 6 + sizeof(void*);
    return frame_bytes * (depth + 4);
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        print_help();
        return 0;
    }

    AppConfig cfg;
    if (!parse_args(argc, argv, cfg)) {
        if (argc > 1) {
            std::cerr << "Invalid arguments. Use --help for usage.\n";
            return 2;
        }
    }

    const long long max_safe_single_run_ns = cfg.max_safe_single_run_ms * 1'000'000LL;
    const std::size_t max_safe_ram_bytes = static_cast<std::size_t>(cfg.max_safe_ram_gib * 1024.0 * 1024.0 * 1024.0);

    rssort::RandomSplittingSorter rs(cfg.seed, cfg.rs_cfg);
    dqsort::DualPivotQuickSorter dual(cfg.seed + 1u);

    if (!run_correctness_tests(rs, dual, cfg)) {
        return 1;
    }

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\nBenchmark\n";

    for (std::size_t n : cfg.sizes) {
        const int trials = trials_for_size(cfg, n);
        const auto source = make_random_vector(n, cfg.value_low, cfg.value_high, cfg.seed + static_cast<std::uint32_t>(n));
        const auto rs_mem = rssort::estimate_sort_ram_usage(n);
        const auto dual_extra = estimate_dualpivot_extra_bytes(n);

        if (n > cfg.max_safe_elements) {
            std::cout << "Safety exit: input size " << n << " exceeds safe limit " << cfg.max_safe_elements << '\n';
            return 2;
        }

        if (rs_mem.estimated_peak_bytes > max_safe_ram_bytes) {
            std::cout << "Safety exit: RandomSplitting estimated RAM " << rs_mem.estimated_peak_bytes
                      << " bytes (" << rssort::bytes_to_gib(rs_mem.estimated_peak_bytes) << " GiB)"
                      << " exceeds safe limit " << max_safe_ram_bytes << " bytes\n";
            return 2;
        }

        std::vector<int> expected = source;
        std::sort(expected.begin(), expected.end());

        std::cout << "\nN = " << n << ", Trials = " << trials << '\n';
        std::cout << "Estimated RAM peak (RandomSplitting): " << rssort::bytes_to_gib(rs_mem.estimated_peak_bytes) << " GiB\n";
        std::cout << "Estimated extra RAM (DualPivot stack): " << rssort::bytes_to_gib(dual_extra) << " GiB\n";

        TimingStats rs_stats{};
        TimingStats std_stats{};
        TimingStats dual_stats{};

        if (cfg.algo == AlgoMode::All || cfg.algo == AlgoMode::RandomSplitting) {
            if (!benchmark_algo(AlgoMode::RandomSplitting, rs, dual, source, trials, max_safe_single_run_ns, rs_stats, expected)) {
                return 2;
            }
            std::cout << "RandomSplitting avg/min/max: " << rs_stats.avg_ms << " / " << rs_stats.min_ms << " / " << rs_stats.max_ms << " ms\n";
        }

        if (cfg.algo == AlgoMode::All || cfg.algo == AlgoMode::StdSort) {
            if (!benchmark_algo(AlgoMode::StdSort, rs, dual, source, trials, max_safe_single_run_ns, std_stats, expected)) {
                return 2;
            }
            std::cout << "std::sort       avg/min/max: " << std_stats.avg_ms << " / " << std_stats.min_ms << " / " << std_stats.max_ms << " ms\n";
        }

        if (cfg.algo == AlgoMode::All || cfg.algo == AlgoMode::DualPivot) {
            if (!benchmark_algo(AlgoMode::DualPivot, rs, dual, source, trials, max_safe_single_run_ns, dual_stats, expected)) {
                return 2;
            }
            std::cout << "DualPivot       avg/min/max: " << dual_stats.avg_ms << " / " << dual_stats.min_ms << " / " << dual_stats.max_ms << " ms\n";
        }

        if (cfg.algo == AlgoMode::All && std_stats.avg_ms > 0.0) {
            std::cout << "Ratio (RandomSplitting/std::sort): " << (rs_stats.avg_ms / std_stats.avg_ms) << "x\n";
            std::cout << "Ratio (DualPivot/std::sort): " << (dual_stats.avg_ms / std_stats.avg_ms) << "x\n";
            std::cout << "Ratio (RandomSplitting/DualPivot): " << (rs_stats.avg_ms / dual_stats.avg_ms) << "x\n";
        }
    }

    return 0;
}