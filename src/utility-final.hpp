#include <gmpxx.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <utility>
#include <random>

// Function to check primality
bool isPrime(const mpz_class& n) {
    return mpz_probab_prime_p(n.get_mpz_t(), 25) > 0; // 25 iterations for a strong check
}

std::vector<mpz_class> primeFactorization(const mpz_class &n) {
    mpz_class num = n;  // Copy the number since we will modify it
    std::vector<mpz_class> primeFactors;  // To store the prime factors

    // Check for factor of 2 first
    while (num % 2 == 0) {
        primeFactors.push_back(2);
        num /= 2;
    }

    // Check for odd factors starting from 3
    for (mpz_class i = 3; i * i <= num; i += 2) {
        while (num % i == 0) {
            primeFactors.push_back(i);
            num /= i;
        }
    }

    // If num is still greater than 1, then it's a prime factor
    if (num > 1) {
        primeFactors.push_back(num);
    }

    return primeFactors;
}

// Find the next prime q >= m using mpz_nextprime
std::pair<mpz_class, std::vector<mpz_class>>  nextPrimeAndFactors(mpz_class m) {
    mpz_class q;
    mpz_nextprime(q.get_mpz_t(), m.get_mpz_t());

    std::vector<mpz_class> factors = primeFactorization(q - 1);
    return std::make_pair(q, factors);
}

// Generate primes of an exact bit length
std::vector<mpz_class> generateExactBitPrimes(int bitLength, int count = 500) {
    mpz_class start = mpz_class(1) << (bitLength - 1); // 2^(bitLength-1)
    std::vector<mpz_class> primes;
    mpz_class num = start;
    while (primes.size() < count) {
        if (isPrime(num)) {
            primes.push_back(num);
        }
        mpz_nextprime(num.get_mpz_t(), num.get_mpz_t());
    }
    return primes;
}

std::pair<mpz_class, std::vector<mpz_class>> findPrimeAndFactors(mpz_class m, int targetBitSize) {
    const mpz_class initial_m = m;
    int currentBits = mpz_sizeinbase(m.get_mpz_t(), 2);

    // Define sets of exact bit primes to use
    std::vector<int> bitSizes = {2001, 2000, 2}; // Add more ranges for flexibility
    std::vector<std::vector<mpz_class>> primePools;

    // Generate primes for all specified bit sizes
    for (int bitSize : bitSizes) {
        primePools.push_back(generateExactBitPrimes(bitSize, 500)); // Generate larger pools
    }
    std::vector<mpz_class> usedPrimes;
    bool foundPrime = false;
    mpz_class q;

    std::random_device rd; 
    std::mt19937 gen(rd()); 

    while (!foundPrime) {
        // Reinitialize m to ensure bit length grows precisely
        m = initial_m;
        currentBits = mpz_sizeinbase(m.get_mpz_t(), 2);
        usedPrimes.clear();

        // Shuffle each prime pool for randomness
        for (auto& pool : primePools) {
            std::shuffle(pool.begin(), pool.end(), gen);
        }

        // Dynamically pick primes without exceeding the target bit size
        for (size_t i = 0; i < primePools.size(); ++i) {
            for (auto& prime : primePools[i]) {
                // Check if adding this prime exceeds the target bit size
                mpz_class temp = m * prime;
                int tempBits = mpz_sizeinbase(temp.get_mpz_t(), 2);

                if (tempBits > targetBitSize) {
                    continue; // Skip if adding this prime exceeds the target bit size
                }

                // Otherwise, include this prime and continue
                m = temp;
                currentBits = tempBits;
                usedPrimes.push_back(prime);

                if (currentBits == targetBitSize) {
                    break;
                }
            }

            if (currentBits == targetBitSize) {
                break;
            }
        }

        // Ensure m has exactly the target bit size
        if (currentBits == targetBitSize) {
            q = m + 1;

            // Check if q is prime
            if (isPrime(q)) {
                foundPrime = true;
            }
        }
    }

    // Combine the initial factors and used primes for final output
    std::vector<mpz_class> factors = primeFactorization(initial_m);
    factors.insert(factors.end(), usedPrimes.begin(), usedPrimes.end());

    return std::make_pair(q, factors);
}