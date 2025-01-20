#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <gmpxx.h>
#include <string>
#include <random>

#include "ipcl/ipcl.hpp"
#include "ippcp.h"

#include "newTypes.hpp"
#include "utility-final.hpp"
#include "mod_utility.hpp"
#include "packing_utility.hpp"
#include "simulation_utility.hpp"

#include "benchmkaring_class.cpp"

// ------------------------------------------------------------------------Protocol Parameters --------------------------------------------------------------
int NUM_Messages;
int N_PER_DFT = NUM_Messages;
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

std::vector<ipcl::CipherText> DFT(std::vector<ipcl::CipherText> ciphertexts, ipcl::PublicKey pk, ipcl::PrivateKey sk) {
    //TODO: k needs to be changed. It can at maximum be the number for ciphertexts per slot. But in practice, much smaller with a still small error probability.
    std::vector<ipcl::CipherText> coefficients;
    int coeff_size;

    int k2 = findSmallestK(k, p, eps);

    coeff_size = 2*k2;

    if (N_PER_DFT < coeff_size) {
        coeff_size = N_PER_DFT / 2;
    }

    coefficients.resize(coeff_size);

    ipcl::CipherText summand;
    ipcl::PlainText zero_ptxt(0);
    ipcl::CipherText sum = pk.encrypt(zero_ptxt);

    for (int i = 0; i < coeff_size; i++) {
         for (int j = 0; j < N_PER_DFT; j++) {
            mpz_class power = powerMod(z, i*j, q);
            std::string hexStrStr = power.get_str(16);
            const char* hexStr = hexStrStr.c_str();
            size_t newSize = std::strlen(hexStr) + 3;
            char* newHexStr = new char[newSize];
            std::strcpy(newHexStr, "0x");
            std::strcat(newHexStr, hexStr);
            BigNumber bn(newHexStr);
            ipcl::PlainText factor(bn);
            summand = ciphertexts[j] * factor;
            sum = sum + summand;
         }
         coefficients[i] = sum;
         sum = pk.encrypt(zero_ptxt);
    }

    return coefficients;
}

std::vector<ipcl::CipherText>  retrieveMessages(ipcl::PrivateKey sk, ipcl::PublicKey pk, int receiver) {
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
    std::vector<BigNumber> bn0;
    for (int i = 0; i < share0.size(); i++) {
        bn0.push_back((int)share0[i]);

    }
    ipcl::PlainText plaintexts0(bn0);
    ipcl::CipherText ciphertexts0 = pk.encrypt(plaintexts0);

    // Part 2: Server 1 takes encrypted indices from Server 0 and combines with xor obliviously to create the Perticnency Vector PV.
    BigNumber bn_minus1(-1);
    ipcl::PlainText pt_minus1(bn_minus1);
    ipcl::CipherText ct_minus1 = pk.encrypt(pt_minus1);
    ipcl::PlainText minus1 = sk.decrypt(ct_minus1);

    std::vector<BigNumber> bn_mask0;
    std::vector<BigNumber> bn_mask1;

    for (int i = 0; i < share0.size(); i++) {
        if (share1[i] == false) {
            bn_mask0.push_back(0);
            bn_mask1.push_back(1);
        } else {
            bn_mask0.push_back(1);
            bn_mask1.push_back(minus1[0]);
        }
    }
    ipcl::PlainText ptxt0(bn_mask0);
    ipcl::PlainText ptxt1(bn_mask1);
    ipcl::CipherText PV = ptxt0 + ptxt1 * ciphertexts0;
    // -------------------------------------------------------------

    // Part 3: Pack the ciphertexts to perform multiple DFTs at the same time.

    std::vector<ipcl::CipherText> packed_PV = pack_ciphertexts(PV, pk, share0.size(), paillier_bits_prime -1, p, sk);

    // Part 4: Compress the sparse data vector using a Discrete Fourier Transform evaluated under HE.

    std::vector<ipcl::CipherText> digest = DFT(packed_PV, pk, sk);

    // Return the DFT coefficients as a digest.
    return digest;
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

void decodeDigest(
    ipcl::PrivateKey sk,
    const std::vector<ipcl::CipherText>& digest,
    const mpz_class& z) {
    // Part 1: Unpack the Ciphertexts. This is a 2D array as we have multiple coeffcients for multiple parallel DFTs.
    std::vector<std::vector<mpz_class>> d = unpack_ciphertexts(digest, sk, paillier_bits_prime - 1, p);
    //i loops through DFTs and j through coeffcients.
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
            std::vector<mpz_class> firstElements(d[i].begin(), d[i].begin() + actual_k);
            std::vector<mpz_class> freq = inverseBlahoot(firstElements, x, actual_k);
            std::vector<mpz_class> PV_dec = inverseDFT(freq, z);
            /*for (int j = 0; j < PV_dec.size(); j++) {
                if ((PV_dec[j] % q) > 0) {
                    std::cout << "Pertinent Index: " << i + j*p << std::endl;
                }
            }*/
        }
    }
}


