#include "pqc_ledger/crypto/pq.hpp"
#include <oqs/oqs.h>
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace pqc_ledger::crypto {

namespace {
    // Map algorithm name to liboqs algorithm string
    // Note: Modern liboqs uses ML-DSA (NIST standard) instead of Dilithium
    // ML-DSA-65 is roughly equivalent to Dilithium3
    const char* get_oqs_alg_name(const std::string& algorithm) {
        if (algorithm == "Dilithium3" || algorithm == "Dilithium-3" || algorithm == "ML-DSA-65") {
            // Use ML-DSA-65 (NIST standard, equivalent to Dilithium3)
            return "ML-DSA-65";
        } else if (algorithm == "Dilithium2" || algorithm == "Dilithium-2" || algorithm == "ML-DSA-44") {
            // Use ML-DSA-44 (NIST standard, equivalent to Dilithium2)
            return "ML-DSA-44";
        } else if (algorithm == "Dilithium5" || algorithm == "Dilithium-5" || algorithm == "ML-DSA-87") {
            // Use ML-DSA-87 (NIST standard, equivalent to Dilithium5)
            return "ML-DSA-87";
        }
        return nullptr;
    }
    
    // Try to find an available ML-DSA algorithm (NIST standard)
    const char* find_available_ml_dsa() {
        // Ensure OQS is initialized
        static bool oqs_initialized = false;
        if (!oqs_initialized) {
            OQS_init();
            oqs_initialized = true;
        }
        
        // Try ML-DSA algorithms (NIST standard names)
        const char* algorithms[] = {"ML-DSA-65", "ML-DSA-44", "ML-DSA-87", 
                                    // Also try old Dilithium names for compatibility
                                    "Dilithium3", "Dilithium-3", "Dilithium2", "Dilithium-2", 
                                    "Dilithium5", "Dilithium-5", nullptr};
        for (int i = 0; algorithms[i] != nullptr; ++i) {
            OQS_SIG* sig = OQS_SIG_new(algorithms[i]);
            if (sig != nullptr) {
                OQS_SIG_free(sig);
                return algorithms[i];
            }
        }
        return nullptr;
    }
}

Result<std::pair<PublicKey, std::vector<uint8_t>>> generate_keypair(const std::string& algorithm) {
    // Initialize OQS if not already initialized
    static bool oqs_initialized = false;
    if (!oqs_initialized) {
        OQS_init();
        oqs_initialized = true;
    }
    
    const char* alg_name = get_oqs_alg_name(algorithm);
    if (!alg_name) {
        return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Err(
            Error(ErrorCode::KeyGenerationFailed, "Unknown algorithm: " + algorithm));
    }
    
    OQS_SIG* sig = OQS_SIG_new(alg_name);
    if (sig == nullptr) {
        // Try to find any available ML-DSA algorithm as fallback
        if (algorithm == "Dilithium3" || algorithm == "Dilithium2" || algorithm == "Dilithium5" ||
            algorithm == "ML-DSA-65" || algorithm == "ML-DSA-44" || algorithm == "ML-DSA-87") {
            const char* fallback = find_available_ml_dsa();
            if (fallback) {
                sig = OQS_SIG_new(fallback);
                if (sig != nullptr) {
                    // Use the fallback algorithm
                    alg_name = fallback;
                }
            }
        }
        
        if (sig == nullptr) {
            return Result<std::pair<PublicKey, std::vector<uint8_t>>>::Err(
                Error(ErrorCode::KeyGenerationFailed, 
                      "Algorithm " + algorithm + " not enabled at compile-time or not available. "
                      "Please ensure liboqs is built with ML-DSA (or Dilithium) algorithms enabled."));
        }
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
    // Initialize OQS if not already initialized
    static bool oqs_initialized = false;
    if (!oqs_initialized) {
        OQS_init();
        oqs_initialized = true;
    }
    
    const char* alg_name = get_oqs_alg_name(algorithm);
    if (!alg_name) {
        return Result<Signature>::Err(
            Error(ErrorCode::SignatureVerificationFailed, "Unknown algorithm: " + algorithm));
    }
    
    OQS_SIG* sig = OQS_SIG_new(alg_name);
    if (sig == nullptr) {
        // Try to find any available ML-DSA algorithm as fallback
        if (algorithm == "Dilithium3" || algorithm == "Dilithium2" || algorithm == "Dilithium5" ||
            algorithm == "ML-DSA-65" || algorithm == "ML-DSA-44" || algorithm == "ML-DSA-87") {
            const char* fallback = find_available_ml_dsa();
            if (fallback) {
                sig = OQS_SIG_new(fallback);
                if (sig != nullptr) {
                    // Use the fallback algorithm
                    alg_name = fallback;
                }
            }
        }
        
        if (sig == nullptr) {
            return Result<Signature>::Err(
                Error(ErrorCode::SignatureVerificationFailed,
                      "Algorithm " + algorithm + " not enabled at compile-time or not available"));
        }
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
    // Initialize OQS if not already initialized
    static bool oqs_initialized = false;
    if (!oqs_initialized) {
        OQS_init();
        oqs_initialized = true;
    }
    
    const char* alg_name = get_oqs_alg_name(algorithm);
    if (!alg_name) {
        return Result<bool>::Err(
            Error(ErrorCode::SignatureVerificationFailed, "Unknown algorithm: " + algorithm));
    }
    
    OQS_SIG* sig = OQS_SIG_new(alg_name);
    if (sig == nullptr) {
        // Try to find any available ML-DSA algorithm as fallback
        if (algorithm == "Dilithium3" || algorithm == "Dilithium2" || algorithm == "Dilithium5" ||
            algorithm == "ML-DSA-65" || algorithm == "ML-DSA-44" || algorithm == "ML-DSA-87") {
            const char* fallback = find_available_ml_dsa();
            if (fallback) {
                sig = OQS_SIG_new(fallback);
                if (sig != nullptr) {
                    // Use the fallback algorithm
                    alg_name = fallback;
                }
            }
        }
        
        if (sig == nullptr) {
            return Result<bool>::Err(
                Error(ErrorCode::SignatureVerificationFailed,
                      "Algorithm " + algorithm + " not enabled at compile-time or not available"));
        }
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
    // Ensure OQS is initialized
    static bool oqs_initialized = false;
    if (!oqs_initialized) {
        OQS_init();
        oqs_initialized = true;
    }
    
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
    // Ensure OQS is initialized
    static bool oqs_initialized = false;
    if (!oqs_initialized) {
        OQS_init();
        oqs_initialized = true;
    }
    
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

