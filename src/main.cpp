#include <cstdlib>
#include <cstring>

#include "run.cpp"

int main(int argc, char* argv[]) {
    int N = 0, M = 0, K = 0;
    bool benchmark = false;
    bool retrieval = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-N") == 0 && i + 1 < argc) {
            N = std::atoi(argv[i + 1]);
            i++; 
        } else if (strcmp(argv[i], "-M") == 0 && i + 1 < argc) {
            M = std::atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-K") == 0 && i + 1 < argc) {
            K = std::atoi(argv[i + 1]);
            i++;
        }else if (strcmp(argv[i], "-B") == 0) {
            benchmark = true;
        }else if (strcmp(argv[i], "-R") == 0) {
            retrieval = true;
        } else {
            std::cerr << "Invalid argument: " << argv[i] << std::endl;
            return 1;
        }
    }
    if (benchmark) {
        return run_benchmarks(retrieval);
    } else {
        return run(retrieval, N, M, K);
    }
}
