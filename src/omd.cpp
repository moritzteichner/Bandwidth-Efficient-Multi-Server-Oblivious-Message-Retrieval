/*#include <chrono>
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
using namespace NTL;*/
namespace OMD {
// ------------------------------------------------------------------------Protocol Parameters --------------------------------------------------------------
int NUM_Messages;
int N_PER_DFT = NUM_Messages;
int LOG_NUM_USERS;
int paillier_bits_prime;
int k;
int p = 1;

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
void sendMessage(size_t recipient, mpz_class message){
    // Generater DPF that evaluates to 1 on the input of the recipient.
    auto keys = DPF::Gen(recipient, LOG_NUM_USERS);

    MessageWithClue msg_S0(message, keys.first);
    MessageWithClue msg_S1(message, keys.second);

    // Simulate sending the messages and attached clues to both servers.
    receiveMessage(0, msg_S0);
    receiveMessage(1, msg_S1);
    return;
}

Vec<Ciphertext> DFT(Vec<PackedCiphertext> ciphertexts, PaillierFast pk, double eps) {
    int coeff_size;

    int k2 = findSmallestK(k, p, eps);
    coeff_size = 2*k2;

    if (N_PER_DFT < coeff_size) {
        coeff_size = N_PER_DFT / 2;
    }
    Vec<Ciphertext> coefficients;
    coefficients.SetLength(coeff_size);

    Ciphertext summand = pk.encrypt(0);
    Ciphertext sum = pk.encrypt(0);

    for (int i = 0; i < coeff_size; i++) {
         for (int j = 0; j < N_PER_DFT; j++) {
            Integer power = z;
            power = power.pow_mod_n(i*j, q);
            summand = ciphertexts[j].data * power;
            sum = sum + summand;
         }
         coefficients[i] = sum;
         sum = pk.encrypt(0);
             }

    return coefficients;
}

Vec<Ciphertext> retrieveMessages(PaillierFast pk, int receiver, double eps) {
    std::vector<int> messages;
    // Part 1: Both servers evaluate shares of DPF.
    std::vector<bool> share0;
    for (int i = 0; i < server0.size(); i++) {
        bool share = DPF::Eval(server0[i].getClue(), receiver, LOG_NUM_USERS);
        share0.push_back(share);
        // Not needed for OMD: messages.push_back(server0[i].getMessage());
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

    // Part 3: Pack the ciphertexts to perform multiple DFTs at the same time.

    int x = static_cast<int>(std::ceil(static_cast<double>(2*paillier_bits_prime - 2) / p)) - 1;
    Vec<PackedCiphertext> packed_PV = ophelib::Vector::pack_ciphertexts_vec(PV, x, pk);

    Vec<Ciphertext> packed_PV_ctx;
    packed_PV_ctx.SetLength(packed_PV.length());
    for (int i = 0; i < packed_PV_ctx.length(); i++) {
        packed_PV_ctx[i] = packed_PV[i].data;
    }

    // Part 4: Compress the sparse data vector using a Discrete Fourier Transform evaluated under HE.

    Vec<Ciphertext> digest = DFT(packed_PV, pk, eps);

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

    for (int i = 0; i < N_PER_DFT; i++) {
        mpz_class sum = 0;
        for (int j = 0; j < N_PER_DFT; j++) {
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

    while (extendedVec.size() < N_PER_DFT) {
        mpz_class newElement = 0;
        
        for (int i = 0; i < k; ++i) {
            newElement += (extendedVec[extendedVec.size() - k + i] * multiplier[i]) % q;
            newElement %= q;
        }
        extendedVec.push_back(newElement);
    }

    return extendedVec;
}

std::vector<mpz_class> decodeDigest(
    PaillierFast sk,
    Vec<Ciphertext> digest,
    const mpz_class& z) {
    // Part 1: Unpack the Ciphertexts. This is a 2D array as we have multiple coeffcients for multiple parallel DFTs.
    Vec<Integer> decrypted;
    decrypted.SetLength(digest.length());
    for (int i = 0; i < decrypted.length(); i++) {
        decrypted[i] = sk.decrypt(digest[i]);
    }

    std::vector<std::vector<mpz_class>> d = unpack(decrypted, 2*paillier_bits_prime -2, p);
    std::vector<mpz_class> pertinent_indices;

    for (int i = 0; i < d.size(); i++) {
        for (int j = 0; j < d[i].size(); j++) {
            d[i][j] = d[i][j] % q;
        }
        //For OMD, the first Fourier coefficient is the number of pertinent messages, k.
        int actual_k = mpz_get_ui(d[i][0].get_mpz_t());
        //std::cout << "Actual k: " << d[i][0] << std::endl;

        //Warning: In the wrong application, this optimisation of ignoring DFTs without non-zero elements can be a side channel to discover a recipients k value.
        if (actual_k > 0) {
            std::vector<mpz_class> x;
            try {
                x = solveModularLinearSystem(d[i], actual_k, q);
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
            }
            vector<mpz_class> firstElements(d[i].begin(), d[i].begin() + actual_k);
            vector<mpz_class> freq = inverseBlahoot(firstElements, x, actual_k);
            vector<mpz_class> PV_dec = inverseDFT(freq, z);
            for (int j = 0; j < PV_dec.size(); j++) {
                if ((PV_dec[j] % q) > 0) {
                    pertinent_indices.push_back(i + j*p);
                    //std::cout << "Pertinent Index: " << i + j*p << std::endl;
                }
            }
        }
    }
    return pertinent_indices;
}

Benchmarking omd_run(
    int input_NUM_Messages,
    size_t input_LOG_NUM_USERS,
    int input_paillier_bits_prime,
    int input_k,
    std::vector<int> recievers,
    int RETRIEVING_USER,
    double eps
) {
    NUM_Messages = input_NUM_Messages;
    LOG_NUM_USERS = input_LOG_NUM_USERS;
    paillier_bits_prime = input_paillier_bits_prime;
    k = input_k;

    //Reset Bulletin Boards for new run.
    server0.clear();
    server1.clear();

    // 1. Start sending all messages. 
    auto start_send = std::chrono::high_resolution_clock::now();
    sendMessage(recievers[0], 131346);
    
    for (int i = 1; i < recievers.size(); i++) {
        //The actual message can be a dummy here as we are only woring on OMD.
        sendMessage(recievers[i], 131346);
    }
    auto end_send = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_send = end_send - start_send;
    cout << "Sending of Messages done. It took " << duration_send.count() << " seconds." << endl;

    // 2. Precomputations Server
    auto start_pre = std::chrono::high_resolution_clock::now();

    //First, estimate the packing factor p and the number of messages per DFT.
    p = estimatePackingFactor((2*paillier_bits_prime)-2, k, NUM_Messages);
    unsigned int x = static_cast<int>(std::ceil(static_cast<double>(2*paillier_bits_prime - 2) / p));


    while (p * x > (2*paillier_bits_prime - 2)) {
        p -= 1;
        x = static_cast<int>(std::ceil(static_cast<double>(2*paillier_bits_prime - 2) / p));
    }
    int k2 = findSmallestK(k, p, eps);
    
    N_PER_DFT = static_cast<int>(std::ceil(static_cast<double>(NUM_Messages) / p));

    //Precalculations for q (DFT modulus) and z (primitive n-th root of unity mod q). Calculation of z is efficient if the prime factorisation of q-1 is known.
    auto [q_, q_factorisation_] = nextPrimeAndFactors(N_PER_DFT);
    q = q_;
    q_factorisation = q_factorisation_;

    //Add dummy messages such that n divides q-1 (by being equal to that)
    int dummy_messages = (q.get_si() - N_PER_DFT) * p;
    N_PER_DFT = q.get_si() - 1;
    z = findPrimitiveRootOfUnity(N_PER_DFT, q, q_factorisation);

    //Send dummy messages addressed to the ghost  identifier 0.
    for (int i = 0; i < dummy_messages; i++) {
        sendMessage(0, 13136);
    }

    auto end_pre = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_pre = end_pre - start_pre;
    cout << "Server Precomputations done. It took " << duration_pre.count() << " seconds." << endl;


    //Create public key for retrieval. Note that in a real application, this would be a lookup.
    PaillierFast crypto(paillier_bits_prime * 2);
    crypto.generate_keys();

    // 3. Retrieval
    auto start_retrieval = std::chrono::high_resolution_clock::now();
    Vec<Ciphertext> digest = retrieveMessages(crypto, RETRIEVING_USER, eps);

    auto end_retrieval = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_retrieval = end_retrieval - start_retrieval;
    cout << "Server Retrieval done. It took " << duration_retrieval.count() << " seconds." << endl;


    // 4. Decode the Digest
    auto start_decode = std::chrono::high_resolution_clock::now();

    std::vector<mpz_class> pertinent_indices = decodeDigest(crypto, digest, z);

    auto end_decode = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_decode = end_decode - start_decode;
    cout << "Client Decoding done. It took " << duration_send.count() << " seconds." << endl;


    // Create Benchmarking object that captures all metrics and the pertinent indices and return it to the benchmarking environment.
    int digest_size = (digest.length() * digest[0].data.size_bits()) / 8;
    cout << "Digest Size: " << digest_size << " bytes." << endl;

    int g_size = crypto.get_pub().g.size_bits() / 8;
    int n_size = crypto.get_pub().n.size_bits() / 8;
    int key_size = (n_size + g_size);
    cout << "PK Size: " << key_size << " bytes." << endl;
    
    Benchmarking benchmark(duration_send.count(), duration_pre.count(), duration_retrieval.count(), duration_decode.count(), digest_size, key_size, pertinent_indices);
    return benchmark;
}
}