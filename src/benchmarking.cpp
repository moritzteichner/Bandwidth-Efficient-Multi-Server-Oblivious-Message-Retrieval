#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <filesystem>

#include "omd.cpp"
//#include "omr.cpp"

int run_benchmarks() {
    // Define possible values for parameters
    std::vector<int> numMessages = {(int)pow(2, 16)};
    std::vector<int> logNumUser = {22};
    std::vector<int> paillierPrimeSize = {1024, 1536, 2048};
    std::vector<int> kValues = {50};

    // Open CSV file for appending results
    const char* path = "benchmark_omr_prime.csv";
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
                    // Run the benchmark for current combination
                    Benchmarking result = omd_run(numMsg, logUser, paillierSize, k);

                    // Append results to the CSV file
                    result.saveToCSV(csvFile, numMsg, logUser, paillierSize, k);

                    // Flush the file after every write
                    csvFile.flush();

                    // Optionally, print the results for each combination
                    result.display();
                }
            }
        }
    }

    // Close the CSV file
    csvFile.close();
    return 0;
}