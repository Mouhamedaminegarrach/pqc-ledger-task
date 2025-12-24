#include "pqc_ledger/crypto/hash.hpp"
#include <cstring>

#ifdef HAVE_OPENSSL
#include <openssl/sha.h>
#endif

namespace pqc_ledger::crypto {

Result<std::vector<uint8_t>> sha256(const std::vector<uint8_t>& data) {
#ifdef HAVE_OPENSSL
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    
    SHA256_CTX ctx;
    if (SHA256_Init(&ctx) != 1) {
        return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::HashError, "SHA256_Init failed"));
    }
    
    if (SHA256_Update(&ctx, data.data(), data.size()) != 1) {
        return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::HashError, "SHA256_Update failed"));
    }
    
    if (SHA256_Final(hash.data(), &ctx) != 1) {
        return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::HashError, "SHA256_Final failed"));
    }
    
    return Result<std::vector<uint8_t>>::Ok(std::move(hash));
#else
    return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::HashError, "OpenSSL not available. SHA256 requires OpenSSL."));
#endif
}

Result<std::vector<uint8_t>> sha256_concat(const std::vector<std::vector<uint8_t>>& parts) {
#ifdef HAVE_OPENSSL
    SHA256_CTX ctx;
    if (SHA256_Init(&ctx) != 1) {
        return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::HashError, "SHA256_Init failed"));
    }
    
    for (const auto& part : parts) {
        if (SHA256_Update(&ctx, part.data(), part.size()) != 1) {
            return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::HashError, "SHA256_Update failed"));
        }
    }
    
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    if (SHA256_Final(hash.data(), &ctx) != 1) {
        return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::HashError, "SHA256_Final failed"));
    }
    
    return Result<std::vector<uint8_t>>::Ok(std::move(hash));
#else
    return Result<std::vector<uint8_t>>::Err(Error(ErrorCode::HashError, "OpenSSL not available. SHA256 requires OpenSSL."));
#endif
}

Result<std::vector<uint8_t>> create_signing_message(uint32_t chain_id,
                                                     const std::vector<uint8_t>& tx_data) {
    // Format: SHA256("TXv1" || chain_id_be || canonical_encode(tx_without_sigs))
    const std::string domain_prefix = "TXv1";
    
    // Convert chain_id to big-endian bytes
    std::vector<uint8_t> chain_id_be(4);
    chain_id_be[0] = static_cast<uint8_t>((chain_id >> 24) & 0xFF);
    chain_id_be[1] = static_cast<uint8_t>((chain_id >> 16) & 0xFF);
    chain_id_be[2] = static_cast<uint8_t>((chain_id >> 8) & 0xFF);
    chain_id_be[3] = static_cast<uint8_t>(chain_id & 0xFF);
    
    // Concatenate: "TXv1" || chain_id_be || tx_data
    std::vector<std::vector<uint8_t>> parts;
    parts.push_back(std::vector<uint8_t>(domain_prefix.begin(), domain_prefix.end()));
    parts.push_back(std::move(chain_id_be));
    parts.push_back(tx_data);
    
    return sha256_concat(parts);
}

} // namespace pqc_ledger::crypto

