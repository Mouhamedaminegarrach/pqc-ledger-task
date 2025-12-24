#pragma once

#include "../error.hpp"
#include "../types.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace pqc_ledger::crypto {

/**
 * Compute SHA256 hash of data.
 * 
 * @param data Input data to hash
 * @return Result containing 32-byte hash or error
 */
Result<std::vector<uint8_t>> sha256(const std::vector<uint8_t>& data);

/**
 * Compute SHA256 hash of multiple byte vectors concatenated.
 * 
 * @param parts Vector of byte vectors to concatenate and hash
 * @return Result containing 32-byte hash or error
 */
Result<std::vector<uint8_t>> sha256_concat(const std::vector<std::vector<uint8_t>>& parts);

/**
 * Create domain-separated signing message.
 * Format: SHA256("TXv1" || chain_id_be || canonical_encode(tx_without_sigs))
 * 
 * @param chain_id Chain ID (big-endian)
 * @param tx_data Canonically encoded transaction (without signatures)
 * @return Result containing 32-byte message hash or error
 */
Result<std::vector<uint8_t>> create_signing_message(uint32_t chain_id, 
                                                     const std::vector<uint8_t>& tx_data);

} // namespace pqc_ledger::crypto

