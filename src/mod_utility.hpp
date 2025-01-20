#include <iostream>
#include <cmath>
#include <vector>
#include <gmpxx.h>

// Function to compute the greatest common divisor (gcd)
int gcd(int a, int b) {
    if (b == 0) {
        return a;
    }
    return gcd(b, a % b);
}

mpz_class powerMod(const mpz_class& a, const mpz_class& b, const mpz_class& q) {
    mpz_class result = 1;
    mpz_class base = a % q;  // Handle cases where base >= q
    mpz_class exponent = b;

    if (base == 0) return 0;  // If base is divisible by q, result is 0 for any exponent

    while (exponent > 0) {
        // If exponent is odd, multiply the current result by base
        if (exponent % 2 == 1)
            result = (result * base) % q;

        // Now exponent must be even, so square the base
        base = (base * base) % q;
        exponent = exponent / 2;
    }
    return result;
}

// Find a generator of the multiplicative group mod q
mpz_class findGenerator(const mpz_class &q, const std::vector<mpz_class> &factors) {
    mpz_class phi = q - 1; // Euler's totient for a prime q

    for (mpz_class g = 2; g < q; ++g) {
        bool isGenerator = true;
        for (const auto &factor : factors) {
            if (powerMod(g, phi / factor, q) == 1) {
                isGenerator = false;
                break;
            }
        }
        if (isGenerator) return g; // Found a generator
    }
    return -1; // No generator found (shouldn't happen for prime q)
}

// Find a primitive nth root of unity mod q
mpz_class findPrimitiveRootOfUnity(const mpz_class &n, const mpz_class &q, std::vector<mpz_class> &factors) {
    if ((q - 1) % n != 0) {
        std::cerr << "n does not divide q - 1, so no primitive nth root of unity exists.\n";
        return -1;
    }

    mpz_class generator = findGenerator(q, factors);
    if (generator == -1) {
        std::cerr << "Failed to find a generator for q.\n";
        return -1;
    }

    mpz_class omega = powerMod(generator, (q - 1) / n, q); // Candidate nth root of unity

    // Verify if omega is a primitive nth root of unity
    for (mpz_class k = 1; k < n; ++k) {
        if (powerMod(omega, k, q) == 1) {
            std::cerr << "Failed verification: omega is not a primitive nth root of unity.\n";
            return -1;
        }
    }
    return omega;
}

// Modular inverse function using GMP
mpz_class modInverse(const mpz_class& a, const mpz_class& q) {
    mpz_class inv;
    if (mpz_invert(inv.get_mpz_t(), a.get_mpz_t(), q.get_mpz_t()) == 0) {
        throw std::runtime_error("No modular inverse exists (element is not invertible mod q)");
    }
    return inv;
}

