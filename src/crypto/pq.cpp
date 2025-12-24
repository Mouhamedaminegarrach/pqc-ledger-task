#include "pqc_ledger/crypto/pq.hpp"
#include <oqs/oqs.h>
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace pqc_ledger::crypto {

namespace {
    // Map algorithm name to liboqs algorithm string
    const char* get_oqs_alg_name(const std::string& algorithm) {
        if (algorithm == "Dilithium2") {
            return "Dilithium2";
        } else if (algorithm == "Dilithium3") {
            return "Dilithium3";
        } else if (algorithm == "Dilithium5") {
            return "Dilithium5";
        }
        return nullptr;
    }
}

Result<std::pair<PublicKey, std::vector<uint8_t>>> generate_keypair(const std::string& algorithm) {
    const char* alg_name = get_oqs_alg_name(algorithm);
    if (!alg_name) {
        return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Err(
            Error(ErrorCode::KeyGenerationFailed, "Unknown algorithm: " + algorithm));
    }
    
    OQS_SIG* sig = OQS_SIG_new(alg_name);
    if (sig == nullptr) {
        return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Err(
            Error(ErrorCode::KeyGenerationFailed, 
                  "Algorithm " + algorithm + " not enabled at compile-time or not available"));
    }
    
    PublicKey pubkey(sig->length_public_key);
    std::vector<uint8_t> privkey(sig->length_secret_key);
    
    OQS_STATUS status = OQS_SIG_keypair(sig, pubkey.data(), privkey.data());
    
    if (status != OQS_SUCCESS) {
        OQS_SIG_free(sig);
        return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Err(
            Error(ErrorCode::KeyGenerationFailed, "Key generation failed with status: " + std::to_string(status)));
    }
    
    OQS_SIG_free(sig);
    
    return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Ok({std::move(pubkey), std::move(privkey)});
}

Result<PublicKey> load_public_key(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<PublicKey>::Err(Error(ErrorCode::FileReadError, "Cannot open file: " + path));
    }
    
    std::vector<uint8_t> key((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    file.close();
    
    return Result<PublicKey>::Ok(std::move(key));
}

Result<std::vector<uint8_t>> load_private_key(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::FileReadError, "Cannot open file: " + path));
    }
    
    std::vector<uint8_t> key((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    file.close();
    
    return Result<std::vector<uint8_t>>::Ok(std::move(key));
}

Result<void> save_public_key(const PublicKey& pubkey, const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<void>::Err(Error(ErrorCode::FileWriteError, "Cannot open file for writing: " + path));
    }
    
    file.write(reinterpret_cast<const char*>(pubkey.data()), pubkey.size());
    file.close();
    
    return Result<void>::Ok();
}

Result<void> save_private_key(const std::vector<uint8_t>& privkey, const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<void>::Err(Error(ErrorCode::FileWriteError, "Cannot open file for writing: " + path));
    }
    
    file.write(reinterpret_cast<const char*>(privkey.data()), privkey.size());
    file.close();
    
    return Result<void>::Ok();
}

Result<Signature> sign(const std::vector<uint8_t>& message,
                       const std::vector<uint8_t>& privkey,
                       const std::string& algorithm) {
    const char* alg_name = get_oqs_alg_name(algorithm);
    if (!alg_name) {
        return Result<Signature>::Err(
            Error(ErrorCode::SignatureVerificationFailed, "Unknown algorithm: " + algorithm));
    }
    
    OQS_SIG* sig = OQS_SIG_new(alg_name);
    if (sig == nullptr) {
        return Result<Signature>::Err(
            Error(ErrorCode::SignatureVerificationFailed,
                  "Algorithm " + algorithm + " not enabled at compile-time or not available"));
    }
    
    // Verify private key size matches expected
    if (privkey.size() != sig->length_secret_key) {
        OQS_SIG_free(sig);
        return Result<Signature>::Err(
            Error(ErrorCode::InvalidPublicKey,
                  "Private key size mismatch: expected " + std::to_string(sig->length_secret_key) +
                  ", got " + std::to_string(privkey.size())));
    }
    
    Signature signature(sig->length_signature);
    size_t signature_len = 0;
    
    OQS_STATUS status = OQS_SIG_sign(sig, signature.data(), &signature_len,
                                     message.data(), message.size(), privkey.data());
    
    if (status != OQS_SUCCESS) {
        OQS_SIG_free(sig);
        return Result<Signature>::Err(
            Error(ErrorCode::SignatureVerificationFailed,
                  "Signing failed with status: " + std::to_string(status)));
    }
    
    // Resize signature to actual length (though it should match expected)
    signature.resize(signature_len);
    
    OQS_SIG_free(sig);
    
    return Result<Signature>::Ok(std::move(signature));
}

Result<bool> verify(const std::vector<uint8_t>& message,
                    const Signature& signature,
                    const PublicKey& pubkey,
                    const std::string& algorithm) {
    const char* alg_name = get_oqs_alg_name(algorithm);
    if (!alg_name) {
        return Result<bool>::Err(
            Error(ErrorCode::SignatureVerificationFailed, "Unknown algorithm: " + algorithm));
    }
    
    OQS_SIG* sig = OQS_SIG_new(alg_name);
    if (sig == nullptr) {
        return Result<bool>::Err(
            Error(ErrorCode::SignatureVerificationFailed,
                  "Algorithm " + algorithm + " not enabled at compile-time or not available"));
    }
    
    // Verify key sizes match expected
    if (pubkey.size() != sig->length_public_key) {
        OQS_SIG_free(sig);
        return Result<bool>::Ok(false);  // Invalid key size, verification fails
    }
    
    if (signature.size() != sig->length_signature) {
        OQS_SIG_free(sig);
        return Result<bool>::Ok(false);  // Invalid signature size, verification fails
    }
    
    OQS_STATUS status = OQS_SIG_verify(sig, message.data(), message.size(),
                                       signature.data(), signature.size(), pubkey.data());
    
    OQS_SIG_free(sig);
    
    if (status == OQS_SUCCESS) {
        return Result<bool>::Ok(true);
    } else {
        return Result<bool>::Ok(false);  // Verification failed, but return false (not error)
    }
}

Result<size_t> get_pubkey_size(const std::string& algorithm) {
    const char* alg_name = get_oqs_alg_name(algorithm);
    if (!alg_name) {
        return Result<size_t>::Err(Error(ErrorCode::InvalidPublicKey, "Unknown algorithm: " + algorithm));
    }
    
    OQS_SIG* sig = OQS_SIG_new(alg_name);
    if (sig == nullptr) {
        return Result<size_t>::Err(Error(ErrorCode::InvalidPublicKey,
                                         "Algorithm " + algorithm + " not available"));
    }
    
    size_t pubkey_size = sig->length_public_key;
    OQS_SIG_free(sig);
    
    return Result<size_t>::Ok(pubkey_size);
}

Result<size_t> get_signature_size(const std::string& algorithm) {
    const char* alg_name = get_oqs_alg_name(algorithm);
    if (!alg_name) {
        return Result<size_t>::Err(Error(ErrorCode::InvalidSignature, "Unknown algorithm: " + algorithm));
    }
    
    OQS_SIG* sig = OQS_SIG_new(alg_name);
    if (sig == nullptr) {
        return Result<size_t>::Err(Error(ErrorCode::InvalidSignature,
                                         "Algorithm " + algorithm + " not available"));
    }
    
    size_t sig_size = sig->length_signature;
    OQS_SIG_free(sig);
    
    return Result<size_t>::Ok(sig_size);
}

} // namespace pqc_ledger::crypto

