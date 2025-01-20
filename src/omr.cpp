#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <gmpxx.h>
#include <string>
#include <random>

#include "newTypes.hpp"
#include "packing_utility.hpp"
#include "utility-final.hpp"
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

// ------------------------------------------------------------------------Protocol Parameters --------------------------------------------------------------
int NUM_Messages;
int LOG_NUM_USERS;
int paillier_bits_prime;
int k;
int p = 1;
double eps = pow(2.0, -40);

int RETRIEVING_USER = 7;

//Determine Modulus q and primitive n-th root of unity z
mpz_class q, z;

std::vector<mpz_class> q_factorisation;

// ------------------------------------------------------------------------Bulletin Boards of Servers-------------------------------------------------

std::vector<MessageWithClue> server0;
std::vector<MessageWithClue> server1;
// ------------------------------------------------------------------------Server Implementation ------------------------------------------------------------
void receiveMessage(int server, MessageWithClue msg){
    if (server == 0) {
        server0.push_back(msg);
    } else {
        server1.push_back(msg);
    }
    return;
}

// ------------------------------------------------------------------------Client (Sender) Send Implementation --------------------------------------------------------
void sendMessage(size_t recipient, int message){
    // Generater DPF that evaluates to 1 on the input of the recipient.
    auto keys = DPF::Gen(recipient, LOG_NUM_USERS);

    MessageWithClue msg_S0(message, keys.first);
    MessageWithClue msg_S1(message, keys.second);

    // Simulate sending the messages and attached clues to both servers.
    receiveMessage(0, msg_S0);
    receiveMessage(1, msg_S1);
    return;
}

Vec<Ciphertext> DFT(Vec<Ciphertext> ciphertexts, PaillierFast pk) {
    //TODO: k needs to be changed. It can at maximum be the number for ciphertexts per slot. But in practice, much smaller with a still small error probability.
    int coeff_size;
    coeff_size = 2*k;

    Vec<Ciphertext> coefficients;
    coefficients.SetLength(coeff_size);

    Ciphertext summand = pk.encrypt(0);
    Ciphertext sum = pk.encrypt(0);

    for (int i = 0; i < coeff_size; i++) {
         for (int j = 0; j < NUM_Messages; j++) {
            Integer power = z;
            power = power.pow_mod_n(i*j, q);
            summand = ciphertexts[j] * power;
            sum = sum + summand;
         }
         coefficients[i] = sum;
         sum = pk.encrypt(0);
    }

    return coefficients;
}

Vec<Ciphertext>  retrieveMessages(PaillierFast pk, int receiver) {
    std::vector<int> messages;
    // Part 1: Both servers evaluate shares of DPF.

    std::vector<bool> share0;
    for (int i = 0; i < server0.size(); i++) {
        bool share = DPF::Eval(server0[i].getClue(), receiver, LOG_NUM_USERS);
        share0.push_back(share);
        messages.push_back(server0[i].getMessage());
    }

    std::vector<bool> share1;
    for (int i = 0; i < server1.size(); i++) {
        bool share = DPF::Eval(server1[i].getClue(), receiver, LOG_NUM_USERS);
        share1.push_back(share);
    }
    // Part 1.5: Server 0 encrypts its indices and sends them to Server 1.
    std::vector<Ciphertext> share0_enc;
    for (int i = 0; i < share0.size(); i++) {
        Integer ptxt = (int)share0[i];
        share0_enc.push_back(pk.encrypt(ptxt));

    }
    // Part 2: Server 1 takes encrypted indices from Server 0 and combines with xor obliviously to create the Perticnency Vector PV.

    Vec<Ciphertext> PV;
    PV.SetLength(share0.size());
    for (int i = 0; i < share0.size(); i++) {
        if (share1[i] == false) {
            PV[i] = share0_enc[i];
        } else {
            Integer ptxt = (int)share1[i];
            PV[i] = pk.encrypt(ptxt) - share0_enc[i];
        }
    }

    // Part 3: Multiply the PV with the data vector.

    for (int i = 0; i < PV.length(); i++) {
        PV[i] = PV[i] * messages[i];
    }

    // Part 4: Compress the sparse data vector using a Discrete Fourier Transform evaluated under HE.

    Vec<Ciphertext> digest = DFT(PV, pk);

    // Return the DFT coefficients as a digest.
    return digest;
}

// ------------------------------------------------------------------------Client (Receiver) Decode Implementation --------------------------------------------------------


//Perform the inverse Discrete Fourier Transform.
std::vector<mpz_class> inverseDFT(std::vector<mpz_class> coefficients, const mpz_class &z) {
    mpz_class N = coefficients.size();
    mpz_class z_inv;
    z_inv = modInverse(z, q);

    mpz_class p_inv = modInverse(N, q);

    std::vector<mpz_class> result;
    result.resize(coefficients.size());

    for (int i = 0; i < NUM_Messages; i++) {
        mpz_class sum = 0;
        for (int j = 0; j < NUM_Messages; j++) {
            sum +=  (coefficients[j] * powerMod(z_inv, i*j, q)) % q;
        }
        result[i] = (sum * p_inv) % q;
        sum = 0;
    }
    return result;
}

