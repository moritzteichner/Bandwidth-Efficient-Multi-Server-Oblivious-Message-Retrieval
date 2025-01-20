#include <gsl/gsl_sf_lambert.h>
#include <gsl/gsl_errno.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <gmpxx.h>

#include "ophelib/paillier_fast.h"
#include "ophelib/packing.h"
#include "ophelib/util.h"
#include "ophelib/ml.h"

using namespace std;
using namespace ophelib;

double binomialCoefficient(int n, int k) {
    if (k > n) return 0;
    if (k == 0 || k == n) return 1;
    double result = 1;
    for (int i = 1; i <= k; ++i) {
        result *= (n - i + 1);
        result /= i;
    }
    return result;
}

//n balls, beta bins. 
double computeProb(int n, int k, double beta) {
    double sum = 0.0;
    double invBeta = 1.0 / beta;
    for (int i = 0; i < k; ++i) {
        double term = binomialCoefficient(n, i) * pow(invBeta, i) * pow(1 - invBeta, n - i);
        sum += term;
    }
    return 1.0 - pow(sum, beta);
}

int findSmallestK(int n, double beta, double epsilon) {
    int k = 0;
    while (true) {
        double p = computeProb(n, k, beta);
        if (p < epsilon) {
            return k;
        }
        ++k;
    }
}

double computeLambertW(double x) {
    gsl_set_error_handler_off(); // Disable GSL's default error handler

    const double min_x = -1.0 / std::exp(1); // Minimum valid input for Lambert W functions
    gsl_sf_result result;
    int status;

    if (x >= min_x && x >= 0) {
        // Use the principal branch W0 for x >= 0
        status = gsl_sf_lambert_W0_e(x, &result);
    } else if (x >= min_x && x < 0) {
        // Use the secondary branch W-1 for -1/e <= x < 0
        status = gsl_sf_lambert_Wm1_e(x, &result);
    } else {
        // Input is outside the domain of both branches
        throw std::domain_error("Input to Lambert W is out of domain. x must be >= -1/e.");
    }

    if (status != GSL_SUCCESS) {
        throw std::runtime_error("GSL Lambert W computation failed.");
    }

    return result.val;
}

// Function to compute -t * ln(2) / W(-t * ln(2) / (k * N)), rounded down to the nearest integer
int estimatePackingFactor(int t, int k, int N) {
    if (t <= 0 || k <= 0 || N <= 0) {
        throw std::invalid_argument("Parameters t, k, and N must be positive integers.");
    }

    double numerator = -static_cast<double>(t) * std::log(2);
    double denominator = static_cast<double>(k * N);
    double argument = numerator / denominator;

    if (argument >= 0) {
        throw std::domain_error("The argument to Lambert W must be negative for this computation.");
    }
    //std::cout << "Argument: " << argument << std::endl;
    double W_value = computeLambertW(argument); // Lambert W function (principal branch)
    double result = numerator / W_value;

    return static_cast<int>(std::floor(result)); // Round down to the nearest integer
}

// Function to pack ciphertexts into fewer ciphertexts
vector<Ciphertext>  pack_ciphertexts(
    std::vector<Ciphertext> ciphertexts,
    PaillierFast pk,
    int NUM_Ciphertexts,
    int plaintext_size, 
    int n_packed
) {
    // Calculate the exponent increment x for each slot
    unsigned int x = static_cast<int>(std::ceil(static_cast<double>(plaintext_size) / n_packed)) -1;
    //std::cout << "x: " << x << std::endl;

    //mpz_class power_of_two;
    //mpz_ui_pow_ui(power_of_two.get_mpz_t(), 2, x);
    //mpz_mul_2exp(power_of_two.get_mpz_t(), power_of_two.get_mpz_t(), exponent);
    //std::cout << "Power: " << power_of_two << std::endl;
    //std::string value_str = power_of_two.get_str();
    //BigNumber power(value_str.c_str());
    Integer power = 2;
    power = power.pow(x);
    Integer current_power = 1;

    vector<Ciphertext> packed_ciphertexts;
    Ciphertext current_ciphertext = pk.encrypt(0);
    Ciphertext mult;
    for (size_t i = 0; i < NUM_Ciphertexts; i += n_packed) {
        for (int j = 0; j < n_packed && (i + j) < NUM_Ciphertexts; ++j) {
            mult = ciphertexts[i + j] * current_power;
            current_ciphertext += mult;
            current_power *= power;
        }
        packed_ciphertexts.push_back(current_ciphertext);
        current_ciphertext = pk.encrypt(0);
        current_power = 1;
        cout << "One Packed done." << endl;

    }
    
    return packed_ciphertexts;
}


std::vector<std::vector<mpz_class>> unpack(
    Vec<Integer> packed_plaintexts,
    int plaintext_size, 
    int n_packed
) {
    // Calculate the bit offset x for each slot
    int x = static_cast<int>(std::ceil(static_cast<double>(plaintext_size) / n_packed));

    std::vector<std::vector<mpz_class>> original_plaintexts(n_packed);

    // Process each packed ciphertext
    for (const auto& ptxt : packed_plaintexts) {

        mpz_class value(ptxt.to_string_(16), 16);

        // Extract the individual plaintexts from the packed plaintext
        for (int j = 0; j < n_packed; ++j) {
            // Create the mask for the current slot
            mpz_class mask = (mpz_class(1) << x) - 1;

            // Extract the value for the current slot
            mpz_class extracted_value = value & mask;

            value = value >> x;

            // Add the extracted plaintext to the result list
            original_plaintexts[n_packed - j - 1].push_back(extracted_value);
        }
    }

    return original_plaintexts;
}