// Function to solve the modular linear system A * x = b mod q
std::vector<mpz_class> solveModularLinearSystem(std::vector<mpz_class> d, int s,
                                                const mpz_class& q) {
                                                    
    // Define matrix A (Toeplitz matrix) and vector b modulo q
    std::vector<std::vector<mpz_class>> A(s, std::vector<mpz_class>(s));
    std::vector<mpz_class> b(s);

    // Populate vector b with d[s] to d[2s-1]
    for (int i = 0; i < s; ++i) {
        //b[i] = d[s + i];
        b[i] = d[s + i];
    }

    // Populate matrix A with the required Toeplitz structure
    for (int row = 0; row < s; ++row) {
        for (int col = 0; col < s; ++col) {
            //A[row][col] = d[row + col];
            A[row][col] = d[row + col];
        }
    };

    size_t n = A.size();
    std::vector<std::vector<mpz_class>> AugmentedMatrix = A;
    std::vector<mpz_class> x = b;

    // Augment the matrix A with the vector b to form an augmented matrix [A | b]
    for (size_t i = 0; i < n; ++i) {
        AugmentedMatrix[i].push_back(x[i]);
    }

    // Perform Gaussian Elimination
    for (size_t i = 0; i < n; ++i) {
        // Pivot: Make sure A[i][i] is non-zero
        if (AugmentedMatrix[i][i] == 0) {
            bool swapped = false;
            for (size_t j = i + 1; j < n; ++j) {
                if (AugmentedMatrix[j][i] != 0) {
                    std::swap(AugmentedMatrix[i], AugmentedMatrix[j]);
                    swapped = true;
                    break;
                }
            }
            if (!swapped) {
                throw std::runtime_error("Matrix is singular (no solution or infinitely many solutions)");
            }
        }

        // Normalize row i (make A[i][i] = 1)
        mpz_class inv = modInverse(AugmentedMatrix[i][i], q);
        for (size_t j = 0; j < n; ++j) {
            AugmentedMatrix[i][j] = AugmentedMatrix[i][j] * inv % q;
        }
        AugmentedMatrix[i][n] = AugmentedMatrix[i][n] * inv % q;

        // Eliminate all entries below A[i][i]
        for (size_t j = i + 1; j < n; ++j) {
            if (AugmentedMatrix[j][i] != 0) {
                mpz_class factor = AugmentedMatrix[j][i];
                for (size_t k = 0; k < n + 1; ++k) {
                    AugmentedMatrix[j][k] = (AugmentedMatrix[j][k] - factor * AugmentedMatrix[i][k]) % q;
                    if (AugmentedMatrix[j][k] < 0) {
                        AugmentedMatrix[j][k] += q;
                    }
                }
            }
        }
    }

    // Back substitution to solve for x
    std::vector<mpz_class> result(n);
    for (size_t i = n; i-- > 0;) {
        result[i] = AugmentedMatrix[i][n];
        for (size_t j = i + 1; j < n; ++j) {
            result[i] = (result[i] - AugmentedMatrix[i][j] * result[j]) % q;
            if (result[i] < 0) {
                result[i] += q;
            }
        }
    }

    return result;
}

mpz_class ceil_log(const mpz_class& n, int base) {
    if (n <= 0) {
        throw std::invalid_argument("Logarithm undefined for non-positive values.");
    }

    if (base <= 1) {
        throw std::invalid_argument("Logarithm base must be greater than 1.");
    }

    // Get the bit length of the number
    size_t bit_count = mpz_sizeinbase(n.get_mpz_t(), 2);

    // Approximate the base-2 logarithm
    double log2_n = static_cast<double>(bit_count - 1) +
                    std::log2(mpz_get_d(n.get_mpz_t()) / std::pow(2.0, bit_count - 1));

    // Convert to the desired base
    double log_b_n = log2_n / std::log2(base);

    // Round up and return as an integer
    return static_cast<mpz_class>(std::ceil(log_b_n));
}

mpz_class power(const mpz_class& base, const mpz_class& exponent) {
    mpz_class result = 1;
    for (mpz_class i = 0; i < exponent; ++i) {
        result *= base;
    }
    return result;
}

mpz_class find_next_prime(const mpz_class& N) {
    mpz_class next_prime;
    mpz_nextprime(next_prime.get_mpz_t(), N.get_mpz_t());
    return next_prime;
}

// Function to compute prime factors of a given number q
std::vector<mpz_class> prime_factors(const mpz_class& q) {
    std::vector<mpz_class> factors;
    mpz_class n = q; // Copy of q to factorize
    mpz_class factor = 2; // Start with the smallest prime
    
    while (mpz_cmp_ui(n.get_mpz_t(), 1) > 0) { // While n > 1
        if (mpz_divisible_p(n.get_mpz_t(), factor.get_mpz_t())) {
            factors.push_back(factor); // Add the factor
            mpz_divexact(n.get_mpz_t(), n.get_mpz_t(), factor.get_mpz_t()); // Divide n by factor
        } else {
            mpz_nextprime(factor.get_mpz_t(), factor.get_mpz_t()); // Move to the next prime
        }
    }

    return factors;
}