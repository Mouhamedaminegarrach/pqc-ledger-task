#include "pqc_ledger/crypto/classical.hpp"
#include <fstream>
#include <stdexcept>

#ifdef HAVE_OPENSSL
#include <openssl/evp.h>
#include <openssl/ed25519.h>
#else
// Ed25519 constants when OpenSSL is not available
#define ED25519_PUBKEY_SIZE 32
#define ED25519_PRIVATE_KEY_LEN 32
#define ED25519_SIG_SIZE 64
#endif

namespace pqc_ledger::crypto {

Result<std::pair<PublicKey, std::vector<uint8_t>>> generate_ed25519_keypair() {
#ifdef HAVE_OPENSSL
    // Ed25519 keypair is 32 bytes private key + 32 bytes public key
    PublicKey pubkey(ED25519_PUBKEY_SIZE);
    std::vector<uint8_t> privkey(ED25519_PRIVATE_KEY_LEN);  // 32 bytes
    
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
    if (!ctx) {
        return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Err(
            Error(ErrorCode::KeyGenerationFailed, "Failed to create Ed25519 context"));
    }
    
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Err(
            Error(ErrorCode::KeyGenerationFailed, "Failed to initialize Ed25519 keygen"));
    }
    
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Err(
            Error(ErrorCode::KeyGenerationFailed, "Failed to generate Ed25519 keypair"));
    }
    
    // Extract public key
    size_t pubkey_len = ED25519_PUBKEY_SIZE;
    if (EVP_PKEY_get_raw_public_key(pkey, pubkey.data(), &pubkey_len) <= 0) {
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Err(
            Error(ErrorCode::KeyGenerationFailed, "Failed to extract Ed25519 public key"));
    }
    
    // Extract private key
    size_t privkey_len = ED25519_PRIVATE_KEY_LEN;
    if (EVP_PKEY_get_raw_private_key(pkey, privkey.data(), &privkey_len) <= 0) {
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Err(
            Error(ErrorCode::KeyGenerationFailed, "Failed to extract Ed25519 private key"));
    }
    
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);
    
    return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Ok({pubkey, privkey});
#else
    return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Err(
        Error(ErrorCode::KeyGenerationFailed, "OpenSSL not available. Ed25519 requires OpenSSL."));
#endif
}

Result<PublicKey> load_ed25519_public_key(const std::string& path) {
#ifdef HAVE_OPENSSL
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<PublicKey>::Err(Error(ErrorCode::FileReadError, "Cannot open file: " + path));
    }
    
    PublicKey key(ED25519_PUBKEY_SIZE);
    file.read(reinterpret_cast<char*>(key.data()), ED25519_PUBKEY_SIZE);
    file.close();
    
    if (file.gcount() != ED25519_PUBKEY_SIZE) {
        return Result<PublicKey>::Err(Error(ErrorCode::InvalidPublicKey, "Invalid Ed25519 public key size"));
    }
    
    return Result<PublicKey>::Ok(std::move(key));
#else
    return Result<PublicKey>::Err(Error(ErrorCode::FileReadError, "OpenSSL not available. Ed25519 requires OpenSSL."));
#endif
}

Result<std::vector<uint8_t>> load_ed25519_private_key(const std::string& path) {
#ifdef HAVE_OPENSSL
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::FileReadError, "Cannot open file: " + path));
    }
    
    std::vector<uint8_t> key(ED25519_PRIVATE_KEY_LEN);
    file.read(reinterpret_cast<char*>(key.data()), ED25519_PRIVATE_KEY_LEN);
    file.close();
    
    if (file.gcount() != ED25519_PRIVATE_KEY_LEN) {
        return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::InvalidPublicKey, "Invalid Ed25519 private key size"));
    }
    
    return Result<std::vector<uint8_t>>::Ok(std::move(key));
#else
    return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::FileReadError, "OpenSSL not available. Ed25519 requires OpenSSL."));
#endif
}

Result<void> save_ed25519_public_key(const PublicKey& pubkey, const std::string& path) {
#ifdef HAVE_OPENSSL
    if (pubkey.size() != ED25519_PUBKEY_SIZE) {
        return Result<void>::Err(Error(ErrorCode::InvalidPublicKey, "Invalid Ed25519 public key size"));
    }
    
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<void>::Err(Error(ErrorCode::FileWriteError, "Cannot open file for writing: " + path));
    }
    
    file.write(reinterpret_cast<const char*>(pubkey.data()), ED25519_PUBKEY_SIZE);
    file.close();
    
    return Result<void>::Ok();
