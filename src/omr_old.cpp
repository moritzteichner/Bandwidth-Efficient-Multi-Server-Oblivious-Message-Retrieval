// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <vector>
#include <algorithm>
#include <gmpxx.h>
#include <string>

#include "ipcl/ipcl.hpp"
#include "ippcp.h"

#include "newTypes.hpp"
#include "utility-final.hpp"
#include "mod_utility.hpp"

// ------------------------------------------------------------------------Protocol Parameters --------------------------------------------------------------
int NUM_Messages = 100000;
size_t LOG_NUM_USERS = 15;
bool omr = true;
int LOG_MES_SIZE = 128;
int paillier_bits_prime = 1536;
int k = 50; //Bund on the number of pertinent messages

int RETRIEVING_USER = 7;

//Determine Modulus q and primitive n-th root of unity z
mpz_class q, z;

std::vector<mpz_class> q_factorisation;

// ------------------------------------------------------------------------Temporary Variables until QRPC works-------------------------------------------------
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

std::vector<ipcl::CipherText> DFT(ipcl::CipherText ciphertexts, ipcl::PublicKey pk, ipcl::PrivateKey sk) {
    std::vector<ipcl::CipherText> coefficients;
    coefficients.resize(2 * k);

    ipcl::CipherText summand;
    ipcl::PlainText zero_ptxt(0);
    ipcl::CipherText sum = pk.encrypt(zero_ptxt);

    for (int i = 0; i < (2*k); i++) {
        std::vector<BigNumber> factors;
        ipcl::PlainText factors_ptxt;
        ipcl::CipherText mult;

         for (int j = 0; j < NUM_Messages; j++) {
            mpz_class power = powerMod(z, i*j, q);
            std::string hexStrStr = power.get_str(16);
            const char* hexStr = hexStrStr.c_str();
            size_t newSize = std::strlen(hexStr) + 3;
            char* newHexStr = new char[newSize];
            std::strcpy(newHexStr, "0x");
            std::strcat(newHexStr, hexStr);
            BigNumber bn(newHexStr);
            factors.push_back(bn);
            //ipcl::PlainText factor(bn);
            //summand = ciphertexts.getCipherText(j) * factor;

            //sum = sum + summand;
         }
         std::cout << "Start " << i << std::endl;
         factors_ptxt = ipcl::PlainText(factors);
         mult = ciphertexts * factors_ptxt;
         for (int j = 0; j < NUM_Messages; j++) {
            sum = sum + mult.getCipherText(j);
         }
         coefficients[i] = sum;
         sum = pk.encrypt(zero_ptxt);
         std::cout << "End " << i << std::endl;
    }

    return coefficients;
}

std::vector<ipcl::CipherText> retrieveMessages(ipcl::PrivateKey sk, ipcl::PublicKey pk, int receiver) {
    std::vector<int> messages;
    //Part 1: Evaluate shares of DPF.

    //Server 0
    std::vector<bool> share0;
    for (int i = 0; i < server0.size(); i++) {
        bool share = DPF::Eval(server0[i].getClue(), receiver, LOG_NUM_USERS);
        share0.push_back(share);
        messages.push_back(server0[i].getMessage());
    }

    //Server 1
    std::vector<bool> share1;
    for (int i = 0; i < server1.size(); i++) {
        bool share = DPF::Eval(server1[i].getClue(), receiver, LOG_NUM_USERS);
        share1.push_back(share);
        messages.push_back(server0[i].getMessage());
    }
    // -------------------------------------------------------------
    //Server 0 encrypts its indices and sends them to Server 1.
    std::vector<BigNumber> bn1;
    for (int i = 0; i < share0.size(); i++) {
        bn1.push_back((int)share0[i]);

    }
    std::cout << "Starting now." << std::endl;
    ipcl::PlainText plaintexts0(bn1);
    ipcl::CipherText ciphertexts0 = pk.encrypt(plaintexts0);

    BigNumber bn_minus1(-1);
    ipcl::PlainText pt_minus1(bn_minus1);
    ipcl::CipherText ct_minus1 = pk.encrypt(pt_minus1);
    ipcl::PlainText minus1 = sk.decrypt(ct_minus1);

    std::cout << "Encryption done" << std::endl;
    //Server 1 takes encrypted indices from Server 0 and combines with xor obliviously to create the Perticnency Vector PV.
    std::vector<BigNumber> bn_mask0;
    std::vector<BigNumber> bn_mask1;

    //PV.resize(share0.size());
    for (int i = 0; i < share0.size(); i++) {
        if (share1[i] == false) {
            //PV[i] = ciphertexts0[i];
            bn_mask0.push_back(0);
            bn_mask1.push_back(1);
        } else {
            bn_mask0.push_back(1);
            bn_mask1.push_back(minus1[0]);
            //ipcl::PlainText pt(share1[i]);
            //ipcl::CipherText ctxt = pk.encrypt(pt);
            //ipcl::CipherText inv = minus1 * ciphertexts0[i];
            //PV[i] = ctxt + inv;
        }
    }
    ipcl::PlainText ptxt0(bn_mask0);
    ipcl::PlainText ptxt1(bn_mask1);
    ipcl::CipherText PV = ptxt0 + ptxt1 * ciphertexts0;
    // -------------------------------------------------------------
    std::cout << "XOR done" << std::endl;
    //Part 3: Multiply the PV with the data vector to obtain the sparse data vector.

    std::vector<BigNumber> bn_mes;
    for (int i = 0; i < NUM_Messages; i++) {
        bn_mes.push_back(messages[i]);
    }
    ipcl::PlainText ptxt_mes(bn_mes);
    PV = PV * ptxt_mes;
    std::cout << "PV Multiply done." << std::endl;
    //Part 4: Compress the sparse data vector using a Discrete Fourier Transform evaluated under HE.

    std::vector<ipcl::CipherText> digest = DFT(PV, pk, sk);
    std::cout << "DFT done" << std::endl;
    // -------------------------------------------------------------

    return digest;
}





