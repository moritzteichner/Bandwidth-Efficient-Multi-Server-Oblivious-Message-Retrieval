#include <cstdlib>
#include <cstring>
#include <climits>

#include "run.cpp"

void printUsage(const char* program) {
    std::cerr << "Usage: " << program << " -N <messages> -M <log users> -K <pertinent> [-R]\n"
              << "       " << program << " -B [-R]\n\n"
              << "  -N  Number of messages on the bulletin board (> 0)\n"
              << "  -M  Log2 of the number of users (5 to 30)\n"
              << "  -K  Number of messages pertinent to the retrieving user (1 to N)\n"
              << "  -R  Run full retrieval (OMR) instead of detection (OMD)\n"
              << "  -B  Run the benchmark suite defined in run.cpp and append to benchmark.csv\n"
              << "  -h  Show this help\n";
}

// Parses a strictly positive integer. Returns false on anything else.
bool parsePositiveInt(const char* s, int& out) {
    char* end = nullptr;
    long value = std::strtol(s, &end, 10);
    if (end == s || *end != '\0' || value <= 0 || value > INT_MAX) {
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

int main(int argc, char* argv[]) {
    int N = 0, M = 0, K = 0;
    bool benchmark = false;
    bool retrieval = false;

    for (int i = 1; i < argc; i++) {
        int* target = nullptr;
        if (strcmp(argv[i], "-N") == 0) {
            target = &N;
        } else if (strcmp(argv[i], "-M") == 0) {
            target = &M;
        } else if (strcmp(argv[i], "-K") == 0) {
            target = &K;
        } else if (strcmp(argv[i], "-B") == 0) {
            benchmark = true;
            continue;
        } else if (strcmp(argv[i], "-R") == 0) {
            retrieval = true;
            continue;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Invalid argument: " << argv[i] << "\n\n";
            printUsage(argv[0]);
            return 1;
        }

        if (i + 1 >= argc || !parsePositiveInt(argv[i + 1], *target)) {
            std::cerr << argv[i] << " requires a positive integer.\n\n";
            printUsage(argv[0]);
            return 1;
        }
        i++;
    }

    if (benchmark) {
        return run_benchmarks(retrieval);
    }

    if (N == 0 || M == 0 || K == 0) {
        std::cerr << "-N, -M and -K are required.\n\n";
        printUsage(argv[0]);
        return 1;
    }
    // The simulated retrieving user has ID 27, so the ID space must hold at least 32 users.
    // User IDs are stored as int, which bounds M from above.
    if (M < 5 || M > 30) {
        std::cerr << "-M must be between 5 and 30.\n";
        return 1;
    }
    if (K > N) {
        std::cerr << "-K cannot exceed -N.\n";
        return 1;
    }
    return run(retrieval, N, M, K);
}
