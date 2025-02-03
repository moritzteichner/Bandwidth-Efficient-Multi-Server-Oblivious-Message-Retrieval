#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <random>

#include <algorithm>
#include <gmpxx.h>
#include <string>

#include "newTypes.hpp"
#include "packing_utility.hpp"
#include "prime_utility.hpp"
#include "mod_utility.hpp"

#include "ophelib/paillier_fast.h"
#include "ophelib/packing.h"
#include "ophelib/util.h"
#include "ophelib/ml.h"

using namespace std;
using namespace ophelib;

#include "simulation_utility.hpp"
#include "benchmkaring_class.cpp"

#include <NTL/vec_ZZ.h>
using namespace NTL;

#include "omd.cpp"
#include "omr.cpp"

bool areVectorsEqual(std::vector<int> v1, std::vector<mpz_class> v2) {
    std::vector<mpz_class> v1_mpz(v1.begin(), v1.end());

    std::sort(v1_mpz.begin(), v1_mpz.end());
    std::sort(v2.begin(), v2.end());

    return v1_mpz == v2;
}

int run(bool retrieval, int N, int M, int K) {
    //Standard Parameters. Change if needed.
    int RETRIEVING_USER = 27;
    double eps = pow(2.0, -40);
    int paillierPrimeSize = 2048;
    //Generate sample receievers for messages and remeber pertinent ones.
    std::vector<int> recievers = generateClients(K, N, M, RETRIEVING_USER);
    std::vector<int> pertinent_indices;

    //Remember the real pertinent indices to check the output of Decode.
    for (int i = 0; i < recievers.size(); i++) {
        if (recievers[i] == RETRIEVING_USER) {
            pertinent_indices.push_back(i);
        }
    }
    Benchmarking result;
    if (retrieval) {
        result = OMR::omr_run(N, M, paillierPrimeSize, K, recievers, RETRIEVING_USER);
    } else {
        result = OMD::omd_run(N, M, paillierPrimeSize, K, recievers, RETRIEVING_USER, eps);
    }

    if (areVectorsEqual(pertinent_indices, result.getOutput())) {
        std::cout << "Result is correct." << std::endl;
    }
    return 0;
}

int run_benchmarks(bool retrieval) {
    // Define possible values for parameters
    std::vector<int> numMessages = {(int)pow(2, 12)};
    std::vector<int> logNumUser = {22};
    std::vector<int> paillierPrimeSize = {2048};
    std::vector<int> kValues = {2};
    int RETRIEVING_USER = 27;
    double eps = pow(2.0, -40);

    // Open CSV file for appending results
    const char* path = "benchmark.csv";
    bool fileExists = std::filesystem::exists(path);
    std::ofstream csvFile(path, std::ios::app);

    if (!csvFile.is_open()) {
        std::cerr << "Error opening CSV file!" << std::endl;
        return 1;
    }

    // Write CSV header only if the file is newly created
    if (!fileExists) {
        csvFile << "NUM_Messages,LOG_NUM_USER,PAILLIER_PRIME_SIZE,k,Sender_Runtime,Server_Runtime,"
                   "Recipient_Runtime,Digest_Size,Public_Key_Size\n";
        csvFile.flush(); // Ensure the header is written immediately
    }

    // Run the benchmark for each combination of parameters
    for (int numMsg : numMessages) {
        for (int logUser : logNumUser) {
            for (int paillierSize : paillierPrimeSize) {
                for (int k : kValues) {

                    //Generate sample receievers for messages and remeber pertinent ones.
                    std::vector<int> recievers = generateClients(k, numMsg, logUser, RETRIEVING_USER);
                    std::vector<int> pertinent_indices;

                    //Remember the real pertinent indices to check the output of Decode.
                    for (int i = 0; i < recievers.size(); i++) {
                        if (recievers[i] == RETRIEVING_USER) {
                            pertinent_indices.push_back(i);
                        }
                    }

                    Benchmarking result = OMD::omd_run(numMsg, logUser, paillierSize, k, recievers, RETRIEVING_USER, eps);

                    if (areVectorsEqual(pertinent_indices, result.getOutput())) {
                        std::cout << "Result is correct." << std::endl;
                    }

                    result.saveToCSV(csvFile, numMsg, logUser, paillierSize, k);
                    csvFile.flush();
                }
            }
        }
    }

    // Close the CSV file
    csvFile.close();
    return 0;
}