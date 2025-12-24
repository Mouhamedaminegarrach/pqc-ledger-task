#pragma once

#include "../types.hpp"
#include "../error.hpp"
#include <vector>
#include <cstdint>

namespace pqc_ledger::codec {

/**
 * Canonically encode a transaction to binary format.
 * 
 * Encoding rules:
 * - All integers are big-endian
 * - Variable bytes: len (u16 BE) || bytes
 * - Fixed fields (like 'to') have no length prefix
 * - auth_tag: u8 (0=pq-only, 1=hybrid)
 * 
 * @param tx Transaction to encode
 * @return Result containing encoded bytes or error
 */
Result<std::vector<uint8_t>> encode(const Transaction& tx);

/**
 * Encode transaction without signatures (for signing).
 * This excludes the auth field.
 * 
 * @param tx Transaction to encode (signatures are ignored)
 * @return Result containing encoded bytes or error
 */
Result<std::vector<uint8_t>> encode_for_signing(const Transaction& tx);

/**
 * Encode bytes to hex string.
 * 
 * @param bytes Binary data to encode
 * @return Hex string representation
 */
std::string encode_to_hex(const std::vector<uint8_t>& bytes);

/**
 * Encode bytes to base64 string.
 * 
 * @param bytes Binary data to encode
 * @return Base64 string representation
 */
std::string encode_to_base64(const std::vector<uint8_t>& bytes);

/**
 * Encode transaction to hex string.
 * 
 * @param tx Transaction to encode
 * @return Result containing hex string or error
 */
Result<std::string> encode_to_hex(const Transaction& tx);

/**
 * Encode transaction to base64 string.
 * 
 * @param tx Transaction to encode
 * @return Result containing base64 string or error
 */
Result<std::string> encode_to_base64(const Transaction& tx);

} // namespace pqc_ledger::codec