#else
    return Result<void>::Err(Error(ErrorCode::FileWriteError, "OpenSSL not available. Ed25519 requires OpenSSL."));
#endif
}

Result<void> save_ed25519_private_key(const std::vector<uint8_t>& privkey, const std::string& path) {
#ifdef HAVE_OPENSSL
    if (privkey.size() != ED25519_PRIVATE_KEY_LEN) {
        return Result<void>::Err(Error(ErrorCode::InvalidPublicKey, "Invalid Ed25519 private key size"));
    }
    
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<void>::Err(Error(ErrorCode::FileWriteError, "Cannot open file for writing: " + path));
    }
    
    file.write(reinterpret_cast<const char*>(privkey.data()), ED25519_PRIVATE_KEY_LEN);
    file.close();
    
    return Result<void>::Ok();
#else
    return Result<void>::Err(Error(ErrorCode::FileWriteError, "OpenSSL not available. Ed25519 requires OpenSSL."));
#endif
}

Result<Signature> ed25519_sign(const std::vector<uint8_t>& message,
                                const std::vector<uint8_t>& privkey) {
#ifdef HAVE_OPENSSL
    if (privkey.size() != ED25519_PRIVATE_KEY_LEN) {
        return Result<Signature>::Err(Error(ErrorCode::InvalidPublicKey, "Invalid Ed25519 private key size"));
    }
    
    if (message.size() != 32) {
        return Result<Signature>::Err(Error(ErrorCode::HashError, "Message must be 32 bytes (hash)"));
    }
    
    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, privkey.data(), ED25519_PRIVATE_KEY_LEN);
    if (!pkey) {
        return Result<Signature>::Err(Error(ErrorCode::SignatureVerificationFailed, "Failed to create Ed25519 key"));
    }
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return Result<Signature>::Err(Error(ErrorCode::SignatureVerificationFailed, "Failed to create context"));
    }
    
    Signature sig(ED25519_SIG_SIZE);
    size_t sig_len = ED25519_SIG_SIZE;
    
    if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey) <= 0 ||
        EVP_DigestSign(ctx, sig.data(), &sig_len, message.data(), message.size()) <= 0) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return Result<Signature>::Err(Error(ErrorCode::SignatureVerificationFailed, "Ed25519 signing failed"));
    }
    
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    return Result<Signature>::Ok(std::move(sig));
#else
    return Result<Signature>::Err(Error(ErrorCode::SignatureVerificationFailed, "OpenSSL not available. Ed25519 requires OpenSSL."));
#endif
}

Result<bool> ed25519_verify(const std::vector<uint8_t>& message,
                            const Signature& signature,
                            const PublicKey& pubkey) {
#ifdef HAVE_OPENSSL
    if (pubkey.size() != ED25519_PUBKEY_SIZE) {
        return Result<bool>::Err(Error(ErrorCode::InvalidPublicKey, "Invalid Ed25519 public key size"));
    }
    
    if (signature.size() != ED25519_SIG_SIZE) {
        return Result<bool>::Ok(false);
    }
    
    if (message.size() != 32) {
        return Result<bool>::Err(Error(ErrorCode::HashError, "Message must be 32 bytes (hash)"));
    }
    
    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, pubkey.data(), ED25519_PUBKEY_SIZE);
    if (!pkey) {
        return Result<bool>::Err(Error(ErrorCode::InvalidPublicKey, "Failed to create Ed25519 key"));
    }
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return Result<bool>::Err(Error(ErrorCode::SignatureVerificationFailed, "Failed to create context"));
    }
    
    bool valid = false;
    if (EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey) > 0 &&
        EVP_DigestVerify(ctx, signature.data(), signature.size(), message.data(), message.size()) > 0) {
        valid = true;
    }
    
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    return Result<bool>::Ok(valid);
#else
    return Result<bool>::Err(Error(ErrorCode::SignatureVerificationFailed, "OpenSSL not available. Ed25519 requires OpenSSL."));
#endif
}

} // namespace pqc_ledger::crypto