Benchmarking omd(
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

    auto start_send = std::chrono::high_resolution_clock::now();
    // 1. Start sending all messages. 
    
    for (int receiver : recievers) {
        //The actual message can be a dummy here as we are only woring on OMD.
        sendMessage(receiver, 131346);
    }

    auto end_send = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_send = end_send - start_send;

    // 2. Precomputations Server
    auto start_pre = std::chrono::high_resolution_clock::now();

    //First, estimate the packing factor p and the number of messages per DFT.
    p = estimatePackingFactor((paillier_bits_prime)-1, k, NUM_Messages);
    int k2 = findSmallestK(k, p, eps);
    p = estimatePackingFactor((paillier_bits_prime)-1, k2, NUM_Messages);
    int k3 = findSmallestK(k, p, eps);
    p = estimatePackingFactor((paillier_bits_prime)-1, k3, NUM_Messages);

    
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

    auto end_pre = std::chrono::high_resolution_clock::now();  // Record the end time
    std::chrono::duration<double> duration_pre = end_pre - start_pre;

    //Create public key for retrieval. Note that in a real application, this would be a lookup.
    ipcl::KeyPair key = ipcl::generateKeypair(paillier_bits_prime);
    ipcl::PublicKey pk = key.pub_key;
    ipcl::PrivateKey sk = key.priv_key;

    // 3. Retrieval
    auto start_retrieval = std::chrono::high_resolution_clock::now();

    std::vector<ipcl::CipherText> digest = retrieveMessages(sk, pk, RETRIEVING_USER);

    auto end_retrieval = std::chrono::high_resolution_clock::now();  // Record the end time
    std::chrono::duration<double> duration_retrieval = end_retrieval - start_retrieval;

    ipcl::PlainText test = sk.decrypt(digest[0]);

    // Measure Digest Size in bytes.
    std::ostringstream os;
    ipcl::serializer::serialize(os, digest[0]);
    std::string serialized_ct = os.str();
    int digest_size = serialized_ct.size() * digest.size();

    //Measure the size of the public key.
    ipcl::serializer::serialize(os, pk);
    std::string serialized_pk = os.str();
    int pk_size = serialized_pk.size();

    // 4. Decode the Digest
    auto start_decode = std::chrono::high_resolution_clock::now();

    decodeDigest(sk, digest, z);

    auto end_decode = std::chrono::high_resolution_clock::now();  // Record the end time
    std::chrono::duration<double> duration_decode = end_decode - start_decode;

    // Create Benchmarking object that captures all metrics and return it to the benchmarking environment.

    Benchmarking benchmark(duration_send.count(), duration_pre.count(), duration_retrieval.count(), duration_decode.count(), digest_size, pk_size);
    return benchmark;
}