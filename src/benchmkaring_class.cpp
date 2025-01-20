#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>

// Benchmarking class to store the results
class Benchmarking {
private:
    double sendRuntime;
    double precomputeRuntime;
    double retrievalRuntime;
    double decodeRuntime;
    size_t digestSize;
    size_t publicKeySize;

public:
    // Constructor to initialize values
    Benchmarking(double send = 0.0, double precomputation = 0.0, double retrieval = 0.0, double decode = 0.0,
                 size_t digest = 0, size_t publicKey = 0)
        : sendRuntime(send), precomputeRuntime(precomputation), retrievalRuntime(retrieval), decodeRuntime(decode),
          digestSize(digest), publicKeySize(publicKey) {}

    // Getter methods
    double getSendRuntime() const { return sendRuntime; }
    double getPrecomptationRuntime() const { return precomputeRuntime; }
    double getRetrievalRuntime() const { return retrievalRuntime; }
    double getDecodeRuntime() const { return decodeRuntime; }
    size_t getDigestSize() const { return digestSize; }
    size_t getPublicKeySize() const { return publicKeySize; }

    // Display the benchmarking results
    void display() const {
        std::cout << "Sender Runtime: " << sendRuntime << " seconds\n";
        std::cout << "Server Runtime: " << precomputeRuntime << " seconds\n";
        std::cout << "Server Runtime: " << retrievalRuntime << " seconds\n";
        std::cout << "Recipient Runtime: " << decodeRuntime << " seconds\n";
        std::cout << "Digest Size: " << digestSize << " bytes\n";
        std::cout << "Public Key Size: " << publicKeySize << " bytes\n";
    }

    // Method to save the benchmarking results to CSV
    void saveToCSV(std::ofstream &file, int numMessages, int logNumUser, int paillierPrimeSize, int k) {
        file << numMessages << "," << logNumUser << "," << paillierPrimeSize << "," << k << ","
             << sendRuntime << "," << precomputeRuntime << "," << retrievalRuntime << "," << decodeRuntime << ","
             << digestSize << "," << publicKeySize << "\n";
    }
};