// ------------------------------------------------------------------------Client Send Implementation --------------------------------------------------------
void sendMessage(size_t receiver, int message){
    auto keys = DPF::Gen(receiver, LOG_NUM_USERS);

    MessageWithClue msg_S0(message, keys.first);
    MessageWithClue msg_S1(message, keys.second);

    // Here comes the QRPC Part to send these two shares to the two servers.
    receiveMessage(0, msg_S0);
    receiveMessage(1, msg_S1);
    return;
}

// ------------------------------------------------------------------------Client Decode Implementation --------------------------------------------------------



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

std::vector<mpz_class> inverseBlahoot(const std::vector<mpz_class>& vec, const std::vector<mpz_class>& multiplier, int k) {
    std::vector<mpz_class> extendedVec = vec;  // Start with the original vector
    
    // Check that multiplier has the same size as s
    if (multiplier.size() != k) {
        std::cerr << "Error: Multiplier vector must have the same size as s." << std::endl;
        return extendedVec;
    }

    // Extend the vector until it reaches size p
    while (extendedVec.size() < NUM_Messages) {
        mpz_class newElement = 0;
        
        // Compute the new element as the sum of element-wise products, mod q
        for (int i = 0; i < k; ++i) {
            newElement += (extendedVec[extendedVec.size() - k + i] * multiplier[i]) % q;
            newElement %= q;  // Keep result within mod q after each addition
        }
        
        // Append the new element to the extended vector
        extendedVec.push_back(newElement);
    }

    return extendedVec;
}

void decodeDigest(
    ipcl::PrivateKey sk,
    const std::vector<ipcl::CipherText>& digest,
    const mpz_class& z) {

    std::vector<mpz_class> d;
    d.resize(digest.size());

    for (int i = 0; i < digest.size(); i++) {
        ipcl::PlainText dec = sk.decrypt(digest[i]);
        mpz_class value = BigNumberToMPZ(dec[0]);
        d[i] = (value % q);
    }
    int actual_k = mpz_get_ui(d[0].get_mpz_t());
    //TODO: Replace Hardcoded 2
    actual_k = 2;
    std::vector<mpz_class> x;
    try {
        x = solveModularLinearSystem(d, actual_k, q);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    std::vector<mpz_class> firstElements(d.begin(), d.begin() + actual_k);
    std::vector<mpz_class> freq = inverseBlahoot(firstElements, x, actual_k);
    std::vector<mpz_class> PV_dec = inverseDFT(freq, z);
    for (int i = 0; i < 5; i++) {
        std::cout << "PV: " << PV_dec[i] % q << std::endl;
    }
}


int omr_method() {
    //Precalculations (q, z)
    auto [q_, q_factorisation_] = findPrimeAndFactors(500000, LOG_MES_SIZE);
    q = q_;
    q_factorisation = q_factorisation_;
    std::cout << "Q: " << q_ << std::endl;
    z = findPrimitiveRootOfUnity(NUM_Messages, q, q_factorisation);
    std::cout << "z found" << std::endl;
    //Sending Messages
    sendMessage(RETRIEVING_USER, 244);
    sendMessage(RETRIEVING_USER, 249);
    for (int i = 0; i < NUM_Messages - 2; i++) {
        sendMessage(2, 13136);
    }
    std::cout << "Sent all Messages!" << std::endl;
    //Request Retrieval
    ipcl::KeyPair key = ipcl::generateKeypair(paillier_bits_prime);
    ipcl::PublicKey pk = key.pub_key;
    ipcl::PrivateKey sk = key.priv_key;

    std::vector<ipcl::CipherText> digest = retrieveMessages(sk, pk, RETRIEVING_USER);

    //decodeDigest(sk, digest, z);
    return 0;
}