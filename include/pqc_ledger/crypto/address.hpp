#pragma once

#include "../types.hpp"
#include "../error.hpp"

namespace pqc_ledger::crypto {

/**
 * Derive address from public key.
 * Address = first_32_bytes(SHA256(from_pubkey_bytes))
 * 
 * @param pubkey Public key bytes
 * @return Result containing 32-byte address or error
 */
Result<Address> derive_address(const PublicKey& pubkey);

/**
 * Convert address to hex string.
 * 
 * @param addr Address to convert
 * @return Hex string representation
 */
std::string address_to_hex(const Address& addr);

/**
 * Convert hex string to address.
 * 
 * @param hex Hex string (must be 64 characters)
 * @return Result containing address or error
 */
Result<Address> address_from_hex(const std::string& hex);

} // namespace pqc_ledger::crypto

