#pragma once

#include "../types.hpp"
#include "../error.hpp"
#include <vector>
#include <cstdint>

namespace pqc_ledger::codec {

/**
 * Strictly decode a transaction from binary format.
 * 
 * Strict decoding rules (must enforce):
 * - No trailing bytes allowed
 * - version == 1 required
 * - Length prefixes must match remaining buffer
 * - Enforce fixed sizes:
 *   - PQ pubkey length must match algorithm's expected length
 *   - PQ signature length must match expected length
 *   - (If hybrid) classical signature length must match expected length
 * 
 * @param data Binary data to decode
 * @return Result containing decoded Transaction or error
 */
Result<Transaction> decode(const std::vector<uint8_t>& data);

/**
 * Decode from hex string.
 * 
 * @param hex Hex-encoded transaction
 * @return Result containing decoded Transaction or error
 */
Result<Transaction> decode_from_hex(const std::string& hex);

/**
 * Decode from base64 string.
 * 
 * @param base64 Base64-encoded transaction
 * @return Result containing decoded Transaction or error
 */
Result<Transaction> decode_from_base64(const std::string& base64);

} // namespace pqc_ledger::codec

