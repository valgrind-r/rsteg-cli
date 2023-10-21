#include <openssl/aes.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <cstring>

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int encrypt(std::vector<unsigned char>& plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, std::vector<unsigned char>& ciphertext)
{
    int k_len = strlen((const char*)key), plaintext_length = static_cast<int>(plaintext.size());

    EVP_CIPHER_CTX *en;
    en = EVP_CIPHER_CTX_new();

    /* Create and initialise the context */
    EVP_CIPHER_CTX_init(en);

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example, we are using 256-bit AES (i.e., a 256-bit key). The
     * IV size for *most* modes is the same as the block size. For AES, this
     * is 128 bits
     */
    if (1 != EVP_EncryptInit_ex(en, EVP_aes_256_cbc(), NULL, key, iv)) {
        fprintf(stderr, "Error: EVP_EncryptInit_ex() failed.\n");
        EVP_CIPHER_CTX_free(en);
        return -1; // Return an error code
    }

    int c_len = plaintext_length + AES_BLOCK_SIZE, f_len = 0;
    ciphertext.resize(c_len);

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if (1 != EVP_EncryptUpdate(en, ciphertext.data(), &c_len, plaintext.data(), plaintext_len)) {
        fprintf(stderr, "Error: EVP_EncryptUpdate() failed.\n");
        EVP_CIPHER_CTX_free(en);
        return -1; // Return an error code
    }

    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if (1 != EVP_EncryptFinal_ex(en, ciphertext.data() + c_len, &f_len)) {
        fprintf(stderr, "Error: EVP_EncryptFinal_ex() failed.\n");
        EVP_CIPHER_CTX_free(en);
        return -1; // Return an error code
    }

    ciphertext.erase(ciphertext.begin() + c_len + f_len, ciphertext.end());

    /* Clean up */
    EVP_CIPHER_CTX_free(en);

    return f_len;
}

int decrypt(std::vector<unsigned char>& ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, std::vector<unsigned char>& plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int k_len = strlen((const char *)key);
    int p_len = static_cast<int>(plaintext.size()), f_len = 0;

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        handleErrors();
    }

    EVP_CIPHER_CTX_init(ctx);

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        handleErrors();
    }

    if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &p_len, ciphertext.data(), p_len)) {
        handleErrors();
    }

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + p_len, &f_len)) {
        // Print error and details if decryption fails
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        return -1; // Indicate decryption failure
    }

    EVP_CIPHER_CTX_free(ctx);

    return f_len;
}

int encrypt_seed(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt_seed(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

EVP_PKEY* loadEcdhKey(const std::string& keyPath, bool isPrivate) {
    FILE* keyFile = fopen(keyPath.c_str(), "r");
    if (!keyFile) {
        throw std::runtime_error("Unable to open key file.");
    }

    EVP_PKEY* key = nullptr;
    if (isPrivate) {
        key = PEM_read_PrivateKey(keyFile, NULL, NULL, NULL);
    } else {
        key = PEM_read_PUBKEY(keyFile, NULL, NULL, NULL);
    }
    fclose(keyFile);
    if (!key) {
        throw std::runtime_error("Unable to load key.");
    }

    return key;
}

std::vector<unsigned char> computeSharedSecret(const std::string& privateKeyPath, const std::string& publicKeyPath) {
    EVP_PKEY *privateKey = loadEcdhKey(privateKeyPath, true);
    EVP_PKEY *publicKey = loadEcdhKey(publicKeyPath, false);

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(privateKey, NULL);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Failed to create EVP_PKEY_CTX.");
    }

    if (EVP_PKEY_derive_init(ctx) <= 0) {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Failed to initialize derivation.");
    }

    if (EVP_PKEY_derive_set_peer(ctx, publicKey) <= 0) {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Failed to set peer key.");
    }

    size_t secretLen = 0;
    if (EVP_PKEY_derive(ctx, NULL, &secretLen) <= 0) {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Failed to determine shared secret length.");
    }

    std::vector<unsigned char> secret(secretLen);
    if (EVP_PKEY_derive(ctx, secret.data(), &secretLen) <= 0) {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Failed to derive shared secret.");
    }

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(privateKey);
    EVP_PKEY_free(publicKey);

    return secret;
}

void deriveAesKeyAndIv(const std::vector<unsigned char>& sharedSecret, unsigned char* aesKey, unsigned char* iv) {
    const size_t AES_KEY_SIZE = 32; // AES-256 key size
    const size_t IV_SIZE = 16;      // AES block size for IV
    const unsigned char* salt = reinterpret_cast<const unsigned char*>("salt"); // Use a secure random salt in practice
    const int iterations = 10000; // Number of PBKDF2 iterations

    // Derive AES key using PBKDF2 with HMAC-SHA256
    if (!PKCS5_PBKDF2_HMAC(reinterpret_cast<const char*>(sharedSecret.data()), sharedSecret.size(),
                           salt, strlen(reinterpret_cast<const char*>(salt)), iterations,
                           EVP_sha256(), AES_KEY_SIZE, aesKey)) {
        throw std::runtime_error("PBKDF2 key derivation with HMAC-SHA256 failed.");
    }

    // Derive IV using PBKDF2 with HMAC-SHA256
    if (!PKCS5_PBKDF2_HMAC(reinterpret_cast<const char*>(sharedSecret.data()), sharedSecret.size(),
                           salt, strlen(reinterpret_cast<const char*>(salt)), iterations,
                           EVP_sha256(), IV_SIZE, iv)) {
        throw std::runtime_error("PBKDF2 IV derivation with HMAC-SHA256 failed.");
    }

    // Optional: Debugging output (uncomment for debugging)
    /*
    std::cout << "Derived AES Key: ";
    for (size_t i = 0; i < AES_KEY_SIZE; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(aesKey[i]);
    }
    std::cout << std::dec << std::endl;

    std::cout << "Derived IV: ";
    for (size_t i = 0; i < IV_SIZE; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(iv[i]);
    }
    std::cout << std::dec << std::endl;
    */
}