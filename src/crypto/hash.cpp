#include "pqc_ledger/crypto/hash.hpp"
#include "picosha2.h"
#include <cstring>

namespace pqc_ledger::crypto {

Result<std::vector<uint8_t>> sha256(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> hash(32);  // SHA256 produces 32 bytes
    picosha2::hash256(data.begin(), data.end(), hash.begin(), hash.end());
    return Result<std::vector<uint8_t>>::Ok(std::move(hash));
}

Result<std::vector<uint8_t>> sha256_concat(const std::vector<std::vector<uint8_t>>& parts) {
    // Concatenate all parts into a single vector
    std::vector<uint8_t> concatenated;
    for (const auto& part : parts) {
        concatenated.insert(concatenated.end(), part.begin(), part.end());
    }
    
    // Hash the concatenated data
    std::vector<uint8_t> hash(32);  // SHA256 produces 32 bytes
    picosha2::hash256(concatenated.begin(), concatenated.end(), hash.begin(), hash.end());
    return Result<std::vector<uint8_t>>::Ok(std::move(hash));
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