//Use Blahut's Theorem to reconstruct the full vector of  Fourier  coefficients.
std::vector<mpz_class> inverseBlahoot(const std::vector<mpz_class>& vec, const std::vector<mpz_class>& multiplier, int k) {
    std::vector<mpz_class> extendedVec = vec;
    
    if (multiplier.size() != k) {
        std::cerr << "Error: Multiplier vector must have the same size as k." << std::endl;
        return extendedVec;
    }

    while (extendedVec.size() < NUM_Messages) {
        mpz_class newElement = 0;
        
        for (int i = 0; i < k; ++i) {
            newElement += (extendedVec[extendedVec.size() - k + i] * multiplier[i]) % q;
            newElement %= q;
        }
        extendedVec.push_back(newElement);
    }

    return extendedVec;
}

void decodeDigest(
    PaillierFast sk,
    Vec<Ciphertext> digest,
    const mpz_class& z) {
    vector<mpz_class> d;
    d.resize(digest.length());
    for (int i = 0; i < d.size(); i++) {
        mpz_class value(sk.decrypt(digest[i]).to_string_(16), 16);
        d[i] = value % q;
    }
    std::vector<mpz_class> x;
    try {
        x = solveModularLinearSystem(d, k, q);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    vector<mpz_class> firstElements(d.begin(), d.begin() + k);
    vector<mpz_class> freq = inverseBlahoot(firstElements, x, k);
    vector<mpz_class> PV_dec = inverseDFT(freq, z);
    /*for (int j = 0; j < PV_dec.size(); j++) {
        if ((PV_dec[j] % q) > 0) {
            std::cout << "Pertinent Message at index " << j << ": " << (PV_dec[j] % q) << std::endl;
        }
    }*/
}

Benchmarking omr_run(
    int input_NUM_Messages,
    size_t input_LOG_NUM_USERS,
    int input_paillier_bits_prime,
    int input_k
) {
    NUM_Messages = input_NUM_Messages;
    LOG_NUM_USERS = input_LOG_NUM_USERS;
    paillier_bits_prime = input_paillier_bits_prime;
    k = input_k;

    //Reset Bulletin Boards for new run.
    server0.clear();
    server1.clear();

    //Generate random receivers, including k messages for the retrieving user of this simulation.
    std::vector<int> recievers = generateClients(k, NUM_Messages, LOG_NUM_USERS, RETRIEVING_USER);
    std::vector<int> pertinent_indices;

    //Remember the real pertinent indices to check the output of Decode.
    for (int i = 0; i < recievers.size(); i++) {
        if (recievers[i] == RETRIEVING_USER) {
            pertinent_indices.push_back(i);
        }
    }

    auto start_send = std::chrono::high_resolution_clock::now();
    // 1. Start sending all messages. 
    
    for (int receiver : recievers) {
        //The actual message can be a dummy here as we are only woring on OMD.
        sendMessage(receiver, 17255);
    }

    auto end_send = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_send = end_send - start_send;

    // 2. Precomputations Server
    auto start_pre = std::chrono::high_resolution_clock::now();

    int log_q = 2*paillier_bits_prime - 3 - static_cast<int>(std::ceil(std::log2(k)));

    auto [q_, q_factorisation_] = findPrimeAndFactors(NUM_Messages, 4000);
    q = q_;
    q_factorisation = q_factorisation_;
    z = findPrimitiveRootOfUnity(NUM_Messages, q, q_factorisation);

    cout << "Found Prime." << endl;

    auto end_pre = std::chrono::high_resolution_clock::now();  // Record the end time
    std::chrono::duration<double> duration_pre = end_pre - start_pre;

    //Create public key for retrieval. Note that in a real application, this would be a lookup.
    PaillierFast crypto(paillier_bits_prime * 2);
    crypto.generate_keys();

    // 3. Retrieval
    auto start_retrieval = std::chrono::high_resolution_clock::now();

    Vec<Ciphertext> digest = retrieveMessages(crypto, RETRIEVING_USER);

    auto end_retrieval = std::chrono::high_resolution_clock::now();  // Record the end time
    std::chrono::duration<double> duration_retrieval = end_retrieval - start_retrieval;

    // 4. Decode the Digest
    auto start_decode = std::chrono::high_resolution_clock::now();

    decodeDigest(crypto, digest, z);

    auto end_decode = std::chrono::high_resolution_clock::now();  // Record the end time
    std::chrono::duration<double> duration_decode = end_decode - start_decode;

    // Create Benchmarking object that captures all metrics and return it to the benchmarking environment.

    Benchmarking benchmark(duration_send.count(), duration_pre.count(), duration_retrieval.count(), duration_decode.count(), 0, 0);
    return benchmark;

    return 0;
}