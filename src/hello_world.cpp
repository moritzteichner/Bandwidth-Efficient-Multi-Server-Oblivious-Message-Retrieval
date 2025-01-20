#include "ophelib/paillier_fast.h"
#include "ophelib/packing.h"
#include "ophelib/util.h"
#include "ophelib/ml.h"

using namespace std;
using namespace ophelib;

/**
 * Small demo program to show basic functionality of
 * the library.
 */
int hello () {
    cout << "Running on ophelib version " << ophelib_version() << ", git ref " << ophelib_ref() << endl;

    const size_t key_size = 3072;
    PaillierFast crypto(key_size);
    crypto.generate_keys();

    cout << "key size: " << key_size << endl;

    const Integer a = 10;
    const Integer b = 1;
    const Ciphertext a_e = crypto.encrypt(a);
    const Ciphertext b_e = crypto.encrypt(b);

    Vec<Ciphertext> vect;
    vect.SetLength(2);
    vect[0] = a_e;
    vect[1] = b_e;

    PackedCiphertext pack = ophelib::Vector::pack_ciphertexts(vect, 10, crypto);

    Integer dec = crypto.decrypt(pack.data);
    cout << dec.to_string_(2) << endl;
    /*
    
    cout << "n = " << n.to_string_() << ", m = " << m.to_string_() << endl;
    cout << "plaintexts addition: n + m = " << (n + m).to_string_() << endl;
    cout << "sum decrypted: decrypt( n_e + m_e ) = " << crypto.decrypt(sum).to_string_() << endl << endl;

    // plaintexts packing
    const size_t bits_per_plaintext = 128;
    const size_t n_plaintexts = Vector::pack_count(bits_per_plaintext, crypto);
    const Vec<Integer> plaintexts = Vector::rand_bits(n_plaintexts, bits_per_plaintext);
    cout << "plaintexts: ";
    for(auto p: plaintexts)
        cout << p << "; ";
    cout << endl;
    const PackedCiphertext packed = Vector::encrypt_pack(plaintexts, bits_per_plaintext, crypto);
    cout << "packed ciphertext: " << packed.data.data.to_string(false) << endl;
    cout << "packed decrypted: ";
    for(auto p: Vector::decrypt_pack(packed, crypto))
        cout << p << "; ";
    cout << endl << endl;

    // ciphertext packing
    const Vec<Ciphertext> ciphertexts = Vector::encrypt(plaintexts, crypto);
    const auto packed_2 = Vector::pack_ciphertexts(ciphertexts, bits_per_plaintext, crypto);
    cout << "packed_2 decrypted: ";
    for(auto p: Vector::decrypt_pack(packed_2, crypto))
        cout << p << "; ";
    cout << endl << endl;

    // linear regression
    const string path = "../test/fixtures/rand.txt";
    NTL::Mat<float> X;
    NTL::Vec<float> y;
    Vector::load_data(path, X, y);

    Vector::Normalizer normX, normY;
    Vector::Integerizer inter(30);

    const auto interY = inter.double_precision();
    const auto y_tr = interY.transform(normY.fit_transform(y));
    const auto X_tr = inter.transform(normX.fit_transform(X));

    const auto y_enc = Vector::encrypt(y_tr, crypto);

    const auto callback = ML::LinregPlainEnc::construct_client_callback(crypto);
    ML::LinregPlainEnc reg(callback, inter.get_factor(), crypto.get_pub(), 1, 100);
    reg.fit(X_tr, y_enc);
    const auto y_pred = normY.inverse_transform(interY.inverse_transform(Vector::decrypt(reg.predict(X_tr), crypto)));

    cout << "y = " << y << endl;
    cout << "y_pred = " << y_pred << endl;
    */
    return 0;
}
